// SPDX-License-Identifier: MIT
//
// `minitool` is the single front-end for the toolchain. Subcommands are added
// as the corresponding subsystems land; today it can describe the frozen ISA
// and decode raw instruction words, which is enough to inspect V1/V2 output.

#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "minitool/common/print.hpp"
#include "minitool/isa/encoding.hpp"
#include "minitool/isa/opcode.hpp"

namespace {

using minitool::io::println;

constexpr std::string_view kVersion = "0.2.0";

int usage() {
    println("minitool {} - toolchain for the MiniToolchain 64-bit virtual ISA", kVersion);
    println();
    println("usage: minitool <command> [args]");
    println();
    println("commands:");
    println("  version              print the tool version");
    println("  isa                  print the frozen opcode table");
    println("  decode <hex-word>    decode one 64-bit instruction word");
    return 2;
}

int cmdIsa() {
    println("{:<6} {:<8} {:<8} {}", "op", "mnemonic", "format", "notes");
    for (const minitool::isa::OpcodeInfo& info : minitool::isa::allOpcodes()) {
        std::string notes;
        if (info.writes_flags) {
            notes += "flags ";
        }
        if (info.is_branch) {
            notes += "branch ";
        }
        if (info.is_terminator) {
            notes += "terminator ";
        }
        println("0x{:02X}   {:<8} {:<8} {}", static_cast<unsigned>(info.opcode), info.mnemonic,
                minitool::isa::formatName(info.format), notes);
    }
    return 0;
}

int cmdDecode(std::string_view text) {
    std::string_view digits = text;
    if (digits.starts_with("0x") || digits.starts_with("0X")) {
        digits.remove_prefix(2);
    }
    minitool::u64 word = 0;
    const char* begin = digits.data();
    const char* end = digits.data() + digits.size();
    const std::from_chars_result result = std::from_chars(begin, end, word, 16);
    if (digits.empty() || result.ec != std::errc{} || result.ptr != end) {
        println(stderr, "error: '{}' is not a 64-bit hexadecimal word", text);
        return 1;
    }
    const auto decoded = minitool::isa::decode(word);
    if (!decoded.has_value()) {
        println(stderr, "error: {}", minitool::isa::decodeErrorName(decoded.error()));
        return 1;
    }
    println("{:016X}  {}", word, minitool::isa::toString(*decoded));
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    if (args.empty()) {
        return usage();
    }
    const std::string_view command = args[0];
    if (command == "version") {
        println("{}", kVersion);
        return 0;
    }
    if (command == "isa") {
        return cmdIsa();
    }
    if (command == "decode") {
        if (args.size() != 2) {
            return usage();
        }
        return cmdDecode(args[1]);
    }
    println(stderr, "error: unknown command '{}'", command);
    return usage();
}
