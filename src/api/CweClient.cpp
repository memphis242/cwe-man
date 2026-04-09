#include "api/CweClient.hpp"
#include "logging/Logger.hpp"

#include <curl/curl.h>
#include <format>
#include <stdexcept>

namespace cweman {

namespace {

struct CurlGlobalInit {
    CurlGlobalInit()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalInit() { curl_global_cleanup(); }
};

// Ensures curl_global_init is called exactly once, before any CURL handle.
CurlGlobalInit& ensure_curl_init() {
    static CurlGlobalInit instance;
    return instance;
}

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* response = static_cast<std::string*>(userdata);
    response->append(ptr, size * nmemb);
    return size * nmemb;
}

} // namespace

CweClient::CweClient() {
    ensure_curl_init();
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize libcurl");
    }
}

CweClient::~CweClient() {
    if (curl_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
    }
}

std::string CweClient::get_version() {
    return get("/cwe/version");
}

std::string CweClient::get_view(int view_id) {
    return get(std::format("/cwe/view/{}", view_id));
}

std::string CweClient::get_category(int category_id) {
    return get(std::format("/cwe/category/{}", category_id));
}

std::string CweClient::get_weaknesses(const std::string& comma_ids) {
    return get(std::format("/cwe/weakness/{}", comma_ids));
}

std::string CweClient::get(const std::string& path) {
    auto* curl = static_cast<CURL*>(curl_);
    std::string url = std::string{BASE_URL} + path;
    std::string response;

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "cwe-man/0.1");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    Logger::instance().debug(std::format("GET {}", url));

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // The CWE API server drops the connection after sending data, causing
    // CURLE_RECV_ERROR (56) despite a complete 200 response. Tolerate this
    // if we got a 200 and received data.
    if (res != CURLE_OK) {
        if (res == CURLE_RECV_ERROR && http_code == 200 && !response.empty()) {
            Logger::instance().warn(std::format(
                "Server closed connection early on {} (data received OK)", path));
        } else {
            std::string err = std::format("HTTP request failed: {} ({})",
                                          curl_easy_strerror(res), url);
            Logger::instance().error(err);
            throw std::runtime_error(err);
        }
    }

    if (http_code != 200) {
        std::string err = std::format("HTTP {} from {}", http_code, url);
        Logger::instance().error(err);
        throw std::runtime_error(err);
    }

    Logger::instance().debug(
        std::format("Response from {} ({} bytes)", path, response.size()));
    Logger::instance().debug(response);

    return response;
}

} // namespace cweman
