#include "internal.h"
#include <curl/curl.h>
#include <string>
#include <cstring>
#include <algorithm>

namespace Metriqos::Internal {

static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* resp = static_cast<HttpResponse*>(userdata);
    std::string header(buffer, size * nitems);

    // Case-insensitive header parsing
    std::string lower = header;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto extractValue = [&](const std::string& prefix) -> std::string {
        if (lower.find(prefix) != 0) return {};
        auto val = header.substr(prefix.size());
        // Trim whitespace and trailing \r\n
        size_t start = val.find_first_not_of(" \t");
        if (start == std::string::npos) return {};
        size_t end = val.find_last_not_of(" \t\r\n");
        return val.substr(start, end - start + 1);
    };

    auto ra = extractValue("retry-after:");
    if (!ra.empty()) resp->retryAfter = ra;

    auto qr = extractValue("x-quota-remaining:");
    if (!qr.empty()) resp->quotaRemaining = qr;

    return size * nitems;
}

HttpResponse httpPost(const std::string& url, const std::string& body,
                      const std::string& apiKey, const std::string& userAgent) {
    HttpResponse resp;

    CURL* curl = curl_easy_init();
    if (!curl) {
        log(LogLevel::Error, "Failed to initialize CURL handle");
        return resp;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-API-Key: " + apiKey).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("User-Agent: " + userAgent).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log(LogLevel::Error, std::string("HTTP request failed: ") + curl_easy_strerror(res));
    } else {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        resp.statusCode = static_cast<int>(code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return resp;
}

} // namespace Metriqos::Internal
