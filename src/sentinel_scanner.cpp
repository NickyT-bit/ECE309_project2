#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(sentinel), pending_("") {
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    std::string combined = pending_;
    combined += chunk;

    std::size_t found = combined.find(sentinel_);

    if (found != std::string::npos) {
        std::string safe = combined.substr(0, found);

        pending_.clear();

        return {safe, true};
    }

    if (combined.size() <= sentinel_.size() - 1) {
        pending_ = combined;
        return {"", false};
    }

    std::size_t safe_length = combined.size() - (sentinel_.size() - 1);

    std::string safe = combined.substr(0, safe_length);

    pending_ = combined.substr(safe_length);

    return {safe, false};
}

SentinelScanner::Out SentinelScanner::flush() {
    std::string safe = pending_;

    pending_.clear();

    return {safe, false};
}