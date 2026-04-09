#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cweman {

struct Category {
    int         id{};
    std::string name;
    std::string summary;
    int         view_id{699};
};

struct Consequence {
    std::vector<std::string> scopes;
    std::vector<std::string> impacts;
    std::string              note;
};

struct Mitigation {
    std::string phase;
    std::string strategy;
    std::string description;
};

struct Cwe {
    int         id{};
    std::string name;
    std::string abstraction;
    std::string status;
    std::string description;
    std::string extended_description;
    std::string likelihood_of_exploit;
    std::vector<Consequence>  consequences;
    std::vector<Mitigation>   mitigations;
    std::vector<int>          related_weakness_ids;
    std::vector<int>          category_ids;
    std::string url;
};

enum class NotificationSeverity { Info, Warning, Error, Critical };

struct Notification {
    int64_t                                id{};
    std::string                            message;
    NotificationSeverity                   severity{NotificationSeverity::Info};
    std::chrono::system_clock::time_point  timestamp{};
    bool                                   read{false};
};

struct SyncState {
    std::optional<std::chrono::system_clock::time_point> last_sync;
    std::string api_version;
};

// Represents a single visible row in the tree (either a category header or a CWE leaf).
struct TreeNode {
    enum class Kind { Category, Weakness };
    Kind        kind;
    int         id;
    std::string label;
    int         category_id;   // parent category (same as id when kind == Category)
    bool        expanded;      // only meaningful for Category nodes
};

} // namespace cweman
