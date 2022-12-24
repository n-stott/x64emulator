#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <string_view>
#include <vector>

inline std::string_view strip(std::string_view sv) {
    const char* skippedCharacters = " \n\t";
    size_t first = sv.find_first_not_of(skippedCharacters);
    size_t last = sv.find_last_not_of(skippedCharacters);
    if(first != std::string::npos && last != std::string::npos) {
        return sv.substr(first, last+1-first);
    } else {
        return "";
    }
}

inline std::vector<std::string_view> split(std::string_view sv, char separator) {
    std::vector<std::string_view> result;
    size_t pos = 0;
    size_t next = 0;
    while(next != sv.size()) {
        next = sv.find(separator, pos);
        if(next == std::string::npos) next = sv.size();
        std::string_view item = std::string_view(sv.begin()+pos, next-pos);
        result.push_back(strip(item));
        pos = next+1;
    }
    return result;
}

inline std::vector<std::string_view> splitFirst(std::string_view sv, char separator) {
    std::vector<std::string_view> result;
    size_t next = sv.find(separator, 0);
    if(next == std::string::npos) next = sv.size();
    result.push_back(std::string_view(sv.begin(), next));
    if(next != sv.size()) {
        result.push_back(sv.substr(next+1));
    }
    return result;
}

inline std::string filenameFromPath(const std::string& filepath) {
    auto lastSlash = filepath.rfind('/');
    std::string filename = filepath.substr(lastSlash == std::string::npos ? 0 : lastSlash+1);
    auto firstDot = filename.find('.');
    std::string filenameNoExtension = (firstDot == std::string::npos ? filename : filename.substr(0, firstDot));
    return filenameNoExtension;
}

inline std::string removeOverride(std::string_view sv) {
    size_t dsOverride = sv.find("ds:");
    if(dsOverride != std::string_view::npos) {
        return std::string(sv.substr(0, dsOverride)) + std::string(sv.substr(dsOverride+3));
    }
    size_t esOverride = sv.find("es:");
    if(esOverride != std::string_view::npos) {
        return std::string(sv.substr(0, esOverride)) + std::string(sv.substr(esOverride+3));
    }
    return std::string(sv);
}

inline bool startsWith(std::string_view sv, std::string_view prefix) {
    if(sv.size() < prefix.size()) return false;
    return sv.substr(0, prefix.size()) == prefix;
}


#endif