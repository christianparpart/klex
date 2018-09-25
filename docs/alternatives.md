klex Alternative Solutions
==========================

Let's create a comparison of scanner and parser generators that target at least C++.

A best match is considered a project that fulfills: the following criteria:
- targets at least C++, but is potentially extensible to other target languages
- active development / maintenance
- software maturity
- separation of concerns with regards to grammar specification and semantic actions
- convinience-first: lexical and syntactical analysis seemlessly integrated

## Alternative Projects

Project          | lexic | syntax | software quality | maintenance | target languages | actions | runtime environment
-----------------|-------|--------|------------------|-------------|------------------|---------|--------------------
[flex](https://github.com/westes/flex)          | yes   | no     | mature           | yes         | C, C++           | inline  | posix
[bison](https://www.gnu.org/software/bison/)    | no    | yes    | mature           | yes         | C, C++           | inline  | posix
bison++                                         | ?     | ?      | ?                | ?           | ?                | ?       | ?
bisonc++                                        | ?     | ?      | ?                | ?           | ?                | ?       | ?
[CppCC](http://cppcc.sourceforge.net/)          | ?     | ?      | ?                | ?           | ?                | ?       | ?
[LRSTAR](https://github.com/VestniK/lrstar)     | ?     | ?      | poor             | ?           | ?                | ?       | ?
[Ragel](http://www.colm.net/open-source/ragel/) | yes   | yes    | mature           | ?           | ?                | ?       | ?
ANTLR4           | ?     | ?      | mature           | ?           | ?                | ?       | ?
[MyParser](https://github.com/hczhcz/myparser)  | ?     | ?      | ?                | ?           | ?                | ?       | ?
JetPAG           | ?     | ?      | ?                | ?           | ?                | ?       | ?
[CSP](http://csparser.sourceforge.net/)         | ?     | ?      | ?                | ?           | ?                | ?       | ?
Dragon           | ?     | ?      | ?                | ?           | ?                | ?       | ?
Yacc++           | ?     | ?      | ?                | ?           | ?                | ?       | ?

### Conclusion

