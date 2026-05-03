# LaTeX-Fmt

命令行 LaTeX 代码格式化工具，提供代码美化与规范化能力。

**核心原则：绝不因格式化导致原本能编译的代码无法编译。** 遇到不确定的情况，宁保留原样，不随意修改。

## 快速开始

### 构建

环境要求：C++17、CMake 3.16+。

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

生成的可执行文件为 `build/latex-fmt`。

### 安装

```bash
# 方式一：在项目根目录执行
cmake --install build                     # 安装到默认位置（/usr/local）
cmake --install build --prefix /usr/local # 显式指定安装路径

# 方式二：在 build 目录内执行
cd build && make install                  # 使用 Makefile 安装

# 安装到用户本地目录（无需 root 权限）
cmake --install build --prefix ~/.local
make install DESTDIR=~/.local             # 或从 build 目录执行
```

### 基本使用

```bash
# 从标准输入读取，格式化到标准输出
latex-fmt < input.tex > output.tex

# 格式化文件并输出到标准输出
latex-fmt paper.tex

# 原地格式化
latex-fmt -i paper.tex

# 输出到指定文件
latex-fmt -o out.tex paper.tex

# Markdown 转 LaTeX 并格式化
latex-fmt --md < input.md > output.tex
```

## 功能特性

以下格式化规则**默认全部启用**，可通过命令行参数关闭。

| 规则 | 说明 |
|------|------|
| 数学定界符统一 | `\(...\)` → `$...$`，`\[...\]` → `$$...$$`（行间公式样式可通过 `--display-math-style` 配置） |
| 花括号补全 | `\frac12` → `\frac{1}{2}`、`\sqrt3` → `\sqrt{3}`（仅限已知命令）；数学模式上下标简写 `x_a` → `x_{a}`、`x^2` → `x^{2}` |
| 去冗余空白 | 文本模式多个空格压缩为 1 个；数学模式保留空格 |
| 缩进与换行 | 按 `\begin`/`\end` 层级自动缩进（默认每层 2 空格） |
| 表格与公式对齐 | 表格（`tabular`/`tblr` 等）和数学环境（`align`/`matrix`/`cases`）中按 `&` 纵向对齐单元格，`\hline` 等分隔线独立成行 |
| CJK-English 间距 | 中文与英文/数字之间自动添加空格 |
| 注释规范化 | `%comment` → `% comment` |
| 行尾空格清除 | 每行末尾空格全部删除 |
| 空白行处理 | 纯空白行变为空行；连续多空行压缩为 1 个 |
| 行间公式独立 | `$$...$$` 自动独立成行并缩进（输出样式可通过 `--display-math-style` 切换为 `\[...\]` / `\begin{equation}` / `\begin{equation*}`） |
| 错误恢复 | 不匹配的花括号、缺失 `\end` 时不会崩溃，尽力恢复 |
| 方括号智能处理 | 数学模式中 `[`/`]` 视为普通字符；仅在命令/环境可选参数位置检查配对 |
| 长行警告与自动换行 | `--max-line-width=N` 警告超长行；`--wrap` / `--wrap-paragraphs` 自动折行 |
| 语法检查 | `--syntax-check` 检查环境/花括号/数学定界符配对，报告位置 |
| 语法修复 | `--syntax-fix` 基于语法检查结果自动补全缺失的 `}` `]` `$` `$$` `\end` 等 |
| 删除公式编号 | `--remove-tags` 删除 `\tag{...}` 和 `\tag*{...}` 命令 |

### 可配置参数

所有规则均可通过命令行或配置文件单独控制

| 参数 | 默认值 | 说明 |
|------|:---:|------|
| `--indent-width=N` | 2 | 缩进宽度（空格数） |
| `--no-cjk-spacing` / `--cjk-spacing` | 开启 | CJK 与 ASCII 字符间自动插入空格 |
| `--no-brace-completion` / `--brace-completion` | 开启 | 单字符参数自动补全花括号 |
| `--no-comment-normalize` / `--comment-normalize` | 开启 | `%comment` → `% comment` |
| `--no-blank-line-compress` / `--blank-line-compress` | 开启 | 连续多空行压缩为 1 个 |
| `--keep-trailing-spaces` / `--remove-trailing-spaces` | 删除 | 是否保留行尾空格 |
| `--no-display-math-format` / `--display-math-format` | 开启 | `$$...$$` 独立成行 |
| `--no-math-unify` / `--math-unify` | 开启 | 数学定界符统一为 `$` / `$$` |
| `--display-math-style=S` | `dollar` | 行间公式输出样式：`dollar`（`$$`）、`bracket`（`\[`）、`equation`、`equation*` |
| `--max-line-width=N` | 0（关闭） | 当 N > 0 时，超长行触发警告 |
| `--wrap` | 关闭 | 配合 `--max-line-width` 自动折行 |
| `--wrap-paragraphs` | 关闭 | 纯文本段落长行自动折行 |
| `--syntax-check` | 关闭 | 格式化前检查语法错误（环境/括号/数学定界符配对） |
| `--syntax-fix` | 关闭 | 自动补全缺失的 `}` `]` `$` `\end` 等定界符 |
| `--remove-tags` | 关闭 | 删除公式中的 `\tag{...}` / `\tag*{...}` 命令 |
| `--context-chars=N` | 10 | `--syntax-check` 错误信息中显示的前后文字数 |

## 使用指南

### 检查、diff 与修复模式

```bash
# 检查是否需要格式化（CI 用，有变更时退出码为 1）
latex-fmt --check paper.tex

# 显示 unified diff 差异
latex-fmt --diff paper.tex
```

### 调整格式化参数

```bash
# 缩进宽度设为 4
latex-fmt --indent-width=4 paper.tex

# 关闭 CJK-ASCII 自动空格
latex-fmt --no-cjk-spacing paper.tex

# 长行自动折行（行宽 80）
latex-fmt --max-line-width=80 --wrap-paragraphs paper.tex

# 行间公式输出为 \begin{equation}...\end{equation}
latex-fmt --display-math-style=equation paper.tex

# 组合多项参数
latex-fmt --indent-width=4 --no-cjk-spacing --quiet paper.tex
```

### 递归处理

```bash
# 原地格式化目录下所有 .tex 文件
latex-fmt -r -i project/

# 递归检查目录
latex-fmt -r --check project/

# 递归显示 diff
latex-fmt -r --diff project/
```

### Markdown 转换

通过 `--md` 选项，可将 Markdown 文本转换为 LaTeX 代码并自动格式化：

```bash
# 从 Markdown 文件转换
latex-fmt --md input.md -o output.tex

# 从标准输入读取 Markdown
cat README.md | latex-fmt --md > readme.tex

# 配合其他格式化参数使用
latex-fmt --md --indent-width=4 notes.md
```

当前支持的 Markdown 语法：

| 语法 | Markdown | LaTeX |
|------|------|------|
| 标题 | `# Title` | `\section{Title}` |
| 加粗 | `**bold**` | `\textbf{bold}` |
| 斜体 | `*italic*` | `\textit{italic}` |
| 行内代码 | `` `code` `` | `\texttt{code}` |
| 代码块 | ` ```...``` ` | `\begin{verbatim}...\end{verbatim}` |
| 链接 | `[text](url)` | `\href{url}{text}` |
| 无序列表 | `- item` | `\begin{itemize} \item item \end{itemize}` |
| 有序列表 | `1. item` | `\begin{enumerate} \item item \end{enumerate}` |
| 引用 | `> quote` | `\begin{quote}quote\end{quote}` |
| 行内公式 | `$E=mc^2$` | 原样保留（不转义内部字符） |
| 行间公式 | `$$...$$` | 原样保留（内容不做转义） |

> **注意**：公式中的 `&`、`\\`、`_`、`#` 等 LaTeX 特殊字符不会被转义，`$...$` 和 `$$...$$` 支持跨多行。`$$` 单独成行时视为行间公式，前方无空行时视为内联闭合定界符。

### 语法检查与智能修复

`--syntax-check` 在格式化前对文档进行基本语法校验，`--syntax-fix` 能自动修复检测到的缺失定界符问题。

```bash
# 仅检查语法（不格式化），有错误时退出码为 1
latex-fmt --syntax-check paper.tex

# 自动修复缺失的定界符并格式化
latex-fmt --syntax-fix paper.tex

# 调整错误上下文显示字符数（默认 10）
latex-fmt --syntax-check --context-chars=20 paper.tex
```

检测和修复覆盖范围：

| 问题类型 | 示例 | `--syntax-check` | `--syntax-fix` |
|---------|------|:---:|:---:|
| 未关闭环境 | `\begin{document}` 缺少 `\end{document}` | 报错 | 插入 `\end{document}` |
| 未关闭行内公式 | `$x+y` 缺少闭合 `$` | 报错 | 插入 `$` |
| 未关闭行间公式 | `$$x+y` 缺少闭合 `$$` | 报错 | 插入 `$$` |
| 未关闭花括号 | `\textbf{Bold` 缺少 `}` | 报错 | 插入 `}` |
| 未关闭方括号（命令可选参数） | `\sqrt[3{2}` 缺少 `]` | 报错 | 智能插入 `]` |
| 多余闭合定界符 | `}text` 多余 `}` | 报错 | 插入 `{` 配对 |
| 环境名缺括号 | `\begin{equation` 缺 `}` | 报错 | 自动识别 |

> **关于方括号**：`[` / `]` 仅在紧跟在已注册命令或环境名后时视为可选参数分隔符。在数学模式（`$...$` / `$$...$$`）和普通文本中，`[` 和 `]` 是普通的字面字符，不需要成对出现。例如张量记号 `\delta^{[a_1}_{a_1}` 中的 `[` 不会触发语法错误。

> **注意**：`--syntax-check` 仅报告错误不修改文件。`--syntax-fix` 会修改源文件内容后再格式化。两者均默认关闭，适用于信任文档能正常编译的场景。

### 删除公式编号

使用 `--remove-tags` 可以删除从其他文档粘贴过来的公式中带有的 `\tag{...}` 编号，避免原文编号与新文档冲突：

```bash
# 删除所有 \tag{...} 和 \tag*{...} 命令
latex-fmt --remove-tags paper.tex
```

支持 `\tag{}` 和 `\tag*{}` 两种形式，适用于行间公式（`$$...$$`）和各数学环境（`equation`、`align` 等）。

### 其他选项

```bash
# 抑制警告输出
latex-fmt --quiet paper.tex

# 指定自定义配置文件
latex-fmt --config-file=/path/to/.latexfmtrc paper.tex
```

## 配置文件

latex-fmt 支持 `.latexfmtrc` 配置文件，按以下优先级查找（后找到的覆盖先找到的）：

1. `~/.latexfmtrc`（全局配置）
2. `./.latexfmtrc`（项目目录配置）
3. `--config-file=<path>` 指定的文件（最高优先级）

命令行参数始终覆盖配置文件中的设置。

配置文件格式为 `key = value`，`#` 开头的行为注释：

```ini
# .latexfmtrc 示例
indent_width = 4
max_line_width = 80
cjk_spacing = false
wrap_paragraphs = true

# 布尔值支持: true / false / 1 / 0 / yes / no / on / off
brace_completion = true
comment_normalize = true

# 行间公式输出样式: dollar / bracket / equation / equation*
display_math_style = bracket
```

## 项目结构

```text
latex-fmt/
├── src/
│   ├── core/              # 基础类型与配置
│   │   ├── config.h       # FormatConfig 配置结构体
│   │   ├── registry.h     # 命令/环境签名注册表
│   │   ├── char_category.h# 字符分类工具
│   │   ├── unicode_width.h# UTF-8 解码与字符宽度
│   │   ├── source_location.h
│   │   ├── syntax_check.h  # 语法检查器
│   │   └── syntax_fix.h    # 语法修复器
│   ├── parse/             # 词法与语法分析
│   │   ├── ast.h          # AST 节点定义
│   │   ├── lexer.h        # 词法分析器 → Token 流
│   │   └── parser.h       # 递归下降解析器 → AST
│   ├── format/            # 格式化引擎
│   │   ├── visitor.h      # AST Visitor，生成格式化输出
│   │   └── math_aligner.h # 表格与数学环境列对齐算法
│   ├── md/                # Markdown 转 LaTeX 转换
│   │   └── md_converter.h # Markdown → LaTeX 转换器
│   ├── utils/             # CLI 工具函数
│   │   ├── io.h           # 文件读写
│   │   ├── diff.h         # unified diff 生成
│   │   └── wrap.h         # 行自动换行
│   └── main.cpp           # CLI 入口
├── tests/
│   └── snapshots/         # 快照测试（.tex 输入 + .expected.tex 预期）
├── scripts/
│   └── setup.sh           # 下载并安装 Catch2
├── external/              # 第三方依赖（gitignore，由脚本生成）
└── CMakeLists.txt
```

## 运行测试

```bash
# 首次运行需安装 Catch2
./scripts/setup.sh

# 构建并运行测试
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest
```

## 技术栈

- **语言**：C++17
- **构建**：CMake 3.16+
- **测试**：Catch2 v3（可选）
- **依赖**：无外部运行时依赖

## 支持的命令与环境

以下列出已注册的所有命令和环境。**未列出的命令不会被修改**，原样保留。

### 命令

#### 文本格式

`\textbf` `\textit` `\emph` `\texttt` `\textsf` `\textrm` `\textsc` `\textup` `\textnormal` — 各 1 个必选参数，支持花括号补全。

#### 数学常用

| 命令 | 参数格式 | 说明 |
|------|------|------|
| `\frac` | `{2}` | 分数 |
| `\sqrt` | `[1]{1}` | 根号（可选根指数） |
| `\binom` | `{2}` | 二项式系数 |

#### 分隔符尺寸

`\left` `\right` — 动态尺寸分隔符，其后接定界符（如 `( [ \{ \langle` 等），需成对出现。

| 命令组 | 示例 | 说明 |
|------|------|------|
| `\bigl` `\Bigl` `\biggl` `\Biggl` | `\bigl( ... \bigr)` | 左侧定尺寸分隔符 |
| `\bigr` `\Bigr` `\biggr` `\Biggr` | `\Bigl[ ... \Bigr]` | 右侧定尺寸分隔符 |
| `\big` `\Big` `\bigg` `\Bigg` | `\big\| ... \big\|` | 中性定尺寸分隔符 |

分隔符可以是：`. [ ] \{ \} ( ) \| \langle \rangle \lfloor \rfloor \lceil \rceil`。

#### 数学修饰

| 命令 | 参数格式 | 说明 |
|------|------|------|
| `\stackrel` `\overset` `\underset` | `{2}` | 上下堆叠 |
| `\substack` | `{1}` | 多行下标 |
| `\text` `\operatorname` | `{1}` | 数学模式中的文本 |
| `\tag` `\tag*` | `{1}` | 公式编号 |

`\notag` `\nonumber` `\limits` `\nolimits` `\displaylimits` `\displaystyle` `\textstyle` `\scriptstyle` `\scriptscriptstyle` `\smash` `\mathclap` `\mathrlap` `\mathllap` `\dd` — 无参数命令。

#### 自定义命令

`\DeclareMathOperator` `{2}`、`\newcommand` `\renewcommand` `\providecommand` `\newenvironment` `\renewenvironment`（参数按独立 Group 解析）、`\ensuremath` `{1}`。

#### 文档结构

`\section` `\subsection` `\subsubsection` `\paragraph` `\subparagraph` `\title` `\author` `\date` — 各 1 个必选参数，支持花括号补全。

#### 交叉引用

`\label` `\ref` `\pageref` `\eqref` `{1}`、`\cite` `[1]{1}`。

#### 包与文档类

`\usepackage` `[1]{1}`、`\documentclass` `[1]{1}`。

#### 浮动体与图形

`\caption` `{1}`、`\includegraphics` `[1]{1}`。

#### 列表

`\item` `[1]`（可选方括号参数）。

#### 无参数命令

`\maketitle` `\tableofcontents` `\newpage` `\clearpage` `\centering` `\raggedright` `\raggedleft` `\noindent` `\indent` `\newline` `\linebreak` `\pagebreak` `\hfill` `\vfill` `\hrulefill` `\dotfill` `\smallskip` `\medskip` `\bigskip` `\par` `\today` `\appendix` `\baselineskip`。

#### 宏包扩展命令

##### caption / subcaption

`\captionsetup` `{1}`、`\subcaption` `{1}`（支持花括号补全）。

##### graphicx / xcolor

`\graphicspath` `{1}`、`\DeclareGraphicsExtensions` `{1}`、`\includesvg` `[1]{1}`、`\color` `[1]{1}`、`\textcolor` `[1]{1}`、`\colorbox` `[1]{1}`、`\pagecolor` `{1}`、`\definecolor` `{3}`。

##### bm

`\bm` `{1}`（支持花括号补全）。

##### mhchem

`\ce` `{1}`（化学式）、`\pu` `{1}`（物理单位）。

##### physics / braket / derivative

| 命令 | 参数格式 | 说明 |
|------|------|------|
| `\abs` `\norm` `\qty` `\order` | `{1}` | 绝对值 / 范数 / 括号 / 阶 |
| `\mqty` | `{1}` | 矩阵 |
| `\eval` `\comm` `\dv` `\pdv` `\odv` `\derivative` | `{2}` | 求值 / 对易子 / 导数 |
| `\ket` `\bra` | `{1}` | 右矢 / 左矢 |
| `\braket` | `{2}` | 内积 |
| `\set` | `{1}` | 集合 |
| `\Bra` `\Ket` | `{1}` | 大尺寸矢 |
| `\dd` | — | 微分算子 |

##### extarrows

`\xleftrightarrow` `\xlongequal` `\xLongrightarrow` `{1}`。

##### gensymb

`\degree` `\celsius` `\micro` `\perthousand` `\ohm`（无参数）。

##### algorithm2e

`\KwIn` `\KwOut` `\KwData` `\KwResult` `\For` `\While` `\If` `\Else` `\ElseIf` `\Return` `\SetKw` `\SetAlgoLined` `\LinesNumbered` `\BlankLine`（无参数）、`\SetKwInOut` `{2}`、`\tcp` `{1}`。

##### enumitem

`\setlist` `{1}`、`\newlist` `{3}`、`\setenumerate` `\setitemize` `\setdescription`（无参数）。

##### lipsum / zhlipsum

`\lipsum` `[1]`（英文盲文）、`\zhlipsum` `[1]`（中文盲文）。

##### Article / Book 文档类

`\part` / `\part*` `[1]{1}` / `{1}`、`\chapter` / `\chapter*` `[1]{1}` / `{1}`、`\thanks` `{1}`、`\frontmatter` `\mainmatter` `\backmatter` `\listoffigures` `\listoftables` `\pagestyle` `\pagenumbering`（无参数）。

##### Beamer 文档类

| 命令 | 参数格式 | 说明 |
|------|------|------|
| `\frametitle` `\framesubtitle` | `{1}` | 帧标题/副标题 |
| `\usetheme` `\usecolortheme` `\usefonttheme` `\useinnertheme` `\useoutertheme` | `{1}` | 主题设置 |
| `\titlegraphic` `\logo` | `{1}` | 标题图/徽标 |
| `\institute` `\alert` `\structure` | `{1}` | 单位/高亮/结构色文字（花括号补全） |
| `\onslide` | `{1}` | 叠层可见范围 |
| `\only` `\uncover` `\visible` `\invisible` | `{2}` | 叠层条件显示 |
| `\pause` `\titlepage` | — | 暂停/标题页 |

#### 表格分隔线

`\hline` `[1]`、`\toprule` `\midrule` `\bottomrule` `\cmidrule` — 表格横线命令，格式化时自动独立成行。

### 环境

#### 数学环境

| 环境 | 对齐策略 | 说明 |
|------|---------|------|
| `align` / `align*` `aligned` `alignedat` `flalign` / `flalign*` `split` | `&` 交替对齐 | 多行公式对齐 |
| `gather` / `gather*` `gathered` `equation` / `equation*` `multline` / `multline*` | 无 | 居中/单行/长公式 |
| `pmatrix` `bmatrix` `Bmatrix` `vmatrix` `Vmatrix` `matrix` `smallmatrix` | 矩阵等宽列 | 各类矩阵 |
| `cases` `dcases` | 分支对齐 | 分段函数 |

#### 列表环境

`itemize`（无序） `enumerate`（有序） `description`（描述）。

#### 原样输出

`verbatim` `lstlisting` `minted` — 内容不做格式化处理。

#### 表格环境

| 环境 | 对齐策略 | 说明 |
|------|---------|------|
| `tabular` `tabularx` `tblr` `longtblr` `talltblr` | `&` 等宽列 + 分隔线独立行 | 表格单元格对齐 |

#### 浮动体与子环境

`figure` / `figure*` `table` / `table*` `subfigure` `subtable`。

#### 算法环境

`algorithm` `function`。

#### Beamer 环境

`frame` `columns` `column` `block` `exampleblock` `alertblock` `overlayarea` `overprint`。

#### Article / Book 环境

`abstract`。

#### 其他环境

`center` `flushleft` `flushright` `quote` `quotation` `tikzpicture`。

## License

「随君所想，成君所愿」
