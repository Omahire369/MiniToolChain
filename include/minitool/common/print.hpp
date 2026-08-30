// SPDX-License-Identifier: MIT
#pragma once

#include <cstdio>
#include <format>
#include <string>
#include <string_view>
#include <utility>

/// Thin wrapper over std::format for line-oriented output. <print> is C++23 but
/// is not available in every supported standard library (GCC 13 lacks it), so
/// the toolchain routes all console output through here instead.
namespace minitool::io {

inline void writeLine(std::FILE* out, std::string_view text) {
    std::fwrite(text.data(), 1, text.size(), out);
    std::fputc('\n', out);
}

template <typename... Args>
void println(std::FILE* out, std::format_string<Args...> fmt, Args&&... args) {
    writeLine(out, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
    println(stdout, fmt, std::forward<Args>(args)...);
}

inline void println() {
    std::fputc('\n', stdout);
}

}  // namespace minitool::io
