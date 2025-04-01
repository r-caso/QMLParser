# QML Parser

A recursive descent parser for the language of Quantified Modal Logic.

This library has been developed as part of my work as a Researcher at [IIF/SADAF/CONICET](https://iif.conicet.gov.ar/?lan=en) and as member of the [Talk Group](https://talk-group.org/).

## Description

The QMLParser library parses the language of Quantified Modal Logic under the following specification of **vocabulary** and **syntax**.

### 1. Vocabulary

The vocabulary of Quantified Modal Logic is composed of:

- **Logical Operators**: ¬, ↔, →, ∧, ∨
- **Modal operators**: □, ⋄
- **Quantifiers**: ∀, ∃
- **Variables**: x, y, z, x1, x2..., y_1, y_2...
- **Predicates**: Walk, Run, etc.
- **Individual constants**: John, Mary, etc.
- **Identity**: =

For convenience, it also includes the **negated existential quantifier** (∄) and the **inequality operator** (≠).

#### 1.1. Non-logical symbols

The non-logical symbols are treated as identifiers that may include any of these symbols:

- a dot (.)
- an underscore (_)
- any alphanumeric character

No constraints are imposed on how these may be combined. That is., "John" and "Mary_1" are valid identifiers, but also are "\_.\_" and "..J.h\_\_on". For your convenience, try to be sensible. The parser, however, won't care.

#### 1.2. Variables

Variables are required to have one of the following formats:

- a single x, y, z character
- a single x, y, z character followed by one or more digits
- a single x, y, z character followed by an underscore, followed by one or more digits

Nothing else will be considered a variable.

As an illustration:

- the following will be recognized as variables: x, y_1, z2
- the following will be treated as non-variable identifiers: x_, y__2, zz2

#### 1.3. Logical Symbols

The lexer recognizes the following logical symbols:

| SYMBOL |     NAME    | CODE POINT | ENCODED BYTES  |
|--------|-------------|------------|----------------|
|   =    | IDENTITY    |   U+003D   | 0x3D           |
|   ¬    | NEGATION    |   U+00AC	| 0xC2 0xAC      |
|   →    | IMPLICATION |   U+2192	| 0xE2 0x86 0x92 |
|   ↔    | EQUIVALENCE |   U+2194	| 0xE2 0x86 0x94 |
|   ∀   | FORALL      |   U+2200	| 0xE2 0x88 0x80 |
|   ∃   | EXISTS      |   U+2203	| 0xE2 0x88 0x83 |
|   ∄    | NOT_EXISTS  |   U+2204	| 0xE2 0x88 0x84 |
|   ∧   | CONJUNCTION |   U+2227	| 0xE2 0x88 0xA7 |
|   ∨   | DISJUNCTION |   U+2228	| 0xE2 0x88 0xA8 |
|   ≠    | INEQUALITY  |   U+2260	| 0xE2 0x89 0xA0 |
|   ⋄    | DIAMOND     |   U+22C4	| 0xE2 0x8B 0x84 |
|   □    | SQUARE      |   U+25A1	| 0xE2 0x96 0xA1 |

### 2. Syntax

The QML Parser implements the following EBNF grammar for QML:

|   NON-TERMINAL   |    |              DEFINITION                  | 
|------------------|----|------------------------------------------|
| equivalence      | =  | implication, {"↔", implication}          |
| implication      | =  | con_dis, {"→", con_dis}                  |
| con_dis          | =  | clause, {"∧", clause}                   |
|                  | \| | clause, {"∨", clause}                   |
| clause           | =  | atomic                                   |
|                  | \| | unary                                    |
|                  | \| | quantificational                         |
|                  | \| | "(", equivalence, ")"                    |
|                  | \| | "[", equivalence, "]"                    |
| quantificational | =  | "∀"\|"∃"\|"∄", variable, clause         |
| unary            | =  | "¬"\|"□"\|"⋄", clause                    | 
| atomic           | =  | predication                              |
|                  | \| | identity                                 |
|                  | \| | inequality                               |
| predication      | =  | identifier, "(", term, {", ", term}, ")" |
| identity         | =  | term, "=", term                          |
| inequality       | =  | term, "≠", term                          |
| term             | =  | identifier\|variable                     |

A few notes on the parser implementation of this grammar:

- the order of operator precedence is implicitly defined as: ↔ \> → \> {∧, ∨} > {¬, □, ⋄, ∀, ∃, ∄}
- when two operators have the same precedence, precedence is left-associative
- order of precedence can be altered by the use of parentheses or backets in the expected way

Also:

- identifiers and variables are characterized in the **Vocabulary** section above
- the start rule for the parser is a ``sentence`` non-terminal, which is an alias for one of the rules above (by default, equivalence) --- see **Usage** for how to set `sentence` as an alias for a different EBNF rule.


## Directory structure

```
/
├── qml-lexer/             # Lexer functionality
│   ├── include/           # Public headers
│   └── src/               # Implementation files
├── qml-parser/            # Parsing functionality
│   ├── include/           # Public headers
│   └── src/               # Implementation files
└── QMLParser/
    └── include/           # convenience header
```

## Build and install

Clone the repository:
```bash
git clone git@github.com:r-caso/QMLParser.git
```

### Prerequisites

- C++23 compatible compiler
- CMake 3.25 or newer
- [QMLExpression](https://github.com/r-caso/QMLExpression) library

Take notice that the QMLParser library requires the [QMLExpression](https://github.com/r-caso/QMLExpression) library. To install it, follow the instructions in the corresponding [README](https://github.com/r-caso/QMLParser/blob/main/README.md) (notice that it need not be installed as a system library).

### Building

Once you have a working installation of QMLExpression, to build the QMLParser library, navigate to the QMLParser root folder, and do the following:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

If QMLExpression is not installed as a system library, you have to tell CMake. Instead of `cmake ..`, do:
```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/QMLExpression
```
Or you can add the following key-value pair in the `cacheVariables` array of your `CMakePresets.json`:
```
"CMAKE_PREFIX_PATH" : "/path/to/QMLExpression"
```

### Installing

To install QMLParser as a system library:
```bash
cmake --install .
```
To install QMLParser to a custom loction (non-system library):
```bash
cmake --install . --prefix /path/to/QMLExpression
```

## Usage

### Including in your project

Once installed, you can use QMLParser in your CMake project in multiple ways:

```cmake
find_package(QMLParser REQUIRED)

# Link to the entire QMLParser interface
target_link_libraries(your_target PRIVATE QMLParser::QMLParser)

# OR link to specific components
target_link_libraries(your_target PRIVATE QMLParser::qml-lexer QMLParser::qml-parser)

# OR link to just one component
target_link_libraries(your_target PRIVATE QMLParser::qml-lexer)
```

If you have installed QMLParser as a non-system library, you should tell CMake where to find it. Use the `-DCMAKE_PREFIX_PATH=/path/to/QMLParser` flag for the command line interface, or the `"CMAKE_PREFIX_PATH" : "/path/to/QMLParser"` under `cacheVariables` in `CMakePresets.json`.

### Lexing and parsing QML formulas

The QMLParser library has several header files, but you will be mainly interested in the following two main components:

- a *lexer* that takes a `std::string` as input and return a `std::vector<Token>` as output, and
- a *parser* that takes a `std::vector<Token>` as input, and returns a `Expression` object as output.

The `Token` class is declared [here](QMLParser/include/token.hpp). The `Expression` objects are provided, as an external dependency, by the [QMLExpression library](https://github.com/r-caso/QMLExpression) (imported as a submodule of the project).

The lexer and parser components can be accessed with the following include directives:
```c++
#include <QMLParser/lexer.hpp>
#include <QMLParser/parser.hpp>
```

The QMLParser library also includes a convenience header that pulls in every function and class:
```c++
#include <QMLParser/QMLParser.hpp>
```

### 1. Invoking the parser

A manual run of the lexer-parser workflow goes as follows:
```c++
namespace QMLExpr = iif_sadaf::talk::QMLExpression;
namespace QMLParser = iif_sadaf::talk::QMLParser;

const std::string formula = "∃x Walk(x)";
const std::vector<QMLParser::Token> tokens = QMLParser::lex(formula);
std::expected<QMLExpr::Expression, std::string> result = QMLParser::Parser(tokens).parse();
```
Alternatively, you can use the convenience function `parse()`:
```c++
const std::string formula = "∃x Walk(x)";
std::expected<QMLExpr::Expression, std::string> result = QMLParser::parse(formula);
```
You can test whether parsing was successful by calling the `has_value()` method of `std::expected`:
```c++
if (result.has_value()) {
    // code to run upon success
    // use result.value() to access the `Expression` object
    // into which the formula was parsed
}
else {
    // code to run upon failure
    // use result.error() to access the error message
    // generated by the parser
}
```

### 2. Parser configuration

The `Parser` class is designed for flexibility, allowing **runtime selection** of both:

1. **The entry point** – the starting function for parsing (e.g., `equivalence`, `implication`, etc.).
2. **The mapping function** – which maps `TokenType` to logical operators, enabling different logical systems.

This makes the parser highly adaptable to different logic systems without modifying its core.

#### 2.1. Selecting the entry point
By default, parsing starts at equivalence, but you can specify any supported entry point, either by manually calling the `QMLParser::parse()` method with an appropriate argument:

```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::Parser(tokens).parse(&QMLParser::Parser::implication);
```
or by calling the `parse()` convenience function with that same argument:
```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::parse(formula, &QMLParser::Parser::implication);
```
The list of rules that may serve as entry point for the parser is provided in [parser.hpp](parser.hpp).

#### 2.2. Customizing the Mapping Function

The mapping function determines how `TokenType` values map to logical operators (`QML::Operator`). This allows different logical systems (e.g., modal logic, deontic logic) to reuse the same parser. For example:

- if you use a map to deontic modal logic, the necessity operator □ will be mapped to a deontic necessity modal operator (`QML::Operator::DEONTIC_NECESSITY`), and the possibility operator ⋄ will be mapped to a deontic possibility operator (`QML::Operator::DEONTIC_POSSIBILITY`);
- if you use a map to epistemic modal logic, the necessity operator □ will be mapped to a deontic necessity modal operator (`QML::Operator::EPISTEMIC_NECESSITY`), and the possibility operator ⋄ will be mapped to a deontic possibility operator (`QML::Operator::EPISTEMIC_POSSIBILITY`).

These different mappings will result in `Expression` objects with modal operators of different flavors.

By default, the QMLParser maps □ to `QML::Operator::NECESSITY`, and ⋄ to `QML::Operator::POSSIBILITY`, but this can be changed by explicitly providing a different mapping function:
```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::Parser(tokens, QMLParser::mapToDeonticOperator).parse();
```
or with the convenience `parse()` function:
```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::parse(formula, QMLParser::mapToDeonticOperator);
```
Obviously, you can customize in both ways at the same time:
```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::Parser(tokens, QMLParser::mapToDeonticOperator).parse(&QMLParser::Parser::implication);
```

```c++
std::expected<QMLExpr::Expression, std::string> result = QMLParser::parse(formula, &QMLParser::Parser::implication, QMLParser::mapToEpistemicOperator);
```

## Contributing

Contributions are more than welcome. If you want to contribute, please do the following:

1. Fork the repository.
2. Create a new branch: `git checkout -b feature-name`.
3. Make your changes.
4. Push your branch: `git push origin feature-name`.
5. Create a pull request.

## License
This project is licensed under the [BSD-3-Clause](LICENSE).