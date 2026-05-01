#include <catch2/catch_test_macros.hpp>
#include "core/registry.h"

TEST_CASE("Registry: built-in commands", "[registry]") {
    using namespace latex_fmt;

    Registry reg;
    reg.registerBuiltin();

    SECTION("frac has 2 mandatory args, brace completion on") {
        auto* frac = reg.lookupCmd("frac");
        REQUIRE(frac != nullptr);
        REQUIRE(frac->mandatory_args == 2);
        REQUIRE(frac->optional_args == 0);
        REQUIRE(frac->mandatory_braces == true);
    }

    SECTION("sqrt has 1 optional + 1 mandatory arg") {
        auto* sqrt = reg.lookupCmd("sqrt");
        REQUIRE(sqrt != nullptr);
        REQUIRE(sqrt->optional_args == 1);
        REQUIRE(sqrt->mandatory_args == 1);
        REQUIRE(sqrt->mandatory_braces == true);
    }

    SECTION("textbf has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("textbf");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
        REQUIRE(cmd->optional_args == 0);
        REQUIRE(cmd->mandatory_braces == true);
    }

    SECTION("textit has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("textit");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
        REQUIRE(cmd->mandatory_braces == true);
    }

    SECTION("emph has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("emph");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
    }

    SECTION("section has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("section");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
        REQUIRE(cmd->mandatory_braces == true);
    }

    SECTION("subsection has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("subsection");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
    }

    SECTION("label has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("label");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
    }

    SECTION("ref has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("ref");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
    }

    SECTION("cite has 1 mandatory arg") {
        auto* cmd = reg.lookupCmd("cite");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
    }

    SECTION("usepackage has 1 mandatory + 1 optional, no brace completion") {
        auto* cmd = reg.lookupCmd("usepackage");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 1);
        REQUIRE(cmd->optional_args == 1);
        REQUIRE(cmd->mandatory_braces == false);
    }

    SECTION("frac12 has 0 args, brace completion off") {
        auto* cmd = reg.lookupCmd("frac12");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 0);
        REQUIRE(cmd->optional_args == 0);
        REQUIRE(cmd->mandatory_braces == false);
    }
}

TEST_CASE("Registry: built-in environments", "[registry]") {
    using namespace latex_fmt;

    Registry reg;
    reg.registerBuiltin();

    SECTION("align is math with AlignmentPair") {
        auto* env = reg.lookupEnv("align");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->is_verbatim == false);
        REQUIRE(env->align_strategy == AlignStrategy::AlignmentPair);
    }

    SECTION("align* is math with AlignmentPair") {
        auto* env = reg.lookupEnv("align*");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->align_strategy == AlignStrategy::AlignmentPair);
    }

    SECTION("equation is math with no alignment") {
        auto* env = reg.lookupEnv("equation");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->is_verbatim == false);
        REQUIRE(env->align_strategy == AlignStrategy::None);
    }

    SECTION("equation* is math with no alignment") {
        auto* env = reg.lookupEnv("equation*");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->align_strategy == AlignStrategy::None);
    }

    SECTION("pmatrix is math with Matrix alignment") {
        auto* env = reg.lookupEnv("pmatrix");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->align_strategy == AlignStrategy::Matrix);
    }

    SECTION("bmatrix is math with Matrix alignment") {
        auto* env = reg.lookupEnv("bmatrix");
        REQUIRE(env != nullptr);
        REQUIRE(env->align_strategy == AlignStrategy::Matrix);
    }

    SECTION("vmatrix is math with Matrix alignment") {
        auto* env = reg.lookupEnv("vmatrix");
        REQUIRE(env != nullptr);
        REQUIRE(env->align_strategy == AlignStrategy::Matrix);
    }

    SECTION("cases is math with Cases alignment") {
        auto* env = reg.lookupEnv("cases");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_math == true);
        REQUIRE(env->align_strategy == AlignStrategy::Cases);
    }

    SECTION("verbatim is verbatim, not math") {
        auto* env = reg.lookupEnv("verbatim");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_verbatim == true);
        REQUIRE(env->is_math == false);
        REQUIRE(env->align_strategy == AlignStrategy::None);
    }

    SECTION("lstlisting is verbatim, not math") {
        auto* env = reg.lookupEnv("lstlisting");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_verbatim == true);
        REQUIRE(env->is_math == false);
    }

    SECTION("minted is verbatim, not math") {
        auto* env = reg.lookupEnv("minted");
        REQUIRE(env != nullptr);
        REQUIRE(env->is_verbatim == true);
    }
}

TEST_CASE("Registry: unknown lookups return nullptr", "[registry]") {
    using namespace latex_fmt;

    Registry reg;
    reg.registerBuiltin();

    SECTION("Unknown command") {
        REQUIRE(reg.lookupCmd("mycustomcmd") == nullptr);
        REQUIRE(reg.lookupCmd("foo") == nullptr);
        REQUIRE(reg.lookupCmd("") == nullptr);
    }

    SECTION("Unknown environment") {
        REQUIRE(reg.lookupEnv("myenv") == nullptr);
        REQUIRE(reg.lookupEnv("customblock") == nullptr);
        REQUIRE(reg.lookupEnv("") == nullptr);
    }
}

TEST_CASE("Registry: custom registration", "[registry]") {
    using namespace latex_fmt;

    Registry reg;

    SECTION("Register and lookup custom command") {
        reg.registerCommand("mycmd", {2, 1, true});
        auto* cmd = reg.lookupCmd("mycmd");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 2);
        REQUIRE(cmd->optional_args == 1);
        REQUIRE(cmd->mandatory_braces == true);
    }

    SECTION("Register and lookup custom environment") {
        reg.registerEnv("myenv", {AlignStrategy::AlignmentPair, false, true});
        auto* env = reg.lookupEnv("myenv");
        REQUIRE(env != nullptr);
        REQUIRE(env->align_strategy == AlignStrategy::AlignmentPair);
        REQUIRE(env->is_verbatim == false);
        REQUIRE(env->is_math == true);
    }

    SECTION("Override existing command") {
        reg.registerBuiltin();
        // Override frac with different signature
        reg.registerCommand("frac", {3, 0, false});
        auto* cmd = reg.lookupCmd("frac");
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->mandatory_args == 3);
        REQUIRE(cmd->mandatory_braces == false);
    }

    SECTION("Override existing environment") {
        reg.registerBuiltin();
        reg.registerEnv("align", {AlignStrategy::None, false, false});
        auto* env = reg.lookupEnv("align");
        REQUIRE(env != nullptr);
        REQUIRE(env->align_strategy == AlignStrategy::None);
        REQUIRE(env->is_math == false);
    }

    SECTION("Multiple custom registrations") {
        reg.registerCommand("cmd1", {1, 0, true});
        reg.registerCommand("cmd2", {0, 1, false});
        reg.registerCommand("cmd3", {2, 2, true});

        REQUIRE(reg.lookupCmd("cmd1")->mandatory_args == 1);
        REQUIRE(reg.lookupCmd("cmd2")->optional_args == 1);
        REQUIRE(reg.lookupCmd("cmd3")->mandatory_args == 2);
        REQUIRE(reg.lookupCmd("cmd3")->optional_args == 2);
    }
}

TEST_CASE("Registry: default values in structs", "[registry]") {
    using namespace latex_fmt;

    SECTION("CommandSignature defaults") {
        CommandSignature sig;
        REQUIRE(sig.mandatory_args == 0);
        REQUIRE(sig.optional_args == 0);
        REQUIRE(sig.mandatory_braces == true);
    }

    SECTION("EnvRule defaults") {
        EnvRule rule;
        REQUIRE(rule.align_strategy == AlignStrategy::None);
        REQUIRE(rule.is_verbatim == false);
        REQUIRE(rule.is_math == false);
    }
}
