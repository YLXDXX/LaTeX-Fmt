#pragma once
#include <sstream>
#include <string>
#include "parse/ast.h"
#include "core/registry.h"
#include "core/char_category.h"
#include "core/config.h"
#include "format/math_aligner.h"

namespace latex_fmt {

    inline bool isTableRuleCommand(const std::string& name) {
        return name == "hline" || name == "toprule" || name == "midrule"
            || name == "bottomrule" || name == "cmidrule";
    }

    class FormatVisitor {
    public:
        FormatVisitor(const Registry& registry, std::string_view source)
        : registry_(registry), source_(source), at_line_start_(true) {}

        FormatVisitor(const Registry& registry, std::string_view source, const FormatConfig& config)
        : registry_(registry), source_(source), at_line_start_(true), config_(config) {}

        FormatVisitor(const Registry& registry, std::string_view source, int max_line_width)
        : registry_(registry), source_(source), at_line_start_(true) {
            config_.max_line_width = max_line_width;
        }

        void visit(const Document& n) {
            for (const auto& child : n.children) {
                visitNode(*child);
            }
        }

        void visit(const Environment& n) {
            if (n.is_malformed) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                return;
            }

            auto* rule = registry_.lookupEnv(n.name);
            bool is_verbatim = rule && rule->is_verbatim;
            bool needs_align = rule && (rule->align_strategy == AlignStrategy::AlignmentPair ||
            rule->align_strategy == AlignStrategy::Matrix ||
            rule->align_strategy == AlignStrategy::Cases ||
            rule->align_strategy == AlignStrategy::Tabular);

            ensureNewline();
            writeText("\\begin{" + n.name + "}");
            for (const auto& arg : n.args) {
                visitNode(*arg);
            }

            if (is_verbatim) {
                for (const auto& child : n.children) {
                    if (auto* t = dynamic_cast<const Text*>(child.get())) {
                        std::string content = t->content;
                        if (!content.empty() && content.front() == '\n') {
                            content = content.substr(1);
                        }
                        if (!content.empty() && content.back() == '\n') {
                            content.pop_back();
                        }
                        ensureNewline();
                        output_ << content;
                        at_line_start_ = false;
                        endOutput(CharCategory::Other);
                        ensureNewline();
                    }
                }
                output_ << "\\end{" << n.name << "}";
                at_line_start_ = false;
                endOutput(CharCategory::Other);
                return;
            }

            ensureNewline();

            bool is_document = (n.name == "document");
            if (!is_document) {
                indent_level_++;
            }

            if (needs_align) {
                std::vector<std::string> lines;
                std::ostringstream current_line;
                for (const auto& child : n.children) {
                    if (rule->align_strategy == AlignStrategy::Tabular) {
                        if (auto* cmd = dynamic_cast<const Command*>(child.get())) {
                            if (isTableRuleCommand(cmd->name)) {
                                std::string cl = current_line.str();
                                if (cl.find_first_not_of(" \t\n") != std::string::npos) {
                                    lines.push_back(std::move(cl));
                                }
                                current_line.str("");
                                current_line.clear();

                                ScopedBuffer buf(*this);
                                visitNode(*child);
                                std::string rule_text = buf.str();
                                lines.push_back(rule_text);
                                continue;
                            }
                        }
                    }

                    bool child_verb = false;
                    if (auto* t = dynamic_cast<const Text*>(child.get())) {
                        child_verb = t->is_verbatim;
                    }
                    ScopedBuffer buf(*this);
                    visitNode(*child);
                    std::string part = buf.str();

                    if (child_verb) {
                        for (size_t i = 0; i + 1 < part.size(); ++i) {
                            if (part[i] == '\\' && part[i+1] == '\\') {
                                part[i] = '\x02'; part[i+1] = '\x02'; ++i;
                            }
                        }
                        for (auto& c : part) if (c == '&') c = '\x01';
                    }

                    size_t pos = 0;
                    while (pos < part.size()) {
                        size_t nl_pos = part.find('\n', pos);
                        size_t bs_pos = part.find("\\\\", pos);
                        if (nl_pos == std::string::npos && bs_pos == std::string::npos) {
                            current_line << part.substr(pos);
                            break;
                        }
                        if (nl_pos != std::string::npos && (bs_pos == std::string::npos || nl_pos < bs_pos)) {
                            current_line << part.substr(pos, nl_pos - pos);
                            std::string cl = current_line.str();
                            if (cl.find_first_not_of(" \t\n") != std::string::npos) {
                                lines.push_back(std::move(cl));
                            }
                            current_line.str("");
                            current_line.clear();
                            pos = nl_pos + 1;
                        } else {
                            current_line << part.substr(pos, bs_pos - pos + 2);
                            lines.push_back(current_line.str());
                            current_line.str("");
                            current_line.clear();
                            pos = bs_pos + 2;
                        }
                    }
                }
                if (!current_line.str().empty()) {
                    lines.push_back(current_line.str());
                }

                std::string aligned = MathAligner::align(lines);
                for (auto& c : aligned) {
                    if (c == '\x01') c = '&';
                    if (c == '\x02') c = '\\';
                }

                std::string indent = getIndent();
                std::istringstream align_stream(aligned);
                std::string line;
                while (std::getline(align_stream, line)) {
                    if (!line.empty()) {
                        output_ << indent << line;
                    }
                    output_ << "\n";
                }
                at_line_start_ = true;
                endOutput(CharCategory::Other);
            } else {
                for (const auto& child : n.children) {
                    visitNode(*child);
                }
            }

            ensureNewline();

            if (!is_document) {
                indent_level_--;
            }

            writeText("\\end{" + n.name + "}");
        }

        void visit(const Command& n) {
            if (n.is_malformed) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                endOutput(CharCategory::Other);
                return;
            }

            if (config_.remove_tags && (n.name == "tag" || n.name == "tag*")) {
                return;
            }

            auto* sig = registry_.lookupCmd(n.name);
            if (!sig) {
                flushPendingSpace();
                writeText("\\" + n.name);
                for (const auto& arg : n.args) {
                    visitNode(*arg);
                }
                if (n.args.empty()) {
                    endOutput(CharCategory::Other);
                }
                return;
            }

            flushPendingSpace();
            writeText("\\" + n.name);

            size_t actual_opt = 0;
            for (const auto& a : n.args) {
                if (auto* g = dynamic_cast<const Group*>(a.get())) {
                    if (g->delim_open == "[") actual_opt++;
                    else break;
                } else {
                    break;
                }
            }

            for (size_t i = 0; i < n.args.size(); ++i) {
                const auto& arg = n.args[i];
                if (auto* g = dynamic_cast<const Group*>(arg.get())) {
                    output_ << g->delim_open;
                    for (const auto& child : g->children) {
                        visitNode(*child);
                    }
                    output_ << g->delim_close;
                    endOutput(CharCategory::Other);
                } else if (auto* t = dynamic_cast<const Text*>(arg.get())) {
                    bool do_brace = config_.brace_completion
                        && t->content.size() == 1
                        && t->content != "{" && t->content != "["
                        && i >= actual_opt
                        && sig->mandatory_braces;
                    if (do_brace) {
                        writeText("{" + t->content + "}");
                        endOutput(CharCategory::ASCII);
                    } else {
                        writeText(t->content);
                        endOutput(CharCategory::ASCII);
                    }
                } else {
                    visitNode(*arg);
                }
            }
            if (n.args.empty()) {
                endOutput(CharCategory::ASCII);
            }
        }

        void visit(const Group& n) {
            if (n.is_malformed) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                return;
            }

            output_ << n.delim_open;
            line_pos_ += static_cast<int>(n.delim_open.size());
            endOutput(CharCategory::Other);

            for (const auto& child : n.children) {
                visitNode(*child);
            }

            output_ << n.delim_close;
            line_pos_ += static_cast<int>(n.delim_close.size());
            endOutput(CharCategory::Other);
        }

        void visit(const Text& n) {
            if (n.is_malformed) {
                writeText(extractSource(n.source));
                return;
            }

            bool in_math = (n.context == ParseContext::InlineMath ||
            n.context == ParseContext::DisplayMath ||
            n.context == ParseContext::MathEnv);

            std::string content = n.content;

            if (!in_math) {
                std::string processed;
                size_t pos = 0;
                CharCategory prev_cat = last_char_cat_;

                while (pos < content.size()) {
                    size_t byte_start = pos;
                    uint32_t cp = decode_utf8(content, pos);
                    if (cp == UTF8_EOF) break;

                    CharCategory cat = classify_cp(cp);

                    if (cat == CharCategory::Space || cat == CharCategory::None) {
                        if (!output_ends_space_) {
                            processed += ' ';
                            output_ends_space_ = true;
                        }
                        continue;
                    }

                    bool need_space = false;
                    if (config_.cjk_spacing) {
                        if (prev_cat == CharCategory::CJK && cat == CharCategory::ASCII) {
                            need_space = true;
                        } else if (prev_cat == CharCategory::ASCII && cat == CharCategory::CJK) {
                            need_space = true;
                        }
                    }

                    if (need_space && !output_ends_space_) {
                        processed += ' ';
                    }

                    output_ends_space_ = false;
                    processed += content.substr(byte_start, pos - byte_start);
                    prev_cat = cat;
                }

                if (!processed.empty()) {
                    flushPendingSpace();
                    writeText(processed);
                    last_char_cat_ = prev_cat;
                    output_ends_space_ = (processed.back() == ' ');
                }
            } else {
                flushPendingSpace();
                if (config_.brace_completion) {
                    std::string processed;
                    size_t i = 0;
                    while (i < content.size()) {
                        unsigned char c = static_cast<unsigned char>(content[i]);
                        if (c == '_' || c == '^') {
                            processed += c;
                            i++;

                            if (i >= content.size()) {
                                pending_sub_super_brace_ = true;
                                continue;
                            }

                            if (content[i] == '{') {
                                processed += content.substr(i);
                                break;
                            }

                            if (content[i] == ' ' || content[i] == '\t') {
                                continue;
                            }

                            size_t cp_start = i;
                            uint32_t cp = decode_utf8(content, i);
                            if (cp != UTF8_EOF) {
                                processed += '{';
                                processed += content.substr(cp_start, i - cp_start);
                                processed += '}';
                            }
                        } else {
                            processed += c;
                            i++;
                        }
                    }
                    content = processed;
                }

                if (at_line_start_) {
                    size_t first_non_space = content.find_first_not_of(" \t");
                    if (first_non_space != std::string::npos) {
                        output_ << getIndent();
                        at_line_start_ = false;
                        output_ << content.substr(first_non_space);
                    }
                } else {
                    output_ << content;
                }
                endOutput(CharCategory::Other);
            }
        }

        void visit(const CommentNode& n) {
            flushPendingSpace();
            std::string text = n.text;

            if (config_.comment_normalize && text.size() >= 1 && text[0] == '%') {
                std::string body = text.substr(1);

                size_t start = body.find_first_not_of(" \t");
                if (start == std::string::npos) start = body.size();
                body = body.substr(start);

                if (body.empty()) {
                    text = "%";
                } else {
                    text = "% " + body;
                }
            }

            if (n.is_line_end) {
                bool has_content = false;
                {
                    std::string out = output_.str();
                    if (!out.empty() && (out.back() == ' ' || out.back() == '\t')) {
                        size_t last_non_space = out.find_last_not_of(" \t");
                        if (last_non_space != std::string::npos) {
                            output_.str(out.substr(0, last_non_space + 1));
                            output_.seekp(0, std::ios::end);
                            has_content = true;
                        } else {
                            output_.str("");
                            output_.seekp(0, std::ios::end);
                        }
                    } else if (!out.empty()) {
                        has_content = true;
                    }
                }
                if (has_content) output_ << ' ';
                output_ << text;
                at_line_start_ = false;
                endOutput(CharCategory::Other);
            } else {
                ensureNewline();
                writeText(text);
            }
        }

        void visit(const ParBreak& n) {
            flushPendingSpace();
            if (config_.blank_line_compress) {
                ensureNewline();
            }
            output_ << "\n";
            at_line_start_ = true;
            endOutput(CharCategory::Other);
        }

        void visit(const Newline& n) {
            flushPendingSpace();
            ensureNewline();
        }

        void visit(const InlineMath& n) {
            if (last_char_cat_ == CharCategory::CJK) {
                pending_space_ = true;
            }
            flushPendingSpace();

            bool use_dollar = config_.math_delimiter_unify || n.is_dollar_form;
            if (use_dollar) {
                writeText("$");
            } else {
                writeText("\\(");
            }

            endOutput(CharCategory::ASCII);
            for (const auto& child : n.children) {
                visitNode(*child);
            }
            pending_sub_super_brace_ = false;
            if (use_dollar) {
                writeText("$");
            } else {
                writeText("\\)");
            }
            endOutput(CharCategory::ASCII);
        }

        void visit(const DisplayMath& n) {
            flushPendingSpace();

            bool use_unify = config_.math_delimiter_unify;
            auto style = config_.display_math_style;

            std::string delim_open, delim_close;
            if (!use_unify) {
                delim_open = n.is_dollar_form ? "$$" : "\\[";
                delim_close = n.is_dollar_form ? "$$" : "\\]";
            } else {
                switch (style) {
                    case DisplayMathStyle::Bracket:
                        delim_open = "\\["; delim_close = "\\]"; break;
                    case DisplayMathStyle::Equation:
                        delim_open = "\\begin{equation}"; delim_close = "\\end{equation}"; break;
                    case DisplayMathStyle::EquationStar:
                        delim_open = "\\begin{equation*}"; delim_close = "\\end{equation*}"; break;
                    default:
                        delim_open = "$$"; delim_close = "$$"; break;
                }
            }

            if (config_.display_math_format) {
                ensureNewline();
            }
            output_ << delim_open;
            at_line_start_ = false;

            if (config_.display_math_format) {
                ensureNewline();
                indent_level_++;
            }

            for (const auto& child : n.children) {
                visitNode(*child);
            }
            pending_sub_super_brace_ = false;

            if (config_.display_math_format) {
                indent_level_--;
                ensureNewline();
            }

            output_ << delim_close;
            at_line_start_ = false;

            if (config_.display_math_format) {
                ensureNewline();
            }

            endOutput(CharCategory::ASCII);
        }

        void visitNode(const ASTNode& n) {
            if (n.is_malformed) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                return;
            }

            bool sub_super_wrap = pending_sub_super_brace_;
            if (sub_super_wrap) {
                pending_sub_super_brace_ = false;
                if (dynamic_cast<const Group*>(&n)) {
                    sub_super_wrap = false;
                }
            }
            if (sub_super_wrap) output_ << '{';

            if (auto* p = dynamic_cast<const Document*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const Environment*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const Command*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const Group*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const Text*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const CommentNode*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const ParBreak*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const Newline*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const InlineMath*>(&n)) visit(*p);
            else if (auto* p = dynamic_cast<const DisplayMath*>(&n)) visit(*p);

            if (sub_super_wrap) output_ << '}';
        }

        std::string extractOutput() const {
            std::string out = output_.str();
            while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) {
                out.pop_back();
            }
            bool had_nl = false;
            while (!out.empty() && out.back() == '\n') {
                out.pop_back();
                had_nl = true;
            }
            if (had_nl) out.push_back('\n');
            return out;
        }

        const std::vector<std::string>& getWarnings() const {
            return warnings_;
        }

    private:
        std::ostringstream output_;
        int indent_level_ = 0;
        bool at_line_start_ = true;
        const Registry& registry_;
        std::string_view source_;
        CharCategory last_char_cat_ = CharCategory::None;
        bool pending_space_ = false;
        bool pending_sub_super_brace_ = false;
        bool output_ends_space_ = false;
        int line_pos_ = 0;
        int line_number_ = 1;
        mutable std::vector<std::string> warnings_;
        FormatConfig config_;

        struct ScopedBuffer {
            std::ostringstream temp;
            bool saved_ls, saved_ps, saved_es;
            CharCategory saved_lc;
            FormatVisitor& v;

            ScopedBuffer(FormatVisitor& vis) : v(vis) {
                std::swap(v.output_, temp);
                saved_ls = v.at_line_start_;
                saved_lc = v.last_char_cat_;
                saved_ps = v.pending_space_;
                saved_es = v.output_ends_space_;
                v.at_line_start_ = false;
                v.last_char_cat_ = CharCategory::None;
                v.pending_space_ = false;
                v.output_ends_space_ = false;
            }

            ~ScopedBuffer() {
                std::swap(v.output_, temp);
                v.at_line_start_ = saved_ls;
                v.last_char_cat_ = saved_lc;
                v.pending_space_ = saved_ps;
                v.output_ends_space_ = saved_es;
            }

            std::string str() const { return v.output_.str(); }
        };

        void endOutput(CharCategory cat) {
            last_char_cat_ = cat;
            output_ends_space_ = false;
        }

        void endOutputSpace() {
            output_ends_space_ = true;
        }

        std::string extractSource(SourceRange range) const {
            return std::string(source_.substr(range.begin_offset, range.end_offset - range.begin_offset));
        }

        std::string getIndent() const {
            return std::string(indent_level_ * config_.indent_width, ' ');
        }

        void flushPendingSpace() {
            if (pending_space_) {
                if (!at_line_start_) {
                    std::string out = output_.str();
                    if (!out.empty() && out.back() != ' ' && out.back() != '\n') {
                        output_ << ' ';
                    }
                }
                pending_space_ = false;
            }
        }

        void writeText(const std::string& text) {
            if (text.empty()) return;

            if (at_line_start_) {
                size_t first_non_space = text.find_first_not_of(" \t");
                if (first_non_space == std::string::npos) {
                    return;
                }
                output_ << getIndent();
                at_line_start_ = false;
                output_ << text.substr(first_non_space);
                line_pos_ = static_cast<int>(first_non_space) + indent_level_ * config_.indent_width;
            } else {
                output_ << text;
                line_pos_ += static_cast<int>(text.size());
            }
            auto mw = config_.max_line_width;
            if (mw > 0 && line_pos_ > mw) {
                warnings_.push_back("line " + std::to_string(line_number_)
                    + ": exceeds max width " + std::to_string(mw)
                    + " (" + std::to_string(line_pos_) + " chars)");
            }
        }

        void ensureNewline() {
            if (at_line_start_) return;

            if (config_.trailing_whitespace_remove) {
                std::string out = output_.str();
                size_t last_non_space = out.find_last_not_of(" \t");
                if (last_non_space != std::string::npos && last_non_space + 1 < out.size()) {
                    output_.str(out.substr(0, last_non_space + 1));
                    output_.seekp(0, std::ios::end);
                }
            }

            output_ << "\n";
            at_line_start_ = true;
            line_pos_ = 0;
            line_number_++;
        }
    };

} // namespace latex_fmt