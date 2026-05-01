# LaTeX-Fmt

一个命令行 LaTeX 代码格式化工具，实现代码的**美化打印**和**规范化**。

## 核心原则

**绝不能因为格式化导致原本能编译的 LaTeX 代码无法编译。**
任何拿不准的情况，宁可原样保留，绝不乱改。

## 功能特性

| 规则 | 说明 |
|------|------|
| 数学定界符统一 | `\(...\)` → `$...$`，`\[...\]` → `$$...$$` |
| 花括号补全 | `\frac12` → `\frac{1}{2}`（仅限已知命令） |
| 去冗余空白 | 文本模式多个空格压缩为 1 个；数学模式保留空格 |
| 缩进与换行 | 按 `\begin`/`\end` 层级自动缩进（默认每层 2 空格） |
| 公式对齐 | `align`、`matrix`、`cases` 等环境按 `&` 纵向对齐 |
| CJK-English 间距 | 中文与英文、数字之间自动添加空格 |
| 注释规范化 | `%comment` → `% comment`；行末注释前保留 1 个空格 |
| 行尾空格清除 | 每行末尾空格全部删除 |
| 空白行处理 | 纯空白行变为空行；连续多空行压缩为 1 个 |
| 行间公式 | `$$...$$` 自动独立成行并缩进 |
| 错误恢复 | 不匹配的花括号、缺失 `\end` 时不会崩溃，尽力恢复 |
| 长行警告与自动换行 | `--max-line-width=N` 警告超长行；`--wrap` / `--wrap-paragraphs` 自动折行 |

### 可选特性

以下格式化规则可通过命令行参数或配置文件单独关闭/开启：

| 参数 | 说明 |
|------|------|
| `--indent-width=N` | 设置缩进宽度（默认 2） |
| `--no-cjk-spacing` / `--cjk-spacing` | 关闭/开启 CJK-ASCII 自动空格 |
| `--no-brace-completion` / `--brace-completion` | 关闭/开启花括号补全 |
| `--no-comment-normalize` / `--comment-normalize` | 关闭/开启注释规范化 |
| `--no-blank-line-compress` / `--blank-line-compress` | 关闭/开启空白行压缩 |
| `--keep-trailing-spaces` / `--remove-trailing-spaces` | 保留/删除行尾空格 |
| `--no-display-math-format` / `--display-math-format` | 关闭/开启行间公式独立格式化 |
| `--no-math-unify` / `--math-unify` | 关闭/开启数学定界符统一 |
| `--wrap` | 在单词边界自动折行（需配合 `--max-line-width=N`） |
| `--wrap-paragraphs` | 对纯文本段落自动折行 |

## 快速开始

### 构建（仅需 C++17 和 CMake 3.16+）

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

生成的可执行文件为 `build/latex-fmt`。

### 安装

```bash
# 安装到系统目录（默认 /usr/local/bin）
cmake --install build
# 指定安装路径
cmake --install build --prefix /usr/local
```

### 使用

```bash
# 完整命令行参考
latex-fmt --help
```

#### 基本用法

```bash
# 从标准输入读取，格式化后输出到标准输出
latex-fmt < input.tex > output.tex

# 直接格式化文件，输出到标准输出
latex-fmt paper.tex

# 原地格式化文件
latex-fmt -i paper.tex

# 格式化后输出到指定文件
latex-fmt -o out.tex paper.tex
```

#### 检查与 diff 模式

```bash
# 检查是否需要格式化（CI 用，如需格式化则退出码为 1）
latex-fmt --check paper.tex

# 显示统一 diff 格式的差异
latex-fmt --diff paper.tex

# 递归检查目录下所有 .tex 文件
latex-fmt -r --check project/
```

#### 格式化参数

```bash
# 缩进宽度设为 4
latex-fmt --indent-width=4 paper.tex

# 关闭 CJK-ASCII 自动空格
latex-fmt --no-cjk-spacing paper.tex

# 关闭花括号补全
latex-fmt --no-brace-completion paper.tex

# 长行自动折行（行宽 80）
latex-fmt --max-line-width=80 --wrap-paragraphs paper.tex

# 组合使用多个参数
latex-fmt --indent-width=4 --no-cjk-spacing --quiet paper.tex
```

#### 递归处理

```bash
# 递归格式化目录
latex-fmt -r -i project/

# 递归检查目录
latex-fmt -r --check project/

# 递归显示 diff
latex-fmt -r --diff project/
```

#### 其他参数

```bash
# 抑制警告输出
latex-fmt --quiet paper.tex

# 使用自定义配置文件
latex-fmt --config-file=/path/to/.latexfmtrc paper.tex
```

### 配置文件

latex-fmt 支持 `.latexfmtrc` 配置文件，按以下优先级查找（后找到的覆盖先找到的）：

1. `~/.latexfmtrc`（全局配置）
2. `./.latexfmtrc`（当前目录配置）
3. 通过 `--config-file=<path>` 指定的文件

配置文件格式为 `key = value`，每行一对。`#` 开头的行为注释。

```ini
# .latexfmtrc 示例
indent_width = 4
max_line_width = 80
cjk_spacing = false
wrap_paragraphs = true

# 布尔值支持: true, 1, yes, on / false, 0, no, off
brace_completion = true
comment_normalize = true
```

命令行参数始终覆盖配置文件中的设置。

### 运行测试（可选，需 Catch2）

```bash
# 首次运行需安装 Catch2
./scripts/setup.sh

# 构建并运行测试
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest
```

## 项目结构

```text
latex-fmt/
├── src/
│   ├── core/              # 基础数据结构
│   │   ├── config.h       # FormatConfig 格式化配置
│   │   ├── registry.h     # 命令/环境签名注册表
│   │   ├── source_location.h
│   │   └── unicode_width.h
│   ├── parse/             # 词法分析、语法分析
│   │   ├── ast.h          # 抽象语法树定义
│   │   ├── lexer.h        # 词法分析器（Token 流）
│   │   └── parser.h       # 递归下降解析器（Token → AST）
│   ├── format/            # 格式化引擎
│   │   ├── visitor.h      # Visitor 模式格式化输出
│   │   └── math_aligner.h # 数学环境对齐算法
│   └── main.cpp           # 入口
├── tests/                 # 测试用例与快照
│   └── snapshots/         # .tex 输入 + .expected.tex 预期输出
├── scripts/
│   └── setup.sh           # 下载并安装 Catch2
├── external/              # 第三方依赖（gitignore，由脚本生成）
└── CMakeLists.txt
```

## 技术栈

- **语言**: C++17
- **构建**: CMake 3.16+
- **测试**: Catch2 v3.14.0（可选）
- **无外部运行时依赖**：格式化器本身不依赖任何第三方库

## License

MIT

## 支持的命令与环境

以下列出当前 `Registry` 中已注册的所有命令和环境。**未在此列表中的命令不会被修改**，原样保留。

### 命令

#### 文本格式（1 个必选参数，支持花括号补全）

| 命令 | 参数 | 说明 |
|------|------|------|
| `\textbf` | `{1}` | 粗体 |
| `\textit` | `{1}` | 斜体 |
| `\emph` | `{1}` | 强调 |
| `\texttt` | `{1}` | 等宽字体 |
| `\textsf` | `{1}` | 无衬线字体 |
| `\textrm` | `{1}` | 衬线字体 |
| `\textsc` | `{1}` | 小型大写 |
| `\textup` | `{1}` | 直立字体 |
| `\textnormal` | `{1}` | 文档默认字体 |

#### 数学（支持花括号补全）

| 命令 | 参数 | 说明 |
|------|------|------|
| `\frac` | `{2}` | 分数（如 `\frac{1}{2}`） |
| `\sqrt` | `[1]{1}` | 根号（可选次数，如 `\sqrt[3]{x}`） |
| `\binom` | `{2}` | 二项式系数 |

#### 数学修饰命令

| 命令 | 参数 | 说明 |
|------|------|------|
| `\stackrel` | `{2}` | 堆叠符号（上/下） |
| `\overset` | `{2}` | 上方堆叠 |
| `\underset` | `{2}` | 下方堆叠 |
| `\substack` | `{1}` | 多行下标（`\\` 分隔） |
| `\text` | `{1}` | 数学模式中的文本 |
| `\operatorname` | `{1}` | 自定义运算符名 |
| `\tag` / `\tag*` | `{1}` | 公式编号 |
| `\notag` / `\nonumber` | — | 取消公式编号 |
| `\limits` / `\nolimits` / `\displaylimits` | — | 极限位置控制 |
| `\displaystyle` / `\textstyle` / `\scriptstyle` / `\scriptscriptstyle` | — | 数学样式 |
| `\smash` / `\mathclap` / `\mathrlap` / `\mathllap` | — | 边界盒控制 |
| `\dd` | — | 微分算子 d |

#### 自定义命令与声明

| 命令 | 参数 | 说明 |
|------|------|------|
| `\DeclareMathOperator` | `{2}` | 声明数学运算符 |
| `\newcommand` / `\renewcommand` / `\providecommand` | — | 自定义命令（参数作为独立 Group 节点解析） |
| `\newenvironment` / `\renewenvironment` | — | 自定义环境 |
| `\ensuremath` | `{1}` | 确保数学模式（支持花括号补全） |

#### 文档结构（1 个必选参数，支持花括号补全）

| 命令 | 参数 | 说明 |
|------|------|------|
| `\section` | `{1}` | 一级节 |
| `\subsection` | `{1}` | 二级节 |
| `\subsubsection` | `{1}` | 三级节 |
| `\paragraph` | `{1}` | 段落 |
| `\subparagraph` | `{1}` | 子段落 |
| `\title` | `{1}` | 标题 |
| `\author` | `{1}` | 作者 |
| `\date` | `{1}` | 日期 |

#### 交叉引用与引用

| 命令 | 参数 | 说明 |
|------|------|------|
| `\label` | `{1}` | 标签 |
| `\ref` | `{1}` | 引用 |
| `\pageref` | `{1}` | 页码引用 |
| `\eqref` | `{1}` | 公式引用 |
| `\cite` | `[1]{1}` | 文献引用 |

#### 包与文档类

| 命令 | 参数 | 说明 |
|------|------|------|
| `\usepackage` | `[1]{1}` | 引入宏包 |
| `\documentclass` | `[1]{1}` | 文档类 |

#### 浮动体与图形

| 命令 | 参数 | 说明 |
|------|------|------|
| `\caption` | `{1}` | 图表标题 |
| `\includegraphics` | `[1]{1}` | 插入图片 |

#### 列表

| 命令 | 参数 | 说明 |
|------|------|------|
| `\item` | `[1]` | 列表项（可选方括号参数） |

#### 无参数命令

`\maketitle` `\tableofcontents` `\newpage` `\clearpage` `\centering`
`\raggedright` `\raggedleft` `\noindent` `\indent` `\newline`
`\linebreak` `\pagebreak` `\hfill` `\vfill` `\hrulefill` `\dotfill`
`\smallskip` `\medskip` `\bigskip` `\par` `\today` `\appendix`
`\baselineskip`

#### 宏包命令

以下按宏包分组列出额外支持的命令。参数中 `{n}` 表示 n 个必选参数，`[n]` 表示 n 个可选参数。

##### caption / subcaption

| 命令 | 参数 | 说明 |
|------|------|------|
| `\captionsetup` | `{1}` | 全局配置图表标题样式 |
| `\subcaption` | `{1}` | 子图/子表标题（支持花括号补全） |

##### graphicx / svg / xcolor

| 命令 | 参数 | 说明 |
|------|------|------|
| `\graphicspath` | `{1}` | 设置图片搜索路径 |
| `\DeclareGraphicsExtensions` | `{1}` | 设置图片扩展名 |
| `\includesvg` | `[1]{1}` | 插入 SVG 图片 |
| `\color` | `[1]{1}` | 设置文字颜色 |
| `\textcolor` | `[1]{1}` | 带颜色的文字 |
| `\colorbox` | `[1]{1}` | 带底色的盒子 |
| `\pagecolor` | `{1}` | 设置页面背景色 |
| `\definecolor` | `{3}` | 定义颜色名 |

##### bm

| 命令 | 参数 | 说明 |
|------|------|------|
| `\bm` | `{1}` | 数学粗体（支持花括号补全） |

##### mhchem

| 命令 | 参数 | 说明 |
|------|------|------|
| `\ce` | `{1}` | 化学式/方程式 |
| `\pu` | `{1}` | 物理单位 |

##### physics / braket / derivative

| 命令 | 参数 | 说明 |
|------|------|------|
| `\abs` `\norm` `\qty` `\order` | `{1}` | 绝对值 / 范数 / 括号 / 阶 |
| `\mqty` | `{1}` | 矩阵 |
| `\eval` `\comm` `\dv` `\pdv` `\odv` `\derivative` | `{2}` | 求值 / 对易子 / 全导数 / 偏导数 / 常导数 |
| `\ket` `\bra` | `{1}` | 右矢 / 左矢 |
| `\braket` | `{2}` | 内积 |
| `\set` | `{1}` | 集合 |
| `\Bra` `\Ket` | `{1}` | 大尺寸左/右矢 |
| `\dd` | — | 微分算子 d |

##### extarrows

| 命令 | 参数 | 说明 |
|------|------|------|
| `\xleftrightarrow` | `{1}` | 可扩展的双向箭头 |
| `\xlongequal` | `{1}` | 可扩展的等号 |
| `\xLongrightarrow` | `{1}` | 可扩展的单向箭头 |

##### gensymb

`\degree` `\celsius` `\micro` `\perthousand` `\ohm`（均为无参数命令）

##### algorithm2e

| 命令 | 参数 | 说明 |
|------|------|------|
| `\KwIn` `\KwOut` `\KwData` `\KwResult` | — | 输入/输出/数据/结果关键字 |
| `\For` `\While` `\If` `\Else` `\ElseIf` `\Return` | — | 控制流关键字 |
| `\SetKw` `\SetAlgoLined` `\LinesNumbered` `\BlankLine` | — | 样式配置 / 空行 |
| `\SetKwInOut` | `{2}` | 自定义输入输出关键字 |
| `\tcp` | `{1}` | 行末注释 |

##### enumitem

| 命令 | 参数 | 说明 |
|------|------|------|
| `\setlist` | `{1}` | 全局列表配置 |
| `\newlist` | `{3}` | 创建新列表环境 |
| `\setenumerate` `\setitemize` `\setdescription` | — | 列表配置 |

##### lipsum / zhlipsum

| 命令 | 参数 | 说明 |
|------|------|------|
| `\lipsum` | `[1]` | 生成英文盲文 |
| `\zhlipsum` | `[1]` | 生成中文盲文 |

##### Article / Book 文档类

| 命令 | 参数 | 说明 |
|------|------|------|
| `\part` / `\part*` | `[1]{1}` / `{1}` | 部 |
| `\chapter` / `\chapter*` | `[1]{1}` / `{1}` | 章 |
| `\thanks` | `{1}` | 标题页脚注 |
| `\frontmatter` `\mainmatter` `\backmatter` | — | Book 前/主/后文 |
| `\listoffigures` `\listoftables` | — | 图/表目录 |
| `\pagestyle` `\pagenumbering` | — | 页面风格和编号 |

##### Beamer 文档类

| 命令 | 参数 | 说明 |
|------|------|------|
| `\frametitle` | `{1}` | 帧标题 |
| `\framesubtitle` | `{1}` | 帧副标题 |
| `\usetheme` `\usecolortheme` `\usefonttheme` `\useinnertheme` `\useoutertheme` | `{1}` | 主题设置 |
| `\titlegraphic` `\logo` | `{1}` | 标题图 / 徽标 |
| `\institute` | `{1}` | 单位（支持花括号补全） |
| `\alert` `\structure` | `{1}` | 高亮 / 结构色文字（支持花括号补全） |
| `\onslide` | `{1}` | 叠层可见范围 |
| `\only` `\uncover` `\visible` `\invisible` | `{2}` | 叠层条件显示 |
| `\pause` `\titlepage` | — | 暂停 / 标题页 |

### 环境

#### 数学对齐环境

| 环境 | 对齐策略 | 说明 |
|------|---------|------|
| `align` / `align*` | `&` 交替对齐 | 多行公式对齐 |
| `aligned` | `&` 交替对齐 | 子公式块（需嵌套） |
| `alignedat` | `&` 交替对齐 | 定列数对齐块 |
| `flalign` / `flalign*` | `&` 交替对齐 | 满幅对齐 |
| `gather` / `gather*` | 无 | 多行公式居中 |
| `gathered` | 无 | 居中子公式块 |
| `split` | `&` 交替对齐 | 单公式多行拆分 |
| `equation` / `equation*` | 无 | 单行公式 |
| `multline` / `multline*` | 无 | 长公式（首行左、末行右） |
| `pmatrix` | 矩阵等宽列 | 圆括号矩阵 |
| `bmatrix` | 矩阵等宽列 | 方括号矩阵 |
| `Bmatrix` | 矩阵等宽列 | 花括号矩阵 |
| `vmatrix` | 矩阵等宽列 | 行列式（单竖线） |
| `Vmatrix` | 矩阵等宽列 | 行列式（双竖线） |
| `matrix` | 矩阵等宽列 | 无括号矩阵 |
| `smallmatrix` | 矩阵等宽列 | 小型矩阵（行内） |
| `cases` | 分支对齐 | 分段函数 |
| `dcases` | 分支对齐 | displaystyle 分段函数 |

#### 列表环境

| 环境 | 说明 |
|------|------|
| `itemize` | 无序列表 |
| `enumerate` | 有序列表 |
| `description` | 描述列表 |

#### 原样输出环境

| 环境 | 说明 |
|------|------|
| `verbatim` | 逐字输出 |
| `lstlisting` | 代码列表 |
| `minted` | 语法高亮代码 |

#### 表格环境

| 环境 | 说明 |
|------|------|
| `tabular` | 表格 |
| `tabularx` | 定宽表格 |
| `tblr` | tabularray 表格 |
| `longtblr` | tabularray 长表格 |
| `talltblr` | tabularray 高表格 |

#### 浮动体子环境

| 环境 | 说明 |
|------|------|
| `subfigure` | 子图 |
| `subtable` | 子表 |

#### 算法环境

| 环境 | 说明 |
|------|------|
| `algorithm` | 算法浮动体 |
| `function` | 函数定义 |

#### Beamer 环境

| 环境 | 说明 |
|------|------|
| `frame` | 幻灯片帧 |
| `columns` | 多列布局 |
| `column` | 单列 |
| `block` | 普通块 |
| `exampleblock` | 示例块 |
| `alertblock` | 警告块 |
| `overlayarea` | 叠层区域 |
| `overprint` | 叠层切换 |

#### Article / Book 环境

| 环境 | 说明 |
|------|------|
| `abstract` | 摘要 |

#### 其他环境

| 环境 | 说明 |
|------|------|
| `figure` / `figure*` | 浮动图片 |
| `table` / `table*` | 浮动表格 |
| `center` | 居中 |
| `flushleft` | 左对齐 |
| `flushright` | 右对齐 |
| `quote` | 短引用 |
| `quotation` | 长引用 |
| `tikzpicture` | TikZ 绘图 |
