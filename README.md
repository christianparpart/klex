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
If            ::= if
Then          ::= then
Else          ::= else
NumberLiteral ::= 0|[1-9][0-9]*
Identifier    ::= [a-zA-Z_][a-zA-Z0-9_]*
Plus          ::= \+
RndOpen       ::= \(
RndOpen       ::= \)
```

### libklex API
```!cpp
#include <lexer/lexer.h>
#include <fstream>
#include "myrules.h"
#include "mytokens.h"

int main(int argc, const char* argv[]) {
  lexer::Lexer lex{myrules, std::ifstream(argv[1])};
  while (lex.recognize() != Token::Eof) {
    lex.dump();
  }
}
```
