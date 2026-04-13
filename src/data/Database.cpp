#include "data/Database.hpp"
#include "logging/Logger.hpp"

#include <format>

namespace cweman {

// ── Statement ───────────────────────────────────────────────────────────────

Statement::Statement(sqlite3* db, std::string_view sql) {
    int rc = sqlite3_prepare_v2(db, sql.data(), static_cast<int>(sql.size()),
                                &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        throw DatabaseError{
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(db))};
    }
}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

Statement::Statement(Statement&& other) noexcept : stmt_{other.stmt_} {
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) sqlite3_finalize(stmt_);
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::bind_int(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
}

void Statement::bind_int64(int index, int64_t value) {
    sqlite3_bind_int64(stmt_, index, value);
}

void Statement::bind_text(int index, std::string_view value) {
    sqlite3_bind_text(stmt_, index, value.data(),
                      static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW)  return true;
    if (rc == SQLITE_DONE) return false;  // GCOVR_EXCL_BR_LINE
    throw DatabaseError{
        std::format("Step failed: {}", sqlite3_errmsg(sqlite3_db_handle(stmt_)))};  // GCOVR_EXCL_BR_LINE
}

void Statement::execute() {
    int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {  // GCOVR_EXCL_BR_LINE
        throw DatabaseError{
            std::format("Execute failed: {}",
                        sqlite3_errmsg(sqlite3_db_handle(stmt_)))};  // GCOVR_EXCL_BR_LINE
    }
}

int Statement::column_int(int index) const {
    return sqlite3_column_int(stmt_, index);
}

int64_t Statement::column_int64(int index) const {
    return sqlite3_column_int64(stmt_, index);
}

std::string Statement::column_text(int index) const {
    const auto* text = sqlite3_column_text(stmt_, index);
    if (!text) return {};
    return reinterpret_cast<const char*>(text);
}

void Statement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

// ── Database ────────────────────────────────────────────────────────────────

Database::Database(const std::filesystem::path& db_path) {
    std::filesystem::create_directories(db_path.parent_path());
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);  // GCOVR_EXCL_BR_LINE
        sqlite3_close(db_);  // GCOVR_EXCL_BR_LINE
        db_ = nullptr;
        throw DatabaseError{std::format("Cannot open database: {}", err)};  // GCOVR_EXCL_BR_LINE
    }
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA foreign_keys=ON");
    Logger::instance().info(  // GCOVR_EXCL_BR_LINE
        std::format("Opened database at {}", db_path.string()));  // GCOVR_EXCL_BR_LINE
}

Database::~Database() {
    if (db_) {  // GCOVR_EXCL_BR_LINE
        sqlite3_close(db_);
    }
}

Statement Database::prepare(std::string_view sql) {
    return Statement{db_, sql};
}

void Database::execute(std::string_view sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, std::string{sql}.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";  // GCOVR_EXCL_BR_LINE
        sqlite3_free(err);
        throw DatabaseError{std::format("Execute failed: {}", msg)};
    }
}

} // namespace cweman
