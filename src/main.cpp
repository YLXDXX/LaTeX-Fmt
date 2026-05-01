#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include "core/registry.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"

int main(int argc, char* argv[]) {
    int max_line_width = 0;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg.compare(0, 17, "--max-line-width=") == 0) {
            max_line_width = std::stoi(std::string(arg.substr(17)));
        }
    }

    std::ostringstream ss;
    ss << std::cin.rdbuf();
    std::string input = ss.str();

    if (input.empty()) return 0;

    latex_fmt::Registry registry;
    registry.registerBuiltin();

    latex_fmt::Lexer lexer(input, registry);
    auto tokens = lexer.tokenize();

    latex_fmt::Parser parser(std::move(tokens), input, registry);
    auto ast = parser.parse();

    latex_fmt::FormatVisitor visitor(registry, input, max_line_width);
    visitor.visit(*ast);

    std::cout << visitor.extractOutput();

    for (const auto& w : visitor.getWarnings()) {
        std::cerr << "WARNING: " << w << "\n";
    }

    return 0;
}
