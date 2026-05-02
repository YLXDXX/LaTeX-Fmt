#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace latex_fmt::utils {

inline bool read_input(const std::string& path, std::string& out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        std::cerr << "latex-fmt: error: cannot read file '" << path << "'\n";
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

inline bool write_output(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "latex-fmt: error: cannot write file '" << path << "'\n";
        return false;
    }
    ofs << content;
    return true;
}

} // namespace latex_fmt::utils
