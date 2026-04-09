#pragma once

#include <string>
#include <string_view>

namespace cweman {

class CweClient {
public:
    static constexpr std::string_view BASE_URL =
        "https://cwe-api.mitre.org/api/v1";

    CweClient();
    ~CweClient();

    CweClient(const CweClient&) = delete;
    CweClient& operator=(const CweClient&) = delete;

    // GET /cwe/version
    std::string get_version();

    // GET /cwe/view/{id}  — returns view metadata including Members list
    std::string get_view(int view_id);

    // GET /cwe/category/{id}  — returns category with Relationships
    std::string get_category(int category_id);

    // GET /cwe/weakness/{ids}  — comma-separated IDs for batch fetch
    std::string get_weaknesses(const std::string& comma_ids);

private:
    std::string get(const std::string& path);
    void* curl_{nullptr};  // CURL* — forward-declared to avoid leaking curl.h
};

} // namespace cweman
