#include "TestUtils.hpp"
#include "data/Database.hpp"
#include "data/Repository.hpp"

#include <gtest/gtest.h>

namespace cweman {

namespace {

Cwe make_sample_cwe(int id, const std::string& name) {
    Cwe cwe;
    cwe.id = id;
    cwe.name = name;
    cwe.abstraction = "Base";
    cwe.status = "Draft";
    cwe.description = "desc";
    cwe.extended_description = "ext";
    cwe.likelihood_of_exploit = "Low";
    cwe.url = "https://cwe.mitre.org/data/definitions/" + std::to_string(id) + ".html";

    Consequence con;
    con.scopes = {"Confidentiality"};
    con.impacts = {"Read Data"};
    con.note = "note";
    cwe.consequences.push_back(con);

    Mitigation mit;
    mit.phase = "Implementation";
    mit.strategy = "Input Validation";
    mit.description = "Validate input";
    cwe.mitigations.push_back(mit);

    cwe.related_weakness_ids = {79, 89};
    return cwe;
}

} // namespace

TEST(RepositoryTest, UpsertAndFetchCategoryAndCwe) {
    auto temp_dir = test::make_temp_dir("cwe-man-repo");
    Database db(temp_dir / "repo.db");
    Repository repo(db);
    repo.initialize_schema();

    Category cat{.id = 1000, .name = "Input Validation", .summary = "s", .view_id = 699};
    repo.upsert_category(cat);

    auto cwe = make_sample_cwe(79, "Cross-site Scripting");
    repo.upsert_cwe(cwe);
    repo.link_cwe_category(cwe.id, cat.id);

    auto categories = repo.get_categories();
    ASSERT_EQ(categories.size(), 1u);
    EXPECT_EQ(categories[0].id, 1000);

    auto loaded = repo.get_cwe(79);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->name, "Cross-site Scripting");
    ASSERT_EQ(loaded->consequences.size(), 1u);
    ASSERT_EQ(loaded->mitigations.size(), 1u);
    ASSERT_EQ(loaded->related_weakness_ids.size(), 2u);

    auto in_category = repo.get_cwes_for_category(1000);
    ASSERT_EQ(in_category.size(), 1u);
    EXPECT_EQ(in_category[0].id, 79);
}

TEST(RepositoryTest, SearchCwesByNameAndId) {
    auto temp_dir = test::make_temp_dir("cwe-man-repo-search");
    Database db(temp_dir / "repo.db");
    Repository repo(db);
    repo.initialize_schema();

    repo.upsert_cwe(make_sample_cwe(79, "Cross-site Scripting"));
    repo.upsert_cwe(make_sample_cwe(89, "SQL Injection"));

    auto by_name = repo.search_cwes("SQL");
    ASSERT_EQ(by_name.size(), 1u);
    EXPECT_EQ(by_name[0].id, 89);

    auto by_id = repo.search_cwes("79");
    ASSERT_FALSE(by_id.empty());
    EXPECT_EQ(by_id.front().id, 79);
}

TEST(RepositoryTest, MalformedJsonFieldsDoNotCrashLoad) {
    auto temp_dir = test::make_temp_dir("cwe-man-repo-json");
    Database db(temp_dir / "repo.db");
    Repository repo(db);
    repo.initialize_schema();

    db.execute(
        "INSERT INTO cwes (id, name, abstraction, status, description, extended_description,"
        " likelihood_of_exploit, consequences, mitigations, related_weaknesses, url)"
        " VALUES (1, 'x', 'a', 's', 'd', '', '', '{bad json', '[1,2,', '[oops', 'u')");

    auto loaded = repo.get_cwe(1);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->consequences.empty());
    EXPECT_TRUE(loaded->mitigations.empty());
    EXPECT_TRUE(loaded->related_weakness_ids.empty());
}

TEST(RepositoryTest, NotificationCrudWorks) {
    auto temp_dir = test::make_temp_dir("cwe-man-repo-notif");
    Database db(temp_dir / "repo.db");
    Repository repo(db);
    repo.initialize_schema();

    Notification n;
    n.message = "sync failed";
    n.severity = NotificationSeverity::Error;

    repo.insert_notification(n);
    auto list = repo.get_notifications();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_FALSE(list[0].read);

    repo.mark_notification_read(list[0].id);
    list = repo.get_notifications();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_TRUE(list[0].read);

    repo.set_notification_read(list[0].id, false);
    list = repo.get_notifications();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_FALSE(list[0].read);

    repo.delete_notification(list[0].id);
    EXPECT_TRUE(repo.get_notifications().empty());
}

} // namespace cweman
