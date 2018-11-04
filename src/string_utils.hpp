#ifndef APP_STRING_UTILS_HPP
#define APP_STRING_UTILS_HPP

#include <string>
#include <sstream>
#include <algorithm>

inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}


inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}


inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}


inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}


inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

inline void split(std::vector<std::string> &tokens, const std::string &src, char delim) {
    std::string token ;

    std::istringstream token_stream(src);
    while (std::getline(token_stream, token, delim))
        tokens.push_back(token);
}

inline bool starts_with(const std::string &str, const char *prefix) {
    return str.find(prefix) != std::string::npos ;
}
#endif
