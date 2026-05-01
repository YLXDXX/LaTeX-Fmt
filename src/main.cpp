#include <iostream>
#include <string>
#include <sstream>
#include "core/registry.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "format/visitor.h"

int main() {
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

    latex_fmt::FormatVisitor visitor(registry, input);
    visitor.visit(*ast);

    std::cout << visitor.extractOutput();
    return 0;
}
