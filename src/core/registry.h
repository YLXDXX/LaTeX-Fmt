#pragma once
#include <string>
#include <unordered_map>

namespace latex_fmt {

    struct CommandSignature {
        int mandatory_args = 0;     // 必选参数个数 {}
        int optional_args = 0;     // 可选参数个数 []
        bool mandatory_braces = true; // 是否强制补全花括号
    };

    enum class AlignStrategy { None, AlignmentPair, Matrix, Cases, Tabular };

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
            registerCommand("stackrel",    {2, 0, false});
            registerCommand("overset",     {2, 0, false});
            registerCommand("underset",    {2, 0, false});
            registerCommand("substack",    {1, 0, false});
            registerCommand("binom",       {2, 0, true});
            registerCommand("text",        {1, 0, false});
            registerCommand("operatorname",{1, 0, false});
            registerCommand("tag",         {1, 0, false});
            registerCommand("tag*",        {1, 0, false});
            for (const auto& cmd : {"notag", "nonumber", "limits", "nolimits",
                                    "displaylimits", "displaystyle", "textstyle",
                                    "scriptstyle", "scriptscriptstyle",
                                    "smash", "mathclap", "mathrlap", "mathllap"}) {
                registerCommand(cmd, {0, 0, false});
            }

            // --- Declare / new commands ---
            registerCommand("DeclareMathOperator", {2, 0, false});
            registerCommand("newcommand",      {0, 0, false});
            registerCommand("renewcommand",    {0, 0, false});
            registerCommand("providecommand",  {0, 0, false});
            registerCommand("newenvironment",  {0, 0, false});
            registerCommand("renewenvironment",{0, 0, false});
            registerCommand("ensuremath",      {1, 0, true});

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

            // --- Delimiter sizing commands (delimiter is a separate token) ---
            for (const auto& cmd : {"left", "right",
                                    "bigl", "Bigl", "biggl", "Biggl",
                                    "bigr", "Bigr", "biggr", "Biggr",
                                    "big", "Big", "bigg", "Bigg"}) {
                registerCommand(cmd, {0, 0, false});
            }

            // --- List command ---
            registerCommand("item", {0, 1, false});

            // --- Graphics ---
            registerCommand("includegraphics", {1, 1, false});

            // --- Float commands ---
            registerCommand("caption", {1, 0, true});

            // --- caption / subcaption ---
            registerCommand("captionsetup", {1, 0, false});
            registerCommand("subcaption",   {1, 0, true});
            registerEnv("subfigure", {AlignStrategy::None, false, false});
            registerEnv("subtable",  {AlignStrategy::None, false, false});

            // --- graphicx / svg ---
            registerCommand("graphicspath",              {1, 0, false});
            registerCommand("DeclareGraphicsExtensions", {1, 0, false});
            registerCommand("includesvg", {1, 1, false});

            // --- xcolor ---
            registerCommand("color",       {1, 1, false});
            registerCommand("textcolor",   {1, 1, false});
            registerCommand("colorbox",    {1, 1, false});
            registerCommand("pagecolor",   {1, 0, false});
            registerCommand("definecolor", {3, 0, false});

            // --- bm ---
            registerCommand("bm", {1, 0, true});

            // --- mhchem ---
            registerCommand("ce", {1, 0, false});
            registerCommand("pu", {1, 0, false});

            // --- physics / braket (shared commands) ---
            registerCommand("abs",    {1, 0, true});
            registerCommand("norm",   {1, 0, true});
            registerCommand("qty",    {1, 0, true});
            registerCommand("mqty",   {1, 0, true});
            registerCommand("eval",   {2, 0, true});
            registerCommand("comm",   {2, 0, true});
            registerCommand("order",  {1, 0, true});
            registerCommand("ket",    {1, 0, true});
            registerCommand("bra",    {1, 0, true});
            registerCommand("braket", {2, 0, true});
            registerCommand("set",    {1, 0, true});
            registerCommand("dv",     {2, 0, true});
            registerCommand("pdv",    {2, 0, true});
            registerCommand("dd",     {0, 0, false});

            // --- braket uppercase variants ---
            registerCommand("Bra",    {1, 0, true});
            registerCommand("Ket",    {1, 0, true});

            // -- derivative ---
            registerCommand("odv",       {2, 0, true});
            registerCommand("derivative",{2, 0, true});

            // --- extarrows ---
            registerCommand("xleftrightarrow", {1, 0, false});
            registerCommand("xlongequal",     {1, 0, false});
            registerCommand("xLongrightarrow",{1, 0, false});

            // --- gensymb ---
            for (const auto& cmd : {"degree", "celsius", "micro", "perthousand", "ohm"}) {
                registerCommand(cmd, {0, 0, false});
            }

            // --- algorithm2e ---
            for (const auto& cmd : {"KwIn", "KwOut", "KwData", "KwResult",
                                    "For", "While", "If", "Else", "ElseIf",
                                    "Return", "SetKw", "SetAlgoLined",
                                    "LinesNumbered", "BlankLine"}) {
                registerCommand(cmd, {0, 0, false});
            }
            registerCommand("SetKwInOut", {2, 0, false});
            registerCommand("tcp",        {1, 0, false});
            registerEnv("algorithm",  {AlignStrategy::None, false, false});
            registerEnv("function",   {AlignStrategy::None, false, false});

            // --- enumitem ---
            registerCommand("setlist",         {1, 0, false});
            registerCommand("newlist",         {3, 0, false});
            registerCommand("setenumerate",    {0, 0, false});
            registerCommand("setitemize",      {0, 0, false});
            registerCommand("setdescription",  {0, 0, false});

            // --- lipsum / zhlipsum ---
            registerCommand("lipsum",   {0, 1, false});
            registerCommand("zhlipsum", {0, 1, false});

            // --- tabularray ---
            registerEnv("tblr",     {AlignStrategy::Tabular, false, false});
            registerEnv("longtblr", {AlignStrategy::Tabular, false, false});
            registerEnv("talltblr", {AlignStrategy::Tabular, false, false});

            // --- Math alignment environments ---
            registerEnv("align",     {AlignStrategy::AlignmentPair, false, true});
            registerEnv("align*",    {AlignStrategy::AlignmentPair, false, true});
            registerEnv("aligned",   {AlignStrategy::AlignmentPair, false, true});
            registerEnv("alignedat", {AlignStrategy::AlignmentPair, false, true});
            registerEnv("flalign",   {AlignStrategy::AlignmentPair, false, true});
            registerEnv("flalign*",  {AlignStrategy::AlignmentPair, false, true});
            registerEnv("gather",    {AlignStrategy::None, false, true});
            registerEnv("gather*",   {AlignStrategy::None, false, true});
            registerEnv("gathered",  {AlignStrategy::None, false, true});
            registerEnv("split",     {AlignStrategy::AlignmentPair, false, true});
            registerEnv("equation",  {AlignStrategy::None, false, true});
            registerEnv("equation*", {AlignStrategy::None, false, true});
            registerEnv("multline",  {AlignStrategy::None, false, true});
            registerEnv("multline*", {AlignStrategy::None, false, true});
            registerEnv("pmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("bmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("Bmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("vmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("Vmatrix",   {AlignStrategy::Matrix, false, true});
            registerEnv("matrix",    {AlignStrategy::Matrix, false, true});
            registerEnv("smallmatrix",{AlignStrategy::Matrix, false, true});
            registerEnv("cases",     {AlignStrategy::Cases, false, true});
            registerEnv("dcases",    {AlignStrategy::Cases, false, true});

            // --- List environments ---
            registerEnv("itemize",    {AlignStrategy::None, false, false});
            registerEnv("enumerate",  {AlignStrategy::None, false, false});
            registerEnv("description",{AlignStrategy::None, false, false});

            // --- Verbatim environments ---
            registerEnv("verbatim",   {AlignStrategy::None, true, false});
            registerEnv("lstlisting", {AlignStrategy::None, true, false});
            registerEnv("minted",     {AlignStrategy::None, true, false});

            // --- Table environments ---
            registerEnv("tabular",  {AlignStrategy::Tabular, false, false});
            registerEnv("tabularx", {AlignStrategy::Tabular, false, false});

            // --- Table rule commands ---
            registerCommand("hline",      {0, 1, false});
            registerCommand("toprule",    {0, 0, false});
            registerCommand("midrule",    {0, 0, false});
            registerCommand("bottomrule", {0, 0, false});
            registerCommand("cmidrule",   {0, 0, false});

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

            // --- Article / Book common ---
            registerCommand("chapter",   {1, 1, true});
            registerCommand("chapter*",  {1, 0, true});
            registerCommand("part",      {1, 1, true});
            registerCommand("part*",     {1, 0, true});
            registerCommand("thanks",    {1, 0, true});
            for (const auto& cmd : {"frontmatter", "mainmatter", "backmatter",
                                    "listoffigures", "listoftables",
                                    "pagestyle", "pagenumbering"}) {
                registerCommand(cmd, {0, 0, false});
            }
            registerEnv("abstract", {AlignStrategy::None, false, false});

            // --- Beamer ---
            registerCommand("frametitle",     {1, 0, true});
            registerCommand("framesubtitle",  {1, 0, true});
            registerCommand("usetheme",       {1, 0, false});
            registerCommand("usecolortheme",  {1, 0, false});
            registerCommand("usefonttheme",   {1, 0, false});
            registerCommand("useinnertheme",  {1, 0, false});
            registerCommand("useoutertheme",  {1, 0, false});
            registerCommand("titlegraphic",   {1, 0, false});
            registerCommand("logo",           {1, 0, false});
            registerCommand("institute",      {1, 0, true});
            registerCommand("alert",          {1, 0, true});
            registerCommand("structure",      {1, 0, true});
            registerCommand("onslide",        {1, 0, false});
            registerCommand("only",           {2, 0, false});
            registerCommand("uncover",        {2, 0, false});
            registerCommand("visible",        {2, 0, false});
            registerCommand("invisible",      {2, 0, false});
            for (const auto& cmd : {"pause", "titlepage",
                                    "insertsectionhead", "insertsubsectionhead"}) {
                registerCommand(cmd, {0, 0, false});
            }
            registerEnv("frame",         {AlignStrategy::None, false, false});
            registerEnv("columns",       {AlignStrategy::None, false, false});
            registerEnv("column",        {AlignStrategy::None, false, false});
            registerEnv("block",         {AlignStrategy::None, false, false});
            registerEnv("exampleblock",  {AlignStrategy::None, false, false});
            registerEnv("alertblock",    {AlignStrategy::None, false, false});
            registerEnv("overlayarea",   {AlignStrategy::None, false, false});
            registerEnv("overprint",     {AlignStrategy::None, false, false});
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
