" klex syntax highlighting
"

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" # comment LF
" RuleName(option) ::= PATTERN

" Options Section
syn keyword klexTodo contained TODO FIXME XXX NOTE BUG
syn match klexComment "#.*$" contains=klexTodo
syn match klexOptions '^%\s*pragma\>.*$'
syn match klexRuleName '^\s*[a-zA-Z_][a-zA-Z0-9]*'
syn match klexOperator "(\|)"
syn match klexAssign "::="
syn match klexRulePattern /\".*\"/
syn match lexEof "<<EOF>>"

" The default highlighting.
hi def link klexComment       Comment
hi def link klexOperator      Operator
hi def link klexAssign        Operator
hi def link klexTodo          Todo
hi def link klexRuleName      Function
hi def link klexRulePattern   Constant
hi def link klexOptions       PreProc
hi def link klexEof           Special

let b:current_syntax = "klex"

" vim:ts=10
