#include "data/Database.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <random>
#include <string>

namespace cweman {
namespace {

std::filesystem::path make_temp_dir() {
    auto base = std::filesystem::temp_directory_path();
    auto seed = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937_64 rng(seed);
    auto dir = base / ("cwe-man-db-test-" + std::to_string(rng()));
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

TEST(DatabaseTest, CreateInsertSelectRoundTrip) {
    auto temp_dir = make_temp_dir();
    auto db_path = temp_dir / "test.db";

    Database db(db_path);
    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT NOT NULL)");

    auto insert = db.prepare("INSERT INTO sample (id, val) VALUES (?, ?)");
    insert.bind_int(1, 42);
    insert.bind_text(2, "hello");
    insert.execute();

    auto query = db.prepare("SELECT id, val FROM sample WHERE id = ?");
    query.bind_int(1, 42);
    ASSERT_TRUE(query.step());
    EXPECT_EQ(query.column_int(0), 42);
    EXPECT_EQ(query.column_text(1), "hello");
    EXPECT_FALSE(query.step());
}

TEST(DatabaseTest, PrepareAndExecuteInvalidSqlThrows) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    EXPECT_THROW((void)db.prepare("SELECT missing FROM no_table"), DatabaseError);
    EXPECT_THROW(db.execute("INSERT INTO no_table VALUES (1)"), DatabaseError);
}

TEST(DatabaseTest, ForeignKeysAreEnabledByDefault) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)");
    db.execute("CREATE TABLE child (parent_id INTEGER REFERENCES parent(id))");

    EXPECT_THROW(db.execute("INSERT INTO child (parent_id) VALUES (99)"), DatabaseError);
}

TEST(DatabaseTest, StatementResetAllowsRebinding) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT NOT NULL)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, 'a'), (2, 'b')");

    auto stmt = db.prepare("SELECT val FROM sample WHERE id = ?");

    stmt.bind_int(1, 1);
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_text(0), "a");

    stmt.reset();
    stmt.bind_int(1, 2);
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_text(0), "b");
}

} // namespace cweman
