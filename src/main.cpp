#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include "core/registry.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"

static const char* LATEX_FMT_VERSION = "1.0.0";

static void print_help(const char* prog) {
    std::cerr
        << "latex-fmt — LaTeX code formatter (v" << LATEX_FMT_VERSION << ")\n"
        << "\n"
        << "Usage:\n"
        << "  " << prog << " [options] [file.tex]\n"
        << "  " << prog << " [options] < input.tex > output.tex\n"
        << "\n"
        << "Options:\n"
        << "  --help, -h          Show this help message and exit\n"
        << "  --version, -V       Show version and exit\n"
        << "  -i                  Format file in-place\n"
        << "  -o <file>           Write output to file\n"
        << "  --max-line-width=N  Warn when a line exceeds N characters (default: 0 = off)\n"
        << "\n"
        << "Examples:\n"
        << "  " << prog << " paper.tex                  Format to stdout\n"
        << "  " << prog << " -i paper.tex                Format file in-place\n"
        << "  " << prog << " -o out.tex paper.tex        Format to output file\n"
        << "  " << prog << " < input.tex > output.tex    Pipe from stdin\n";
}

static void print_version() {
    std::cout << "latex-fmt v" << LATEX_FMT_VERSION << "\n";
}

static bool read_input(const std::string& path, std::string& out) {
    std::ifstream ifs(path);
    if (!ifs) {
        std::cerr << "latex-fmt: error: cannot read file '" << path << "'\n";
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

static bool write_output(const std::string& path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs) {
        std::cerr << "latex-fmt: error: cannot write file '" << path << "'\n";
        return false;
    }
    ofs << content;
    return true;
}

int main(int argc, char* argv[]) {
    int max_line_width = 0;
    bool in_place = false;
    std::string output_path;
    std::string input_path;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "--version" || arg == "-V") {
            print_version();
            return 0;
        }
        if (arg == "-i") {
            in_place = true;
            continue;
        }
        if (arg == "-o") {
            if (i + 1 < argc) {
                output_path = argv[++i];
            } else {
                std::cerr << "latex-fmt: error: -o requires a file argument\n";
                return 1;
            }
            continue;
        }
        if (arg.compare(0, 17, "--max-line-width=") == 0) {
            max_line_width = std::stoi(std::string(arg.substr(17)));
            continue;
        }
        if (arg.size() > 0 && arg[0] == '-') {
            std::cerr << "latex-fmt: error: unknown option '" << arg << "'\n";
            print_help(argv[0]);
            return 1;
        }
        if (input_path.empty()) {
            input_path = arg;
        } else {
            std::cerr << "latex-fmt: error: multiple input files not supported\n";
            return 1;
        }
    }

    if (in_place && !output_path.empty()) {
        std::cerr << "latex-fmt: error: -i and -o cannot be used together\n";
        return 1;
    }

    std::string input;
    if (!input_path.empty()) {
        if (!read_input(input_path, input)) return 1;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }

    if (input.empty()) return 0;

    latex_fmt::Registry registry;
    registry.registerBuiltin();

    latex_fmt::Lexer lexer(input, registry);
    auto tokens = lexer.tokenize();

    latex_fmt::Parser parser(std::move(tokens), input, registry);
    auto ast = parser.parse();

    latex_fmt::FormatVisitor visitor(registry, input, max_line_width);
    visitor.visit(*ast);

    std::string result = visitor.extractOutput();

    if (in_place) {
        if (!write_output(input_path, result)) return 1;
    } else if (!output_path.empty()) {
        if (!write_output(output_path, result)) return 1;
    } else {
        std::cout << result;
    }

    for (const auto& w : visitor.getWarnings()) {
        std::cerr << "WARNING: " << w << "\n";
    }

    return 0;
}
