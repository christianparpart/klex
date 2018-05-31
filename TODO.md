### The very incomplete TODO checklist

* [ ] FA: DFA minimization
* [ ] FA: rename label() to id() or name(), whatever, but be consistent with other vars reflecting this field
* [ ] RE: be able to parse "(a|)", which is either "a" or ""
* [x] RE: be able to parse character classes, such as: [a-zA-Z]
* [ ] RE: invertible character classes ([^abc])
* [x] RE: support range syntax, such as: a{0,1} as well as a{2}
- [ ] RE: support named character classes (see https://en.wikipedia.org/wiki/Regular_expression#Character_classes)
* [x] ThompsonConstruct: {0, 1}
* [x] ThompsonConstruct: {0, inf}
* [x] ThompsonConstruct: {1, inf}
* [x] ThompsonConstruct: {n, n}
* [x] ThompsonConstruct: {m, n} (m < n)
