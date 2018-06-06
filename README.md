# FUN INCOMING

this is my playground project to learn how lexer generators work.

- mklex: CLI tool for compiling regular expressions into state transition tables
- libklex: C++ library for lexing
- klexi: testing tool for quick regular expression pattern development and testing

### klexi CLI

```
Usage:
    klexi -f SOURCE_FILE [RULE ...]

    -f, --file=PATH         source file to lexically analyze.
    -t, --dot               prints dot-graph to stdout, suitable for piping into xdot
    -d, --dump              prints tokenized information from the input file to stderr

        RULE ...            regular expression rules to declare in order
```

### mklex CLI
```
Usage:
  mklex [-v] -f RULES_FILE -o OUTPUT_FILE
```

### Rules File Format

```
RuleFile    ::= (SymbolDef* '%%')? RuleDef*
SymbolDef   ::= '{' IDENT '}' SP+ Expression LF
RuleDef     ::= Expression
Expression  ::= <regular expression> LF
SP          ::= (\s\t)+
LF          ::= '\n'

```

```
%token Token
%%
{NUMBER}  0|[1-9][0-9]*
{IDENT}   [a-zA-Z_][a-zA-Z_0-9]*

%%

" \t\n"
"if"
"else"
"{"
"}"
"("
")"
{NUMBER}
{IDENT}
<<EOF>>
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
