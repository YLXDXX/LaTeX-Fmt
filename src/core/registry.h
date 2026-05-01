#pragma once
#include <string>
#include <unordered_map>

namespace latex_fmt {

    struct CommandSignature {
        int mandatory_args = 0;     // 必选参数个数 {}
        int optional_args = 0;     // 可选参数个数 []
        bool mandatory_braces = true; // 是否强制补全花括号
    };

    enum class AlignStrategy { None, AlignmentPair, Matrix, Cases };

    struct EnvRule {
        AlignStrategy align_strategy = AlignStrategy::None;
        bool is_verbatim = false;
        bool is_math = false;
    };

    class Registry {
    public:
        void registerBuiltin() {
            // Common commands
            registerCommand("frac", {2, 0, true});
            registerCommand("sqrt", {1, 1, true});
            registerCommand("frac12", {0, 0, false}); // Hack to avoid matching \frac if specifically written
            registerCommand("textbf", {1, 0, true});
            registerCommand("textit", {1, 0, true});
            registerCommand("emph", {1, 0, true});
            registerCommand("section", {1, 0, true});
            registerCommand("subsection", {1, 0, true});
            registerCommand("label", {1, 0, true});
            registerCommand("ref", {1, 0, true});
            registerCommand("cite", {1, 0, true});
            registerCommand("usepackage", {1, 1, false});

            // Math alignment environments
            registerEnv("align", {AlignStrategy::AlignmentPair, false, true});
            registerEnv("align*", {AlignStrategy::AlignmentPair, false, true});
            registerEnv("equation", {AlignStrategy::None, false, true});
            registerEnv("equation*", {AlignStrategy::None, false, true});
            registerEnv("pmatrix", {AlignStrategy::Matrix, false, true});
            registerEnv("bmatrix", {AlignStrategy::Matrix, false, true});
            registerEnv("vmatrix", {AlignStrategy::Matrix, false, true});
            registerEnv("cases", {AlignStrategy::Cases, false, true});

            // Verbatim environments
            registerEnv("verbatim", {AlignStrategy::None, true, false});
            registerEnv("lstlisting", {AlignStrategy::None, true, false});
            registerEnv("minted", {AlignStrategy::None, true, false});
        }

        void registerCommand(const std::string& name, CommandSignature sig) {
            commands_[name] = std::move(sig);
        }

        void registerEnv(const std::string& name, EnvRule rule) {
            envs_[name] = std::move(rule);
        }

        const CommandSignature* lookupCmd(const std::string& name) const {
            auto it = commands_.find(name);
            if (it != commands_.end()) return &(it->second);
            return nullptr;
        }

        const EnvRule* lookupEnv(const std::string& name) const {
            auto it = envs_.find(name);
            if (it != envs_.end()) return &(it->second);
            return nullptr;
        }

    private:
        std::unordered_map<std::string, CommandSignature> commands_;
        std::unordered_map<std::string, EnvRule> envs_;
    };

} // namespace latex_fmt
