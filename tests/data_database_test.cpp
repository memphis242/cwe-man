#include "TestUtils.hpp"
#include "data/Database.hpp"

#include <gtest/gtest.h>

namespace cweman {

TEST(DatabaseTest, OpenExecuteAndQueryRoundTrip) {
    auto temp_dir = test::make_temp_dir("cwe-man-db-test");
    auto db_path = temp_dir / "test.db";

    Database db(db_path);
    db.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, val TEXT NOT NULL)");

    auto insert = db.prepare("INSERT INTO sample (id, val) VALUES (?, ?)");
    insert.bind_int(1, 7);
    insert.bind_text(2, "hello");
    insert.execute();

    auto query = db.prepare("SELECT id, val FROM sample WHERE id = ?");
    query.bind_int(1, 7);
    ASSERT_TRUE(query.step());
    EXPECT_EQ(query.column_int(0), 7);
    EXPECT_EQ(query.column_text(1), "hello");
    EXPECT_FALSE(query.step());
}

TEST(DatabaseTest, InvalidSqlThrowsDatabaseError) {
    auto temp_dir = test::make_temp_dir("cwe-man-db-invalid");
    Database db(temp_dir / "test.db");

    EXPECT_THROW((void)db.prepare("SELECT * FROM does_not_exist"), DatabaseError);
    EXPECT_THROW(db.execute("INSERT INTO no_table VALUES (1)"), DatabaseError);
}

TEST(DatabaseTest, ForeignKeysAreEnabled) {
    auto temp_dir = test::make_temp_dir("cwe-man-db-fk");
    Database db(temp_dir / "test.db");

    db.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)");
    db.execute("CREATE TABLE child (pid INTEGER REFERENCES parent(id))");

    EXPECT_THROW(db.execute("INSERT INTO child (pid) VALUES (42)"), DatabaseError);
}

} // namespace cweman
