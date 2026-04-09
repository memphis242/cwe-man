#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sqlite3.h>

namespace cweman {

class DatabaseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// RAII wrapper around sqlite3_stmt*.
class Statement {
public:
    Statement(sqlite3* db, std::string_view sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    void bind_int(int index, int value);
    void bind_int64(int index, int64_t value);
    void bind_text(int index, std::string_view value);

    bool step();   // returns true if a row is available (SQLITE_ROW)
    void execute();// step() once, expecting SQLITE_DONE

    int         column_int(int index) const;
    int64_t     column_int64(int index) const;
    std::string column_text(int index) const;

    void reset();

private:
    sqlite3_stmt* stmt_{nullptr};
};

// RAII wrapper around sqlite3*.
class Database {
public:
    explicit Database(const std::filesystem::path& db_path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Statement prepare(std::string_view sql);
    void execute(std::string_view sql);

    sqlite3* handle() { return db_; }

private:
    sqlite3* db_{nullptr};
};

} // namespace cweman
