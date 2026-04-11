#include "api/Sync.hpp"
#include "logging/Logger.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace cweman {

namespace {

std::filesystem::path sync_state_path() {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::filesystem::path{home} / ".cwe-man" / "sync_state.txt";
}

} // namespace

Sync::Sync(Repository& repo) : repo_{repo} {}

bool Sync::run(int view_id,
               SyncProgressCallback on_progress,
               SyncCompleteCallback on_complete) {
    auto& log = Logger::instance();

    try {
        // Step 1: Fetch view
        log.info(std::format("Starting sync for view {}", view_id));
        if (on_progress) on_progress("Fetching view metadata...", 0, 0);

        auto view_json = client_.get_view(view_id);
        auto view_data = nlohmann::json::parse(view_json);

        auto& views = view_data["Views"];
        if (views.empty()) {
            throw std::runtime_error("Empty view response");
        }

        auto& view = views[0];
        auto& members = view["Members"];
        int total_categories = static_cast<int>(members.size());

        log.info(std::format("View {} has {} categories",
                             view_id, total_categories));

        // Step 2: Fetch each category and its weaknesses
        for (int i = 0; i < total_categories; ++i) {
            int cat_id = std::stoi(members[i]["CweID"].get<std::string>());

            if (on_progress) {
                on_progress(
                    std::format("Fetching category {} ({}/{})...",
                                cat_id, i + 1, total_categories),
                    i + 1, total_categories);
            }

            // Fetch category
            auto cat_json = client_.get_category(cat_id);
            auto cat_data = nlohmann::json::parse(cat_json);

            if (!cat_data.contains("Categories") ||
                cat_data["Categories"].empty()) {
                log.warn(std::format("Category {} returned no data", cat_id));
                continue;
            }

            auto& cat = cat_data["Categories"][0];

            // Store category
            Category category{
                .id      = std::stoi(cat["ID"].get<std::string>()),
                .name    = cat.value("Name", ""),
                .summary = cat.value("Summary", ""),
                .view_id = view_id,
            };
            repo_.upsert_category(category);

            // Collect weakness IDs from Relationships
            std::vector<int> weakness_ids;
            if (cat.contains("Relationships")) {
                for (const auto& rel : cat["Relationships"]) {
                    int cwe_id = std::stoi(rel["CweID"].get<std::string>());
                    weakness_ids.push_back(cwe_id);
                }
            }

            if (weakness_ids.empty()) continue;

            // Batch-fetch weaknesses (in groups of 20 to avoid overly long URLs)
            constexpr int BATCH_SIZE = 20;
            for (size_t start = 0; start < weakness_ids.size();
                 start += BATCH_SIZE) {
                size_t end = std::min(start + BATCH_SIZE, weakness_ids.size());
                std::string comma_ids;
                for (size_t j = start; j < end; ++j) {
                    if (!comma_ids.empty()) comma_ids += ',';
                    comma_ids += std::to_string(weakness_ids[j]);
                }

                auto weak_json = client_.get_weaknesses(comma_ids);
                auto weak_data = nlohmann::json::parse(weak_json);

                if (!weak_data.contains("Weaknesses")) continue;

                for (const auto& w : weak_data["Weaknesses"]) {
                    Cwe cwe;
                    cwe.id   = std::stoi(w["ID"].get<std::string>());
                    cwe.name = w.value("Name", "");
                    cwe.abstraction = w.value("Abstraction", "");
                    cwe.status = w.value("Status", "");
                    cwe.description = w.value("Description", "");
                    cwe.likelihood_of_exploit =
                        w.value("LikelihoodOfExploit", "");
                    cwe.url = std::format(
                        "https://cwe.mitre.org/data/definitions/{}.html",
                        cwe.id);

                    // Consequences
                    if (w.contains("CommonConsequences")) {
                        for (const auto& c : w["CommonConsequences"]) {
                            Consequence con;
                            if (c.contains("Scope")) {
                                con.scopes =
                                    c["Scope"].get<std::vector<std::string>>();
                            }
                            if (c.contains("Impact")) {
                                con.impacts =
                                    c["Impact"].get<std::vector<std::string>>();
                            }
                            if (c.contains("Note")) {
                                con.note = c["Note"].get<std::string>();
                            }
                            cwe.consequences.push_back(std::move(con));
                        }
                    }

                    // Mitigations
                    if (w.contains("PotentialMitigations")) {
                        for (const auto& m : w["PotentialMitigations"]) {
                            Mitigation mit;
                            if (m.contains("Phase")) {
                                // Phase can be an array
                                auto& phase = m["Phase"];
                                if (phase.is_array()) {
                                    std::string phases;
                                    for (const auto& p : phase) {
                                        if (!phases.empty()) phases += ", ";
                                        phases += p.get<std::string>();
                                    }
                                    mit.phase = phases;
                                } else {
                                    mit.phase = phase.get<std::string>();
                                }
                            }
                            mit.strategy =
                                m.value("Strategy", "");
                            mit.description =
                                m.value("Description", "");
                            cwe.mitigations.push_back(std::move(mit));
                        }
                    }

                    // Related weaknesses
                    if (w.contains("RelatedWeaknesses")) {
                        for (const auto& r : w["RelatedWeaknesses"]) {
                            int rid = std::stoi(
                                r["CweID"].get<std::string>());
                            cwe.related_weakness_ids.push_back(rid);
                        }
                    }

                    repo_.upsert_cwe(cwe);
                    repo_.link_cwe_category(cwe.id, category.id);
                }
            }

            log.info(std::format("Synced category {}: {} ({} weaknesses)",
                                 category.id, category.name,
                                 weakness_ids.size()));
        }

        // Step 3: Write sync timestamp
        write_sync_timestamp();

        log.info("Sync completed successfully");
        if (on_complete) on_complete(true, "Sync completed successfully");
        return true;

    } catch (const std::exception& e) {
        log.error(std::format("Sync failed: {}", e.what()));
        if (on_complete) {
            on_complete(false, std::format("Sync failed: {}", e.what()));
        }
        return false;
    }
}

std::string Sync::read_sync_timestamp() {
    auto path = sync_state_path();
    if (!std::filesystem::exists(path)) return {};

    std::ifstream f{path};
    std::string ts;
    std::getline(f, ts);
    return ts;
}

void Sync::write_sync_timestamp() {
    auto path = sync_state_path();
    std::filesystem::create_directories(path.parent_path());

    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::localtime(&tt));
    std::string ts = std::format("{} {}", buf, "CDT");

    std::ofstream f{path};
    f << ts << '\n';

    Logger::instance().info(std::format("Wrote sync timestamp: {}", ts));
}

bool Sync::sync_needed(int max_age_days) {
    auto ts = read_sync_timestamp();
    if (ts.empty()) return true;

    // Stored format: YYYY-MM-DDTHH:MM:SS TZ (e.g., CDT).
    // Keep compatibility with older timestamps without a timezone suffix.
    auto raw_ts = ts;
    size_t space = raw_ts.find(' ');
    if (space != std::string::npos) {
        raw_ts = raw_ts.substr(0, space);
    }

    std::tm tm{};
    std::istringstream ss{raw_ts};
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return true;

    auto last_sync = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto now       = std::chrono::system_clock::now();
    auto age       = std::chrono::duration_cast<std::chrono::hours>(
                         now - last_sync);

    return age.count() > max_age_days * 24;
}

} // namespace cweman
