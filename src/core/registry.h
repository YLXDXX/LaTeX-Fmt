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
            // --- Text formatting commands ---
            registerCommand("textbf", {1, 0, true});
            registerCommand("textit", {1, 0, true});
            registerCommand("emph",   {1, 0, true});
            registerCommand("texttt", {1, 0, true});
            registerCommand("textsf", {1, 0, true});
            registerCommand("textrm", {1, 0, true});
            registerCommand("textsc", {1, 0, true});
            registerCommand("textup", {1, 0, true});
            registerCommand("textnormal", {1, 0, true});

            // --- Math commands ---
            registerCommand("frac", {2, 0, true});
            registerCommand("sqrt", {1, 1, true});
            registerCommand("frac12", {0, 0, false});

            // --- Document structure ---
            registerCommand("section",       {1, 0, true});
            registerCommand("subsection",    {1, 0, true});
            registerCommand("subsubsection", {1, 0, true});
            registerCommand("paragraph",     {1, 0, true});
            registerCommand("subparagraph",  {1, 0, true});
            registerCommand("title",   {1, 0, true});
            registerCommand("author",  {1, 0, true});
            registerCommand("date",    {1, 0, true});

            // --- Cross-references and citations ---
            registerCommand("label",  {1, 0, true});
            registerCommand("ref",    {1, 0, true});
            registerCommand("pageref",{1, 0, true});
            registerCommand("eqref",  {1, 0, true});
            registerCommand("cite",   {1, 1, false});

            // --- Package inclusion ---
            registerCommand("usepackage",    {1, 1, false});
            registerCommand("documentclass", {1, 1, false});

            // --- No-argument commands ---
            for (const auto& cmd : {"maketitle", "tableofcontents", "newpage", "clearpage",
                                    "centering", "raggedright", "raggedleft",
                                    "noindent", "indent", "newline", "linebreak", "pagebreak",
                                    "hfill", "vfill", "hrulefill", "dotfill",
                                    "smallskip", "medskip", "bigskip", "par", "today",
                                    "appendix", "baselineskip"}) {
                registerCommand(cmd, {0, 0, false});
            }

            // --- List command ---
            registerCommand("item", {0, 1, false});

            // --- Graphics ---
            registerCommand("includegraphics", {1, 1, false});

            // --- Float commands ---
            registerCommand("caption", {1, 0, true});

            // --- Math alignment environments ---
            registerEnv("align",     {AlignStrategy::AlignmentPair, false, true});
            registerEnv("align*",    {AlignStrategy::AlignmentPair, false, true});
            registerEnv("gather",    {AlignStrategy::None, false, true});
            registerEnv("gather*",   {AlignStrategy::None, false, true});
            registerEnv("equation",  {AlignStrategy::None, false, true});
            registerEnv("equation*", {AlignStrategy::None, false, true});
            registerEnv("pmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("bmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("vmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("cases",     {AlignStrategy::Cases, false, true});

            // --- List environments ---
            registerEnv("itemize",    {AlignStrategy::None, false, false});
            registerEnv("enumerate",  {AlignStrategy::None, false, false});
            registerEnv("description",{AlignStrategy::None, false, false});

            // --- Verbatim environments ---
            registerEnv("verbatim",   {AlignStrategy::None, true, false});
            registerEnv("lstlisting", {AlignStrategy::None, true, false});
            registerEnv("minted",     {AlignStrategy::None, true, false});

            // --- Table environments ---
            registerEnv("tabular",  {AlignStrategy::None, false, false});
            registerEnv("tabularx", {AlignStrategy::None, false, false});

            // --- Other common environments ---
            registerEnv("figure",       {AlignStrategy::None, false, false});
            registerEnv("figure*",      {AlignStrategy::None, false, false});
            registerEnv("table",        {AlignStrategy::None, false, false});
            registerEnv("table*",       {AlignStrategy::None, false, false});
            registerEnv("center",       {AlignStrategy::None, false, false});
            registerEnv("flushleft",    {AlignStrategy::None, false, false});
            registerEnv("flushright",   {AlignStrategy::None, false, false});
            registerEnv("quote",        {AlignStrategy::None, false, false});
            registerEnv("quotation",    {AlignStrategy::None, false, false});
            registerEnv("tikzpicture",  {AlignStrategy::None, false, false});
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
