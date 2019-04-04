
# LL(1) Syntax Analyzer

## Motivations

- Have a convenience-first API for generating and analyzing context free grammars (of type LL(1) and LL(k)).
- Rule rewriting to solve various conflicts or improve power & convenience of the input grammar:
  - Must solve left-recursion by rewriting into right-recursive rules.
  - Must rewrite iterations into set of right-recursive rules.
  - Must support epsilon rules.
- Keep C++20's constexpr changes in mind to allow early adoption of compile-time table constructions.

## klax-Grammar File Format

```
Start               ::= ExplicitTokenGroup? GrammarRule+
ExplicitTokenGroup  ::= 'token' '{' KLEX_TOKEN_GRAMMAR* '}'
GrammarRule         ::= NonTerminal '::=' Handle ('|' Handle)* ';'
NonTerminal         ::= _*[A-Z][a-zA-Z0-9_]*
Terminal            ::= _*[a-z][A-Za-z0-9_]*
                      | "'" ... "'"
                      | '([^'\n]|\\\\')*'|\"([^\"\n]|\\\")*\"
Handle              ::= (Terminal | NonTerminal)*
```

## klax example files

### Expression-Term-Factor

```
token {
  Spacing(ignore) ::= [\s\t\n]+
  Number          ::= 0|[1-9][0-9]*
  Ident           ::= [a-z]+
  Eof             ::= <<EOF>>
}

# NTS     ::= HANDLES            {ACTION_LABELS}

Start     ::= Expr Eof           {expr}
Expr      ::= Expr '+' Term      {addExpr}
            | Expr '-' Term      {subExpr}
            | Term
            ;
Term      ::= Term '*' Factor    {mulExpr}
            | Term '/' Factor    {divExpr}
            | Factor
            ;
Factor    ::= Number             {numberLiteral}
            | Ident              {variable}
            | '(' Expr ')'
            ;
```

```cpp
using namespace std;

klex::ll::Def pd = klex::ll::Compiler{ETF_RULES}.compile();
klex::ll::Analyzer<int> parser{ pd, "2 + 3 * (10 - 6)" };
parser.action("numberLiteral", [](auto& args) { return stoi(args.literal(1)); })
      .action("mulExpr", [](auto const& args) { return args(1) * args(2); })
      .action("divExpr", [](auto const& args) { return args(1) / args(2); })
      .action("addExpr", [](auto const& args) { return args(1) + args(2); })
      .action("subExpr", [](auto const& args) { return args(1) - args(2); });
unique_ptr<Expr> expr = parser.analyze();
```

The parse-table generator needs to rewrite the left-recursion into right-recursion to make
the grammar LL(1) compatible.

# Random Brainstorming Thoughts

```
# should be supportable
AddExpr ::= MulExpr ('+' MulExpr)*

# and automatically rewritten into right-most derivative grammar
AddExpr ::= MulExpr
          | MulExpr '+' AddExpr
          | MulExpr '-' AddExpr

A  -> aX*b
   into
A  -> ab
    | aX'b
X' -> X X'?
```
