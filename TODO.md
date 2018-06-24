### Current Work (lookahead)

Given the REs `ab/cd` and `abba`, the input:
- `ab` does not match
- `abc` must not match
- `abba` matches abba
- `abcd` matches and rolls back right behind `ab`

Given the REs `ab/cd` and `cde`, the input:
- `abcd` matches rule 1
- `cde` matches rule 2
- `abcde` matche rule 1, then rule 2

- XXX write unit tests for these things

### The very incomplete TODO checklist

```
text -> RE AST -> NFA -> DFA -> MinDFA -> Tables
```

- [ ] <<more unit tests>>
- [ ] RE: be able to parse "(a|)", which is either "a" or ""
- [ ] RE: invertible character classes ([^abc])
- [ ] RE: support named character classes (see https://en.wikipedia.org/wiki/Regular_expression#Character_classes)
- [ ] FA: create RE pattern out of FA
- [x] RE: double-quoted raw string patterns (just like in flex)
- [x] RE/FA: detect EOL ('$')
- [x] FA: how to handle EOF?
- [x] FA: DFA minimization
- [x] RE: be able to parse character classes, such as: [a-zA-Z]
- [x] RE: support range syntax, such as: a{0,1} as well as a{2}
- [x] FA: dot(), merge character cases into one edge per state when they all go to the same target state
- [x] FA: dot() merge character class ranges, such as a-f shouldn't be a,b,c,d,e,f but a-f on the edge
- [x] ThompsonConstruct: {0, 1}
- [x] ThompsonConstruct: {0, inf}
- [x] ThompsonConstruct: {1, inf}
- [x] ThompsonConstruct: {n, n}
- [x] ThompsonConstruct: {m, n} (m < n)
- [x] FA: rename label() to id() or name(), whatever, but be consistent with other vars reflecting this field
- [x] FA: DFA-minimization also merges accept states, hence, it'll be hard to return the recognized
      token. The idea is to not get accept-states grouped, but give each accept states its own
      partition-set (since they only split and never merge).
      Also, we need to "tag" accept states with the input tag given to `Lexer.declare(tag, RE)`
