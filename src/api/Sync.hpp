#pragma once

#include "api/CweClient.hpp"
#include "data/Repository.hpp"

#include <atomic>
#include <functional>
#include <string>

namespace cweman {

// Progress callback: (message, current_step, total_steps)
using SyncProgressCallback =
    std::function<void(const std::string&, int, int)>;

// Completion callback: (success, message)
using SyncCompleteCallback =
    std::function<void(bool, const std::string&)>;

class Sync {
public:
    explicit Sync(Repository& repo);

    // Run synchronously (call from a background thread if needed).
    // Returns true on success.
    bool run(int view_id = 699,
             SyncProgressCallback on_progress = {},
             SyncCompleteCallback on_complete = {});

    // Read/write the sync timestamp file.
    static std::string read_sync_timestamp();
    static void        write_sync_timestamp();
    static bool        sync_needed(int max_age_days = 30);

private:
    void parse_and_store_view(const std::string& json_str, int view_id,
                              SyncProgressCallback& on_progress);
    void parse_and_store_category(const std::string& json_str);
    void parse_and_store_weaknesses(const std::string& json_str);

    Repository& repo_;
    CweClient   client_;
};

} // namespace cweman
