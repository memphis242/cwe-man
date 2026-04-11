#pragma once

#include "data/Database.hpp"
#include "data/Models.hpp"

#include <optional>
#include <string>
#include <vector>

namespace cweman {

class Repository {
public:
    explicit Repository(Database& db);

    // Schema
    void initialize_schema();

    // Categories
    std::vector<Category>  get_categories(int view_id = 699);
    void                   upsert_category(const Category& cat);

    // Weaknesses
    std::vector<Cwe>       get_cwes_for_category(int category_id);
    std::optional<Cwe>     get_cwe(int id);
    std::vector<Cwe>       search_cwes(const std::string& query);
    std::vector<CwePrintRow> get_cwes_for_print();
    void                   upsert_cwe(const Cwe& cwe);
    void                   link_cwe_category(int cwe_id, int category_id);

    // State
    bool has_data();

    // Notifications
    std::vector<Notification> get_notifications();
    void insert_notification(const Notification& n);
    void mark_notification_read(int64_t id);
    void set_notification_read(int64_t id, bool read);
    void delete_notification(int64_t id);

private:
    Database& db_;
};

} // namespace cweman
