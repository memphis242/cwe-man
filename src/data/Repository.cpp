#include "data/Repository.hpp"
#include "logging/Logger.hpp"

#include <format>
#include <nlohmann/json.hpp>

namespace cweman {

static constexpr std::string_view SCHEMA_SQL = R"(
CREATE TABLE IF NOT EXISTS categories (
    id          INTEGER PRIMARY KEY,
    name        TEXT NOT NULL,
    summary     TEXT,
    view_id     INTEGER NOT NULL DEFAULT 699
);

CREATE TABLE IF NOT EXISTS cwes (
    id                    INTEGER PRIMARY KEY,
    name                  TEXT NOT NULL,
    abstraction           TEXT,
    status                TEXT,
    description           TEXT,
    extended_description  TEXT,
    likelihood_of_exploit TEXT,
    consequences          TEXT,
    mitigations           TEXT,
    related_weaknesses    TEXT,
    url                   TEXT
);

CREATE TABLE IF NOT EXISTS cwe_categories (
    cwe_id      INTEGER REFERENCES cwes(id),
    category_id INTEGER REFERENCES categories(id),
    PRIMARY KEY (cwe_id, category_id)
);

CREATE TABLE IF NOT EXISTS notifications (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    message   TEXT NOT NULL,
    severity  TEXT NOT NULL,
    timestamp TEXT NOT NULL,
    read      INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_cwes_name ON cwes(name);
CREATE INDEX IF NOT EXISTS idx_notifications_read ON notifications(read);
)";

Repository::Repository(Database& db) : db_{db} {}

void Repository::initialize_schema() {
    db_.execute(SCHEMA_SQL);
    Logger::instance().info("Database schema initialized");
}

// ── Categories ──────────────────────────────────────────────────────────────

std::vector<Category> Repository::get_categories(int view_id) {
    auto stmt = db_.prepare(
        "SELECT id, name, summary, view_id FROM categories "
        "WHERE view_id = ? ORDER BY name");
    stmt.bind_int(1, view_id);

    std::vector<Category> result;
    while (stmt.step()) {
        result.push_back({
            .id      = stmt.column_int(0),
            .name    = stmt.column_text(1),
            .summary = stmt.column_text(2),
            .view_id = stmt.column_int(3),
        });
    }
    return result;
}

void Repository::upsert_category(const Category& cat) {
    auto stmt = db_.prepare(
        "INSERT INTO categories (id, name, summary, view_id) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name, "
        "summary=excluded.summary, view_id=excluded.view_id");
    stmt.bind_int(1, cat.id);
    stmt.bind_text(2, cat.name);
    stmt.bind_text(3, cat.summary);
    stmt.bind_int(4, cat.view_id);
    stmt.execute();
}

// ── Weaknesses ──────────────────────────────────────────────────────────────

static Cwe row_to_cwe(Statement& stmt) {
    Cwe cwe;
    cwe.id                    = stmt.column_int(0);
    cwe.name                  = stmt.column_text(1);
    cwe.abstraction           = stmt.column_text(2);
    cwe.status                = stmt.column_text(3);
    cwe.description           = stmt.column_text(4);
    cwe.extended_description  = stmt.column_text(5);
    cwe.likelihood_of_exploit = stmt.column_text(6);
    cwe.url                   = stmt.column_text(10);

    // Deserialize JSON arrays
    auto parse_json = [](const std::string& raw) -> nlohmann::json {
        if (raw.empty()) return nlohmann::json::array();
        try { return nlohmann::json::parse(raw); }
        catch (...) { return nlohmann::json::array(); }
    };

    auto cons_json = parse_json(stmt.column_text(7));
    for (const auto& c : cons_json) {
        Consequence con;
        if (c.contains("scopes"))  con.scopes  = c["scopes"].get<std::vector<std::string>>();
        if (c.contains("impacts")) con.impacts  = c["impacts"].get<std::vector<std::string>>();
        if (c.contains("note"))    con.note     = c["note"].get<std::string>();
        cwe.consequences.push_back(std::move(con));
    }

    auto mit_json = parse_json(stmt.column_text(8));
    for (const auto& m : mit_json) {
        Mitigation mit;
        if (m.contains("phase"))       mit.phase       = m["phase"].get<std::string>();
        if (m.contains("strategy"))    mit.strategy    = m["strategy"].get<std::string>();
        if (m.contains("description")) mit.description = m["description"].get<std::string>();
        cwe.mitigations.push_back(std::move(mit));
    }

    auto rel_json = parse_json(stmt.column_text(9));
    for (const auto& r : rel_json) {
        cwe.related_weakness_ids.push_back(r.get<int>());
    }

    return cwe;
}

std::vector<Cwe> Repository::get_cwes_for_category(int category_id) {
    auto stmt = db_.prepare(
        "SELECT c.id, c.name, c.abstraction, c.status, c.description, "
        "c.extended_description, c.likelihood_of_exploit, "
        "c.consequences, c.mitigations, c.related_weaknesses, c.url "
        "FROM cwes c "
        "JOIN cwe_categories cc ON c.id = cc.cwe_id "
        "WHERE cc.category_id = ? ORDER BY c.id");
    stmt.bind_int(1, category_id);

    std::vector<Cwe> result;
    while (stmt.step()) {
        result.push_back(row_to_cwe(stmt));
    }
    return result;
}

std::optional<Cwe> Repository::get_cwe(int id) {
    auto stmt = db_.prepare(
        "SELECT id, name, abstraction, status, description, "
        "extended_description, likelihood_of_exploit, "
        "consequences, mitigations, related_weaknesses, url "
        "FROM cwes WHERE id = ?");
    stmt.bind_int(1, id);
    if (stmt.step()) {
        return row_to_cwe(stmt);
    }
    return std::nullopt;
}

std::vector<Cwe> Repository::search_cwes(const std::string& query) {
    // Try to parse as integer for ID-based search
    bool is_numeric = !query.empty() &&
        std::all_of(query.begin(), query.end(), ::isdigit);

    std::string sql =
        "SELECT id, name, abstraction, status, description, "
        "extended_description, likelihood_of_exploit, "
        "consequences, mitigations, related_weaknesses, url "
        "FROM cwes WHERE ";

    if (is_numeric) {
        sql += "CAST(id AS TEXT) LIKE ? OR name LIKE ?";
    } else {
        sql += "name LIKE ?";
    }
    sql += " ORDER BY id LIMIT 50";

    auto stmt = db_.prepare(sql);
    std::string pattern = "%" + query + "%";

    if (is_numeric) {
        stmt.bind_text(1, pattern);
        stmt.bind_text(2, pattern);
    } else {
        stmt.bind_text(1, pattern);
    }

    std::vector<Cwe> result;
    while (stmt.step()) {
        result.push_back(row_to_cwe(stmt));
    }
    return result;
}

std::vector<CwePrintRow> Repository::get_cwes_for_print() {
    auto stmt = db_.prepare(
        "SELECT cat.name, c.id, c.name, c.description, c.url "
        "FROM categories cat "
        "JOIN cwe_categories cc ON cat.id = cc.category_id "
        "JOIN cwes c ON c.id = cc.cwe_id "
        "ORDER BY cat.name COLLATE NOCASE, c.id");

    std::vector<CwePrintRow> result;
    while (stmt.step()) {
        result.push_back({
            .category_name = stmt.column_text(0),
            .id            = stmt.column_int(1),
            .name          = stmt.column_text(2),
            .description   = stmt.column_text(3),
            .url           = stmt.column_text(4),
        });
    }
    return result;
}

void Repository::upsert_cwe(const Cwe& cwe) {
    // Serialize structured fields to JSON
    nlohmann::json cons_json = nlohmann::json::array();
    for (const auto& c : cwe.consequences) {
        cons_json.push_back({
            {"scopes", c.scopes}, {"impacts", c.impacts}, {"note", c.note}
        });
    }

    nlohmann::json mit_json = nlohmann::json::array();
    for (const auto& m : cwe.mitigations) {
        mit_json.push_back({
            {"phase", m.phase}, {"strategy", m.strategy},
            {"description", m.description}
        });
    }

    nlohmann::json rel_json = cwe.related_weakness_ids;

    auto stmt = db_.prepare(
        "INSERT INTO cwes (id, name, abstraction, status, description, "
        "extended_description, likelihood_of_exploit, consequences, "
        "mitigations, related_weaknesses, url) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name=excluded.name, abstraction=excluded.abstraction, "
        "status=excluded.status, description=excluded.description, "
        "extended_description=excluded.extended_description, "
        "likelihood_of_exploit=excluded.likelihood_of_exploit, "
        "consequences=excluded.consequences, mitigations=excluded.mitigations, "
        "related_weaknesses=excluded.related_weaknesses, url=excluded.url");

    stmt.bind_int(1, cwe.id);
    stmt.bind_text(2, cwe.name);
    stmt.bind_text(3, cwe.abstraction);
    stmt.bind_text(4, cwe.status);
    stmt.bind_text(5, cwe.description);
    stmt.bind_text(6, cwe.extended_description);
    stmt.bind_text(7, cwe.likelihood_of_exploit);
    stmt.bind_text(8, cons_json.dump());
    stmt.bind_text(9, mit_json.dump());
    stmt.bind_text(10, rel_json.dump());
    stmt.bind_text(11, cwe.url);
    stmt.execute();
}

void Repository::link_cwe_category(int cwe_id, int category_id) {
    auto stmt = db_.prepare(
        "INSERT OR IGNORE INTO cwe_categories (cwe_id, category_id) "
        "VALUES (?, ?)");
    stmt.bind_int(1, cwe_id);
    stmt.bind_int(2, category_id);
    stmt.execute();
}

// ── State ───────────────────────────────────────────────────────────────────

bool Repository::has_data() {
    auto stmt = db_.prepare("SELECT COUNT(*) FROM categories");
    stmt.step();
    return stmt.column_int(0) > 0;
}

// ── Notifications ───────────────────────────────────────────────────────────

std::vector<Notification> Repository::get_notifications() {
    auto stmt = db_.prepare(
        "SELECT id, message, severity, timestamp, read "
        "FROM notifications ORDER BY id ASC");

    std::vector<Notification> result;
    while (stmt.step()) {
        Notification n;
        n.id       = stmt.column_int64(0);
        n.message  = stmt.column_text(1);

        auto sev   = stmt.column_text(2);
        if (sev == "Warning")      n.severity = NotificationSeverity::Warning;
        else if (sev == "Error")   n.severity = NotificationSeverity::Error;
        else if (sev == "Critical")n.severity = NotificationSeverity::Critical;
        else                       n.severity = NotificationSeverity::Info;

        // timestamp stored as ISO 8601; we don't parse it back for now
        n.read = stmt.column_int(3) != 0;
        result.push_back(std::move(n));
    }
    return result;
}

void Repository::insert_notification(const Notification& n) {
    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::localtime(&tt));

    std::string severity_str;
    switch (n.severity) {
        case NotificationSeverity::Info:     severity_str = "Info"; break;
        case NotificationSeverity::Warning:  severity_str = "Warning"; break;
        case NotificationSeverity::Error:    severity_str = "Error"; break;
        case NotificationSeverity::Critical: severity_str = "Critical"; break;
    }

    auto stmt = db_.prepare(
        "INSERT INTO notifications (message, severity, timestamp, read) "
        "VALUES (?, ?, ?, 0)");
    stmt.bind_text(1, n.message);
    stmt.bind_text(2, severity_str);
    stmt.bind_text(3, buf);
    stmt.execute();
}

void Repository::mark_notification_read(int64_t id) {
    auto stmt = db_.prepare("UPDATE notifications SET read = 1 WHERE id = ?");
    stmt.bind_int64(1, id);
    stmt.execute();
}

void Repository::delete_notification(int64_t id) {
    auto stmt = db_.prepare("DELETE FROM notifications WHERE id = ?");
    stmt.bind_int64(1, id);
    stmt.execute();
}

} // namespace cweman
