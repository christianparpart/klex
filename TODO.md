### The very incomplete TODO checklist

- [ ] FA: create RE pattern out of FA
- [ ] RE: support named character classes (see https://en.wikipedia.org/wiki/Regular_expression#Character_classes)
* [ ] RE: invertible character classes ([^abc])
* [ ] RE: be able to parse "(a|)", which is either "a" or ""
* [ ] FA: rename label() to id() or name(), whatever, but be consistent with other vars reflecting this field
* [x] FA: DFA minimization
* [x] RE: be able to parse character classes, such as: [a-zA-Z]
* [x] RE: support range syntax, such as: a{0,1} as well as a{2}
- [x] FA: dot(), merge character cases into one edge per state when they all go to the same target state
- [x] FA: dot() merge character class ranges, such as a-f shouldn't be a,b,c,d,e,f but a-f on the edge
* [x] ThompsonConstruct: {0, 1}
* [x] ThompsonConstruct: {0, inf}
* [x] ThompsonConstruct: {1, inf}
* [x] ThompsonConstruct: {n, n}
* [x] ThompsonConstruct: {m, n} (m < n)
