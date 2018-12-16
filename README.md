# klex - A Scanner Generator
[![Build Status](https://travis-ci.org/christianparpart/klex.svg?branch=master)](https://travis-ci.org/christianparpart/klex) [![Build Status](https://ci.appveyor.com/api/projects/status/l8isxx0k38kdnatq?svg=true)](https://ci.appveyor.com/project/christianparpart/klex) [![Code Coverage](https://codecov.io/gh/christianparpart/klex/branch/master/graph/badge.svg)](https://codecov.io/gh/christianparpart/klex) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/christianparpart/klex.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/christianparpart/klex/context:cpp)



- mklex: CLI tool for compiling regular expressions into state transition tables
- libklex: C++ library for lexing

### mklex CLI
```
mklex - klex lexer generator
(c) 2018 Christian Parpart <christian@parpart.family>

 -v, --verbose                Prints some more verbose output
 -h, --help                   Prints this help and exits
 -f, --file=PATTERN_FILE      Input file with lexer rules
 -t, --output-table=FILE      Output file that will contain the compiled tables (use - to represent stderr)
 -T, --output-token=FILE      Output file that will contain the compiled tables (use - to represent stderr)
 -n, --table-name=IDENTIFIER  Symbol name for generated table (may include namespace). [lexerDef]
 -N, --token-name=IDENTIFIER  Symbol name for generated token enum type (may include namespace). [Token]
 -M, --machine-name=IDENTIFIER
                              Symbol name for generated machine enum type (must not include namespace). [Machine]
 -x, --debug-dfa=DOT_FILE     Writes dot graph of final finite automaton. Use - to represent stdout. []
 -d, --debug-nfa              Writes dot graph of non-deterministic finite automaton to stdout and exits.
     --no-dfa-minimize        Do not minimize the DFA
 -p, --perf                   Print performance counters to stderr.
```

### Example klex Grammar

```
# specials
Spacing(ignore) ::= "[\t\s]+"
Eof             ::= <<EOF>>

# symbols
Plus            ::= \+
RndOpen         ::= \(
RndOpen         ::= \)

# keywords
If              ::= if
Then            ::= then
Else            ::= else

# literals & identifiers
NumberLiteral   ::= 0|[1-9][0-9]*
Identifier      ::= [a-zA-Z_][a-zA-Z0-9_]*
```

### klex Lexer API

The great thing about the Lexer API is, that it is header-only, as the most complex parts are done
at compilation already.

You can compile the above grammar with `klex -f rules.klex -t myrules.h -T mytokens.h`
and then compile the code below:

```cpp
#include <klex/Lexer.h>
#include <fstream>
#include <memory>
#include "myrules.h"
#include "mytokens.h"

int main(int argc, const char* argv[]) {
  klex::Lexer<Token> lexer {lexerDef, std::make_unique<std::ifstream>(argv[1])};

  for (Token t = lexer.recognize(); t != Token::Eof; t = lexer.recognize()) {
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             to_string(t), lexer.word());
  }

  return EXIT_SUCCESS;
}
```

### klex lexer generator API

See [examples/mathexpr.cc](https://github.com/christianparpart/klex/blob/master/examples/mathexpr.cc)
as a great example. Here's a snippet:

```cpp
enum class Token { Eof = 1, Plus, Minus, Mul, Div, RndOpen, RndClose, Number, INVALID };
std::string RULES = R"(
    Space(ignore) ::= [\s\t]+
    Eof           ::= <<EOF>>
    Plus          ::= "+"
    Minus         ::= "-"
    Mul           ::= "*"
    Div           ::= "/"
    RndOpen       ::= "("
    RndClose      ::= \)
    Number        ::= -?([0-9]+|[0-9]{1,3}(_[0-9]{3})*)
    INVALID       ::= .
)";

using Number = long long int;
Number expr(Lexer<Token>& lexer) {
  // [... consume lexer tokens here ...]
  return 42;
}

int main(int argc, const char* argv[]) {
  klex::Compiler cc;
  cc.declareAll(std::make_unique<std::stringstream>(RULES));

  Lexer lexer { cc.compile(), std::make_unique<std::stringstream>("2 + 3 * (5 - 1)") };

  lexer.recognize();      // recognize first token
  Number y = expr(lexer);

  std::cerr << fmt::format("{} = {}\n", input, y);

  return EXIT_SUCCESS;
}
```

### References

- https://swtch.com/~rsc/regexp/
