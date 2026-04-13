#include "data/Database.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
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

TEST(DatabaseTest, BindInt64AndColumnInt64RoundTrip) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, big INTEGER NOT NULL)");
    auto insert = db.prepare("INSERT INTO sample (id, big) VALUES (?, ?)");
    insert.bind_int(1, 1);
    insert.bind_int64(2, static_cast<int64_t>(9223372036854775000LL));
    insert.execute();

    auto query = db.prepare("SELECT big FROM sample WHERE id = 1");
    ASSERT_TRUE(query.step());
    EXPECT_EQ(query.column_int64(0), static_cast<int64_t>(9223372036854775000LL));
}

TEST(DatabaseTest, ColumnTextReturnsEmptyOnNullValue) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, NULL)");

    auto query = db.prepare("SELECT val FROM sample WHERE id = 1");
    ASSERT_TRUE(query.step());
    EXPECT_EQ(query.column_text(0), "");
}

TEST(DatabaseTest, StatementExecuteAllowsRowResultForSelect) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, 'x')");

    auto query = db.prepare("SELECT val FROM sample WHERE id = 1");
    EXPECT_NO_THROW(query.execute());  // SQLITE_ROW path is accepted.
}

TEST(DatabaseTest, StepAndExecuteThrowAfterSchemaChange) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, 'x')");

    auto select_stmt = db.prepare("SELECT val FROM sample");
    auto insert_stmt = db.prepare("INSERT INTO sample (id, val) VALUES (?, ?)");
    insert_stmt.bind_int(1, 2);
    insert_stmt.bind_text(2, "y");

    db.execute("DROP TABLE sample");

    EXPECT_THROW((void)select_stmt.step(), DatabaseError);
    EXPECT_THROW(insert_stmt.execute(), DatabaseError);
}

TEST(DatabaseTest, StatementMoveConstructorAndAssignmentAreSafe) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, 'a'), (2, 'b')");

    auto first = db.prepare("SELECT val FROM sample WHERE id = 1");
    Statement moved(std::move(first));  // move ctor path
    ASSERT_TRUE(moved.step());
    EXPECT_EQ(moved.column_text(0), "a");

    auto second = db.prepare("SELECT val FROM sample WHERE id = 2");
    second = std::move(moved);  // move assignment path (non-self)
    second.reset();
    ASSERT_TRUE(second.step());
    EXPECT_EQ(second.column_text(0), "a");

    auto as_rvalue = [](Statement& s) -> Statement&& { return std::move(s); };
    second = as_rvalue(second);  // self-move assignment branch
    second.reset();
    ASSERT_TRUE(second.step());
    EXPECT_EQ(second.column_text(0), "a");
}

TEST(DatabaseTest, MoveAssignmentAlsoHandlesNullDestinationStatement) {
    auto temp_dir = make_temp_dir();
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT)");
    db.execute("INSERT INTO sample (id, val) VALUES (1, 'a'), (2, 'b')");

    Statement source = db.prepare("SELECT val FROM sample WHERE id = 1");
    Statement destination = db.prepare("SELECT val FROM sample WHERE id = 2");
    Statement moved(std::move(destination));  // destination now nullptr

    destination = std::move(source);  // this != &other and stmt_ == nullptr path
    ASSERT_TRUE(destination.step());
    EXPECT_EQ(destination.column_text(0), "a");
}

TEST(DatabaseTest, ConstructorThrowsOnUnusablePath) {
    auto temp_dir = make_temp_dir();

    auto dir_as_db = temp_dir / "dbdir";
    std::filesystem::create_directories(dir_as_db);
    EXPECT_THROW((void)Database(dir_as_db), DatabaseError);

    auto parent_file = temp_dir / "not_a_dir";
    {
        std::ofstream f(parent_file);
        f << "x";
    }
    EXPECT_THROW((void)Database(parent_file / "db.sqlite"), std::filesystem::filesystem_error);
}

} // namespace cweman
