" klax syntax highlighting
"

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" # comment LF
" RuleName(option) ::= PATTERN

syn match klaxSpecial display contained "\\\(t\|v\|r\|n\|s\)\||\|\[\|\]\|\.\|+\|*\|?\|(\|)"
"syn region klaxString start=+L\="+ skip=+\\\\\|\\"+ end=+"+ contains=klaxSpecial
syn region klaxString start=/\v"/ skip=/\v\\./ end=/\v"/ contains=klaxSpecial
syn region klaxRawString start="'" end="'"

" Options Section
syn keyword klaxTodo contained TODO FIXME XXX NOTE BUG
syn match klaxComment "#.*$" contains=klaxTodo
syn match klaxOptions '^%\s*pragma\>.*$'
syn match klaxRuleName '^\s*\(<[a-zA-Z,]\+>\)\?[a-zA-Z_][a-zA-Z0-9_]*'
syn match klaxOperator "(\|)\||"
syn match klaxAssign "::="
syn match lexEof "<<EOF>>"

" The default highlighting.
hi def link klaxComment       Comment
hi def link klaxOperator      Operator
hi def link klaxAssign        Operator
hi def link klaxTodo          Todo
hi def link klaxRuleName      Function
hi def link klaxOptions       PreProc
hi def link klaxEof           Special
hi def link klaxString        String
hi def link klaxRawString     String
hi def link klaxSpecial       Special

let b:current_syntax = "klax"

" vim:ts=10

