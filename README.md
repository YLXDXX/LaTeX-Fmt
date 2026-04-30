# 🛠️ LaTeX 格式化工具 (LaTeX-Fmt) 开发要求说明书

## 一、 项目概述

**项目目标**：使用 C++ 开发一个命令行 LaTeX 代码格式化工具，实现代码的**美化打印**和**规范化**。
**核心铁律**：**绝对不能因为格式化导致原本能编译的 LaTeX 代码无法编译！** 任何拿不准的情况，宁可原样保留，绝不乱改。

## 二、 技术栈与环境

- **语言标准**：C++17（必须使用 `std::string_view`、`std::variant`、`std::optional` 等特性）
- **构建系统**：CMake 3.16+
- **测试框架**：Catch2 (Header-only 版本)
- **第三方依赖**：
  - 允许引入轻量级 header-only 的 UTF-8 解码库（如 `uni-algo` 或 `utf8.h`）用于处理字符宽度。
  - **禁止**引入 ICU 等重型依赖。

## 三、 核心架构与模块划分

项目严格按照以下目录结构组织，各模块职责分明，严禁跨层调用：

```text
latex-fmt/
├── src/
│   ├── core/          # 基础数据结构：AST节点、源码位置、Unicode宽度
│   ├── parse/         # 词法分析、语法分析、注册表
│   ├── format/        # 格式化引擎、对齐算法
│   └── main.cpp       # 入口
├── tests/             # 测试用例
└── external/          # 第三方库
```

---

## 四、 模块详细开发要求

### 模块 1：基础设施

#### 1.1 源码位置追踪 (`source_location.h`)

**要求**：AST 的每个节点必须记住自己在原始文本中的位置，这是错误恢复的救命稻草。

```cpp
struct SourceRange {
    size_t begin_offset; // 起始字节偏移量
    size_t end_offset;   // 结束字节偏移量
};
```

#### 1.2 轻量级字符宽度计算 (`unicode_width.h`)

**背景**：对齐算法不能用 `string.length()`（字节数），因为 UTF-8 中一个中文字符占 3 字节但排版宽度为 2。
**要求**：

1. 实现 `int display_width(std::string_view str)` 函数。
2. 逻辑：遍历 UTF-8 码点，ASCII 字符宽度为 1，CJK 字符（中日韩）宽度为 2，组合符号宽度为 0。
3. **严禁使用 ICU 库**。

#### 1.3 注册表 (`registry.h`)

**背景**：格式化器必须知道 `\frac` 吃 2 个参数，`\sqrt` 吃 1 个可选和 1 个必选参数。自定义命令未来也要能加进来。
**要求**：

```cpp
// 命令签名
struct CommandSignature {
    int mandatory_args = 0;     // 必选参数个数 {}
    int optional_args = 0;     // 可选参数个数 []
    bool mandatory_braces = true; // 是否强制补全花括号（如 \frac12 -> \frac{1}{2}）
};

// 环境规则
enum class AlignStrategy { None, AlignmentPair, Matrix, Cases };
struct EnvRule {
    AlignStrategy align_strategy = AlignStrategy::None;
    bool is_verbatim = false; // 是否是原样输出环境（如 verbatim, lstlisting）
    bool is_math = false;     // 是否是数学环境
};

class Registry {
public:
    void registerBuiltin(); // 硬编码常见规则
    void registerCommand(const std::string& name, CommandSignature sig);
    void registerEnv(const std::string& name, EnvRule rule);
    const CommandSignature* lookupCmd(const std::string& name) const;
    const EnvRule* lookupEnv(const std::string& name) const;
};
```

**⚠️ 开发者注意**：遇到 `lookupCmd` 返回 `nullptr` 的未知命令，**绝对不碰**，原样输出！

---

### 模块 2：词法与语法分析

#### 2.1 词法分析器 (`lexer.h`)

**要求**：将输入文本转化为 Token 流。必须具备**环境感知逃逸模式**。
**核心难点攻克**：

1. **Verbatim 逃逸**：遇到 `\begin{verbatim}` 时，推入 `LexMode::VerbatimSeekEnd` 模式，此时词法分析器停止解析任何 LaTeX 命令，只做纯文本匹配寻找 `\end{verbatim}`。
2. **`\verb` 处理**：遇到 `\verb` 或 `\verb*` 时，推入 `LexMode::VerbSeekDelim`。紧接的下一个字符就是定界符（如 `|`、`!`、`+`），寻找下一个相同定界符之间的内容作为原始文本保留。
3. **注释**：`%` 及其后的行末内容必须作为一个单独的 `CommentToken` 输出，不能直接丢弃！

#### 2.2 抽象语法树 AST (`ast.h`)

**要求**：使用虚基类 + `std::unique_ptr` 管理内存。每个节点必须包含上下文信息。

```cpp
enum class ParseContext {
    Text, InlineMath, DisplayMath, MathEnv, TikZ, Verbatim
};

struct ASTNode {
    SourceRange source;
    ParseContext context;
    bool is_malformed = false; // 标记解析是否失败
    virtual ~ASTNode() = default;
};

// 必须实现以下节点（可根据需要扩充）
struct Document : ASTNode { std::vector<std::unique_ptr<ASTNode>> children; };
struct Environment : ASTNode { std::string name; /* args, children */ };
struct Command : ASTNode { std::string name; /* args */ };
struct Text : ASTNode { std::string content; };
struct CommentNode : ASTNode { std::string text; bool is_line_end; };
struct ParBreak : ASTNode {}; // 空行分段
```

#### 2.3 递归下降解析器 (`parser.h`)

**要求**：根据 Token 流构建 AST。
**错误恢复策略（极重要）**：
当遇到不匹配的花括号或缺少 `\end` 时，解析器**绝对不能崩溃或抛出异常终止**！

1. 将当前异常部分包装成一个 `ASTNode`，设置 `is_malformed = true`。
2. 向后扫描 Token，寻找"安全点"（如 `\end`、空行、`\section`）。
3. 从安全点恢复继续解析。

---

### 模块 3：格式化引擎

#### 3.1 访问者模式 (`visitor.h`)

**要求**：格式化逻辑与 AST 彻底解耦。使用 Visitor 模式遍历 AST 生成新代码。

```cpp
class FormatVisitor {
public:
    void visit(const Document& n);
    void visit(const Environment& n);
    void visit(const Command& n);
    void visit(const Text& n);
    void visit(const CommentNode& n);
    // ...
    std::string extractOutput();
private:
    std::ostringstream output_;
    int indent_level_ = 0;
    const Registry& registry_;
};
```

**核心铁律 1**：当遇到 `node.is_malformed == true` 时，直接从 `node.source` 提取原始字符串拼接到 `output_`，**不做任何格式化**！
**核心铁律 2**：当遇到 Registry 中未注册的未知命令时，连同其参数原样输出，**不做任何规范化**！

#### 3.2 按环境分块缓冲

**要求**：对于普通文本，边遍历边写流；对于 `align`、`matrix` 等需要列对齐的环境，必须**先收集该环境内所有行，计算每列最大宽度，再一次性格式化输出**。

#### 3.3 数学环境对齐算法 (`math_aligner.h`)

**要求**：

1. 只处理已有 `\\` 换行的公式，**绝不主动为长公式拆行**。
2. 使用之前写的 `display_width()` 计算列宽，按最宽列补齐空格。
3. 不同环境对齐策略遵循 `EnvRule::align_strategy`：
   - `AlignmentPair` (align): `&` 成对交替左右对齐。
   - `Matrix` (pmatrix等): `&` 作为列分隔，列间等宽。

---

## 五、 规范化规则执行标准

开发者必须严格按照以下规则实现格式化，不可自行发挥：

| 规则编号  | 规则名称           | 执行标准                                                     | 难度/警告 |
| --------- | ------------------ | ------------------------------------------------------------ | --------- |
| **R1**    | 统一数学定界符     | 将 `\( ... \)` 统一转为 `$ ... $`；将 `\[ ... \]` 统一转为 `$$ ... $$`（或统一环境，视配置而定） | 低        |
| **R2**    | 常见命令补全花括号 | **仅针对 Registry 中 `mandatory_braces=true` 的命令**。如 `\frac12` -> `\frac{1}{2}`。**严禁对未知命令执行此操作！** | 高危      |
| **R4**    | 去冗余空白         | 文本模式：多个空格变1个。**数学模式：绝对不能随便删空格！** `a b` 和 `ab` 在数学模式排版不同，数学模式只删 `&` 或 `=` 附近多余的格式化空格。 | 高危      |
| **R5**    | 统一换行缩进       | 根据 `\begin` 和 `\end` 层级，每层增加固定缩进（如2空格）。空行原样保留。 | 低        |
| **R6/R7** | 表格与公式对齐     | 按照 `MathAligner` 计算结果填充空格，保证 `&` 纵向对齐。     | 中        |

### 注释处理规则 (极容易翻车)

1. AST 中注释作为独立节点 `CommentNode`。
2. 独立行注释：保持原有缩进级别。
3. 行末注释 (`is_line_end == true`)：必须与前导内容保持**至少 1 个空格**的间距。对齐操作插入的空格不能挤压注释。
   *正确示例*：`x &= 1 % comment`（1和%之间有空格）

---

## 六、 测试要求（强制性）

没有测试的代码不予合并！必须包含以下四类测试：

### T1: 单元测试

- 必须测试：`display_width()` 对中文、ASCII、Emoji 的宽度计算。
- 必须测试：`Registry` 的查询、默认规则加载。
- 必须测试：Lexer 对 `\verb|...|` 的定界符提取。

### T2: 快照测试 (最核心)

在 `tests/snapshots/` 目录下创建 `.tex` 输入文件和 `.expected.tex` 预期文件。
运行格式化器，比对输出与预期文件必须**逐字节完全一致**。
必须覆盖的场景：

1. 简单中英文混排
2. 嵌套环境（如公式中套矩阵）
3. 包含 `\verb` 和 `verbatim` 的代码
4. 花括号不匹配的错误恢复
5. 行末注释与独立注释

### T3: 幂等测试 (铁律)

**格式化后的代码再次格式化，结果必须和第一次格式化完全一样！**
如果二次格式化结果不同，说明你的规则写出了副作用（比如每次都在行尾加空格），必须修复。

### T4: 编译回归测试

对于每一个快照测试用例，必须写脚本执行：

1. 用 `xelatex` 编译原始 `.tex`。
2. 用 `xelatex` 编译格式化后的 `.tex`。
3. 如果原文件编译成功，但格式化后文件编译失败，**这是 P0 级严重 Bug！** 必须立即修复。

---

## 七、 开发里程碑

请按照以下顺序开发，严禁跨阶段：

1. **Phase 1 (基建)**：搭建 CMake，实现 `Registry`、`SourceRange`、`display_width`。写完 T1 测试。
2. **Phase 2 (解析)**：实现 `Lexer`（重点攻克 Verbatim 和注释），实现 `Parser`（重点攻克错误恢复 `is_malformed`），构建出完整的 AST。写快照测试验证 AST 正确性。
3. **Phase 3 (基础格式化)**：实现 `FormatVisitor`，先只做 R5(缩进) 和 R1(定界符统一)。**此时必须通过 T3(幂等) 和 T4(编译) 测试。**
4. **Phase 4 (高阶格式化)**：实现 `MathAligner`，加入 R6/R7(对齐) 和 R2(花括号补全)。补充大量数学公式的快照测试。
5. **Phase 5 (收尾)**：完善注释处理逻辑，跑通全量 T4 编译回归测试。

---

**给开发者的最后忠告**：LaTeX 是一门极其魔幻的语言，用户写出的代码千奇百怪。当你犹豫要不要修改某段代码时，请默念三遍：**"如果我改错了，用户的论文就编译不过了"**，然后选择原样保留。祝好运！