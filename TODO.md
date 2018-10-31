# REG

- ignore whitespaces in REGEX rules
- `LookaheadLexer<const size_t N=1>`

# CFG

- klex::LeftFactoring
	Rewrites rules to eliminate common prefixes in order to reduce lookahead from k>1 to k=1
- basic actions

# Incomplete TODO items: Lexer

- [ ] proper file offset reporting
- [ ] distinguish between Token ID, TokenTraits, and Token class

# Incomplete TODO list

- [ ] cfg::ll::SyntaxTable::dump() MUST NOT depend on Grammar
- [ ] left-recursion-elimination (direct)
  - call it: struct LeftToRightRecursion {}; that can idealy be used with std::transform()
  - first all left-recursive rules need to be collected
- [ ] left-recursion-elimination (indirect)
- [ ] Analyzer production matching hooks (check ANTLR)

### left-recursion

```
# left
A  ::= A b
     | b;

# right
A  ::= A' b;
A' ::= b A';
     | ;

# LEFT
Expr ::= Expr '+' Term
       | Expr '-' Term
	   | Term;

Expr ::= Term Expr';
Expr' ::= '+' Expr'
       | '-' Expr';
```
