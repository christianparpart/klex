# klex - A Scanner Generator

- mklex: CLI tool for compiling regular expressions into state transition tables
- libklex: C++ library for lexing

### mklex CLI
```
mklex - klex lexer generator
(c) 2018 Christian Parpart <christian@parpart.family>

 -v, --verbose                Prints some more verbose output
 -h, --help                   Prints this help and exits
 -f, --file=PATTERN_FILE      Input file with lexer rules
 -t, --output-table=FILE      Output file that will contain the compiled tables
 -T, --output-token=FILE      Output file that will contain the compiled tables
 -n, --table-name=IDENTIFIER  Symbol name for generated table. [lexerDef]
 -N, --token-name=IDENTIFIER  Symbol name for generated token enum type. [Token]
 -x, --debug-dfa=DOT_FILE     Writes dot graph of final finite automaton. Use - to
                              represent stdout. []
 -p, --perf                   Print performance counters to stderr.
 -d, --debug-nfa              Print NFA and exit.
```

### Example klex Grammar

```
# specials
Spacing(ignore) ::= "[ \t\s]+"
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

### libklex API

You can compile the above grammar with `klex -f rules.klex -t myrules.h -T mytokens.h` and then compile the code below:

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
