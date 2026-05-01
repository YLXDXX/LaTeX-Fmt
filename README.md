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
| 缩进与换行 | 按 `\begin`/`\end` 层级自动缩进（每层 2 空格） |
| 公式对齐 | `align`、`matrix`、`cases` 等环境按 `&` 纵向对齐 |
| CJK-English 间距 | 中文与英文、数字之间自动添加空格 |
| 注释规范化 | `%comment` → `% comment`；行末注释前保留 1 个空格 |
| 行尾空格清除 | 每行末尾空格全部删除 |
| 空白行处理 | 纯空白行变为空行；连续多空行压缩为 1 个 |
| 行间公式 | `$$...$$` 自动独立成行并缩进 |
| 错误恢复 | 不匹配的花括号、缺失 `\end` 时不会崩溃，尽力恢复 |
| 长行警告 | `--max-line-width=N` 检测超长行，仅警告，不拆行 |

## 快速开始

### 构建（仅需 C++17 和 CMake 3.16+）

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

生成的可执行文件为 `build/latex-fmt`。

### 使用

```bash
# 从标准输入读取，格式化后输出到标准输出
./latex-fmt < input.tex > output.tex

# 启用长行警告（超过 80 字符时发出警告）
./latex-fmt --max-line-width=80 < input.tex > output.tex
```

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
│   ├── core/              # 基础数据结构：AST 节点、源码位置、Unicode 宽度
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

| 命令 | 签名 | 说明 |
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

| 命令 | 签名 | 说明 |
|------|------|------|
| `\frac` | `{2}` | 分数（如 `\frac{1}{2}`） |
| `\sqrt` | `[1]{1}` | 根号（可选次数，如 `\sqrt[3]{x}`） |

#### 文档结构（1 个必选参数，支持花括号补全）

| 命令 | 签名 | 说明 |
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

| 命令 | 签名 | 说明 |
|------|------|------|
| `\label` | `{1}` | 标签 |
| `\ref` | `{1}` | 引用 |
| `\pageref` | `{1}` | 页码引用 |
| `\eqref` | `{1}` | 公式引用 |
| `\cite` | `[1]{1}` | 文献引用 |

#### 包与文档类

| 命令 | 签名 | 说明 |
|------|------|------|
| `\usepackage` | `[1]{1}` | 引入宏包 |
| `\documentclass` | `[1]{1}` | 文档类 |

#### 浮动体与图形

| 命令 | 签名 | 说明 |
|------|------|------|
| `\caption` | `{1}` | 图表标题 |
| `\includegraphics` | `[1]{1}` | 插入图片 |

#### 列表

| 命令 | 签名 | 说明 |
|------|------|------|
| `\item` | `[1]` | 列表项（可选方括号参数） |

#### 无参数命令

`\maketitle` `\tableofcontents` `\newpage` `\clearpage` `\centering`
`\raggedright` `\raggedleft` `\noindent` `\indent` `\newline`
`\linebreak` `\pagebreak` `\hfill` `\vfill` `\hrulefill` `\dotfill`
`\smallskip` `\medskip` `\bigskip` `\par` `\today` `\appendix`
`\baselineskip`

### 环境

#### 数学对齐环境

| 环境 | 对齐策略 | 说明 |
|------|---------|------|
| `align` / `align*` | `&` 交替对齐 | 多行公式对齐 |
| `gather` / `gather*` | 无 | 多行公式居中 |
| `equation` / `equation*` | 无 | 单行公式 |
| `pmatrix` | 矩阵等宽列 | 圆括号矩阵 |
| `bmatrix` | 矩阵等宽列 | 方括号矩阵 |
| `vmatrix` | 矩阵等宽列 | 行列式 |
| `cases` | 分支对齐 | 分段函数 |

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
