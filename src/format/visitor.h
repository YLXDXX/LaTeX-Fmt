#pragma once
#include <sstream>
#include <string>
#include <cctype>
#include "parse/ast.h"
#include "core/registry.h"
#include "core/unicode_width.h"
#include "format/math_aligner.h"

namespace latex_fmt {

    enum class CharCategory { None, ASCII, CJK, CJKPunct, Space, Other };

    inline CharCategory classify_cp(uint32_t cp) {
        if (cp == 0) return CharCategory::None;
        if (cp == ' ' || cp == '\t') return CharCategory::Space;
        if (cp == '~') return CharCategory::Other;
        if (cp < 0x80) {
            if (std::isalnum(static_cast<int>(cp))) return CharCategory::ASCII;
            return CharCategory::Other;
        }
        if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xAC00 && cp <= 0xD7AF)) return CharCategory::CJK;
        if ((cp >= 0x3000 && cp <= 0x303F) ||
            (cp >= 0xFF00 && cp <= 0xFFEF)) return CharCategory::CJKPunct;
        return CharCategory::Other;
    }

    inline CharCategory classify_first_char(std::string_view s) {
        size_t pos = 0;
        uint32_t cp = decode_utf8(s, pos);
        return classify_cp(cp);
    }

    inline CharCategory classify_last_char(std::string_view s) {
        size_t pos = 0;
        uint32_t last_cp = 0;
        while (pos < s.size()) {
            size_t prev = pos;
            uint32_t cp = decode_utf8(s, pos);
            if (cp == 0) break;
            if (cp != ' ' && cp != '\t') last_cp = cp;
        }
        return classify_cp(last_cp);
    }

    inline bool is_cjk_category(CharCategory cat) {
        return cat == CharCategory::CJK || cat == CharCategory::CJKPunct;
    }

    class FormatVisitor {
    public:
        FormatVisitor(const Registry& registry, std::string_view source)
        : registry_(registry), source_(source), at_line_start_(true) {}

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
            rule->align_strategy == AlignStrategy::Cases);

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
                    ScopedBuffer buf(*this);
                    visitNode(*child);
                    std::string part = buf.str();

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

            auto* sig = registry_.lookupCmd(n.name);
            if (!sig) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                endOutput(CharCategory::Other);
                return;
            }

            flushPendingSpace();
            writeText("\\" + n.name);

            for (size_t i = 0; i < n.args.size(); ++i) {
                const auto& arg = n.args[i];
                if (auto* g = dynamic_cast<const Group*>(arg.get())) {
                    output_ << g->delim_open;
                    for (const auto& child : g->children) {
                        visitNode(*child);
                    }
                    output_ << g->delim_close;
                    endOutput(CharCategory::ASCII);
                } else if (auto* t = dynamic_cast<const Text*>(arg.get())) {
                    if (t->content.size() == 1 && t->content != "{" && t->content != "[" &&
                        i >= (size_t)sig->optional_args && sig->mandatory_braces) {
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
            endOutput(CharCategory::ASCII);

            for (const auto& child : n.children) {
                visitNode(*child);
            }

            output_ << n.delim_close;
            endOutput(CharCategory::ASCII);
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
                    if (cp == 0) break;

                    CharCategory cat = classify_cp(cp);

                    if (cat == CharCategory::Space || cat == CharCategory::None) {
                        if (!output_ends_space_) {
                            processed += ' ';
                            output_ends_space_ = true;
                        }
                        continue;
                    }

                    bool need_space = false;
                    if (prev_cat == CharCategory::CJK && cat == CharCategory::ASCII) {
                        need_space = true;
                    } else if (prev_cat == CharCategory::ASCII && cat == CharCategory::CJK) {
                        need_space = true;
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

            if (text.size() >= 1 && text[0] == '%') {
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
                std::string out = output_.str();
                if (!out.empty() && (out.back() == ' ' || out.back() == '\t')) {
                    size_t last_non_space = out.find_last_not_of(" \t");
                    if (last_non_space != std::string::npos) {
                        output_.str(out.substr(0, last_non_space + 1));
                        output_.seekp(0, std::ios::end);
                    } else {
                        output_.str("");
                        output_.seekp(0, std::ios::end);
                    }
                }
                if (!out.empty()) output_ << ' ';
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
            ensureNewline();
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

            writeText("$");

            endOutput(CharCategory::ASCII);
            for (const auto& child : n.children) {
                visitNode(*child);
            }
            writeText("$");
            endOutput(CharCategory::ASCII);
        }

        void visit(const DisplayMath& n) {
            flushPendingSpace();

            ensureNewline();
            output_ << "$$";
            at_line_start_ = false;
            ensureNewline();

            indent_level_++;
            for (const auto& child : n.children) {
                visitNode(*child);
            }
            indent_level_--;

            ensureNewline();
            output_ << "$$";
            at_line_start_ = false;
            ensureNewline();

            endOutput(CharCategory::ASCII);
        }

        void visitNode(const ASTNode& n) {
            if (n.is_malformed) {
                flushPendingSpace();
                writeText(extractSource(n.source));
                return;
            }

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
        }

        std::string extractOutput() const {
            std::string out = output_.str();
            while (!out.empty() && (out.back() == ' ' || out.back() == '\t' || out.back() == '\n')) {
                out.pop_back();
            }
            return out;
        }

    private:
        std::ostringstream output_;
        int indent_level_ = 0;
        bool at_line_start_ = true;
        const Registry& registry_;
        std::string_view source_;
        CharCategory last_char_cat_ = CharCategory::None;
        bool pending_space_ = false;
        bool output_ends_space_ = false;

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
            return std::string(indent_level_ * 2, ' ');
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
            } else {
                output_ << text;
            }
        }

        void ensureNewline() {
            if (at_line_start_) return;

            std::string out = output_.str();
            size_t last_non_space = out.find_last_not_of(" \t");
            if (last_non_space != std::string::npos && last_non_space + 1 < out.size()) {
                output_.str(out.substr(0, last_non_space + 1));
                output_.seekp(0, std::ios::end);
            }

            output_ << "\n";
            at_line_start_ = true;
        }
    };

} // namespace latex_fmt