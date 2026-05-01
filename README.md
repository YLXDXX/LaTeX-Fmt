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
