# klex - A Scanner Generator

- mklex: CLI tool for compiling regular expressions into state transition tables
- libklex: C++ library for lexing

### mklex CLI
```
Usage:
  mklex [-v] -f RULES_FILE -o OUTPUT_FILE

    -f, --file=PATH           source file containing the lexer rules
    -o, --output=PATH         generated header file containing tables and token definitions
        --token-output=PATH   split out token definitions into seperate generated header file.
    -t, --dot=PATH            writes dot graph of final finite automaton. Use - to represent stdout.
    -O, --optimization=NUM    level of optimization for the finite automaton.
                                0 - raw NFA, no tables can be generated.
                                1 - DFA, input rules will be transformed to DFA but not minimized.
                                2 - DFA-minimized, like DFA but also compressed.
```

### klex grammar

```
RuleFile    ::= (SymbolDef* '%%')? RuleDef*
SymbolDef   ::= IDENT SP+ Expression LF
RuleDef     ::= Expression
Expression  ::= <regular expression> LF
SP          ::= (\s\t)+
LF          ::= '\n'
IDENT       ::= [a-zA-Z_][a-zA-Z0-9_]*
```

### libklex API
```!cpp
#include <lexer/lexer.h>
#include <fstream>
#include "myrules.h"

int main(int argc, const char* argv[]) {
  lexer::Lexer lex{myrules, std::ifstream(argv[1])};
  while (lex.recognize() != -1) {
    lex.dump();
  }
}
```
