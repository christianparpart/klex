klex - C++ Lexical Scanner Framework
====================================

`klex` is a modern C++ lexical scanner framework that operates as external CLI to generate
the `Lexer`-tables and tokens as well as an embeddable library that can generate and
use lexical grammars on the fly.

Features
--------

- Intuitive input syntax
- Supports most features that the historic projects such as `flex` also have.
- Outputs modern C++17 code.
- Also outputs the token enumeration that can be used by parsers.

klex File Syntax
----------------

The klex file syntax describing the rules is a way different from what one may know
from other tools, such as flex and lex.

- Empty lines will be ignored
- Lines starting with a '#' will be ignored (use that for commenting)

Rules are described as follows:

```
Rule            ::= SP? StardConditions?
                    SP? RuleName
                    SP? ('(' RuleOption ')')?
                    SP? '::=' SP? RuleExpression
                    LF
                    (SP? '|' SP? RuleExpression LF)*
SP              ::= [\s\t]+
LF              ::= [\n]
StartConditions ::= '<' ((TOKEN (',' TOKEN)*) | '*') '>'
RuleName        ::= TOKEN
RuleOption      ::= 'ref' | 'ignore'
RuleExpression  ::= '<<EOF>>'
                  | <a regular expression>
```

Here's a small hello-world example:

```
# ensure that any spaces in between our interesting tokens are getting ignored
Spacing(ignore)   ::= [\s\t\n]+

# our keywords to recognize (all in one)
Keyword           ::= if|then|else

# An ALGOL-like identifier, defined with the help of named character classes
Identifier        ::= [[:alpha:]_][[:alnum:]_]*

# demonstrates the use of ref-rules
DIGIT(ref)        ::= [0-9]
NumberLiteral     ::= {DIGIT}+

# recognize any positive number, allowing helper delimiters, such as 12_123_456_789
CoolNumberLiteral ::= [0-9]+|[0-9]{1,3}(_[0-9]{3})*
```

Start Conditions
----------------

With start conditions it is possible to effectivily implement context sensitive languages
or even multiple protocols within the same lexer.

Stard conditions denote when a rule is going to be visible to the lexer. The lexer can switch
between the standard condition (called `INITIAL`) and any other custom named stard condition.

A rule can also given the condition `*`, which tells the lexer to recognize that rule
in all stard conditions. This is particulary useful for EOF tokens.

Example:

```
# ...
Div             ::= /
RegMatch        ::= =~
<RE>RegExpr     ::= /.*/
<*>Eof          ::= <<EOF>>
# ...
```

Now, when your parser consumes token and recognizes `RegMatch` it knows (by context) that the next
token certainly must not be a `Div` token but more likely a regular expression literal, such as
`RegExpr`. So what it does is, telling the lexer to start recognizing words denoted by
the stard condition `RE`.

Example C++ code:

```
void parseExpr() {
  // ...
  if (currentToken == Token::RegMatch) {
    lexer.setMachine(Machine::RE);
    parseRegExpr();
  }
  // ...
}

void parseRegExpr() {
  // ...
  switch (lexer.recognize()) {
    case Token::RegExpr:
      // handle lexer.word()
      lexer.setMachine(Machine::INITIAL); // switch back to default stard-condition
      break;
    default:
      // syntax error
  }
  // ...
}
```

Ignoring Rules
--------------

There may be tokens that the parser is not interested in, such as spacing between the interesting
tokens. Hence, there must be a way to inform the lexer which tokens to ignore and which not.

To do so, you can set the rule option `ignore`, such as in the following:

```
Spacing(ignore)   ::= [\s\t\n]+
Identifier        ::= [[:alpha:]][[:alnum:]]*
Eof               ::= <<EOF>>
```

Now, considering the input stream `Hello World` will lead to the token stream:

- Identifier (`Hello`)
- Spacing (` `)
- Identifier (`World`)
- Eof

Whereas a consecutive call to `Lexer.recognize()` will only yield:

- Identifier (`Hello`)
- Identifier (`World`)
- Eof

effectively ignoring any spacing.


Referencing Rules
-----------------

Some rules may end up so complex that it is benificial in breaking them down into
helper rules (what klex calls ref-rules) that are not recognized as standalone tokens but
can be used (referenced) by actual tokens to make them easier to describe and more maintainable.

Ref-rules are declared as standard rules with the option `ref` attached to it.
Referencing ref-rules in other non-ref-rules and ref-rules is done by using
the ref-rules name surrounded by curly braces. When a rule's grammar is being parsed,
all occurences of such ref-rule references will be replaced with the ref-rules grammar.

An example would be a token that recognizes an IPv4 or even IPv6 address.
We'll provide the IPv4 example to keep it short:

```
# represents any number between 0 and 255 (an IPv4 octect)
IPv4Octet(ref)    ::= [0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5]

# represents an IPv4 address
IPv4(ref)         ::= {IPv4Octet}(\.{IPv4Octet}){3}

# the actual token to recognize
IPv4Literal       ::= {IPv4}
```

Here, the token to be recognized will have the name `IPv4Literal`. The other two rules will
not be recognized as a token by the lexer but are there to help describing the `IPv4Literal`.

Mind, that the `IPv4Literal` itself simply includes the ref-rule `IPv4`. You could of course
just write the production rule directly, but usually, when you want to recognize IPv4 tokens
you like want to recognize IPv6 tokens too, which is a superset of IPv4 (also lexically).

Alternating Multiline Rules
---------------------------

Some rules may be so complex that they won't fit into a single line without breaking maintainability.
Hence, you can split up your rule into multiple lines, breaking them at the alternation symbol `|`
that must be leading any following lines of the rule.

Any spaces between the alternation symbol `|` and the following regular expression will be trimmed.

Trival example:
```
# alternating regular expression spanning multiple lines
VeryLongRule1  ::= Very
                 | Long
                 | Rule

# equivalent single-line regular expression
VeryLongRule2   ::= Very|Long|Rule
```

Keeping in mind that the example above serves the purpose of demonstrating the capability,
real-world rules (such as IPv6 address patterns) are much more readable this way than
having everything in one line.

Regular Expression Syntax
-------------------------

The following constructs are supported for describing the syntax of a word in `klex`:

- `r|s` (alternation) matches either `r` or `s`
- `rs` (concatenation) matches `r` and then `s`
- `r/s` matches `r` if and only if followed by `s` (without consuming `s`)
- `r$` matches `r` if and only if followed by a line feed (without consuming)
- `r?` either matches `r` or no `r`
- `r*` matches zero or more `r`
- `r+` matches one or more `r`
- `r{n}` matches exactly `n` times `r`
- `r{m,n}` matches `r` at least `m` times and at most `n` times
- `r{m,}` matches `r` at least `m` times
- `"T"` matches uninterpreted literal `T` (may include spaces but no line feeds)
- `[abc]` matches either `a` or `b` or `c`
- `[a-f]` matches one character between `a` and `f`, e.g. `a`, `b`, `c`, `d`, `e`, `f`
- `[a-f0-9]` matches one character between `a` and `f` or `0` and `9`
- `[^abc]` matches one character that is neither `a` nor `b` nor `c`
- `[[:name:]]` matches a named character class; supported named character classes are:
  `alnum`, `alpha`, `blank`, `cntrl`, `digit`, `graph`, `lower`, `print`, `punct`, `space`,
  `upper`, `xdigit`; the meaning directly maps to their counter-parts in the POSIX API, such as
  `isalpha(c)`, `isupper()` etc.
- `(r)` matches `r` - use round braces to override or strengthen operator precedence
` `<<EOF>>` matches the end of input (typically the end of the file, hence EOF)

Matching the beginning of the line (`^`) is not yet supported.
