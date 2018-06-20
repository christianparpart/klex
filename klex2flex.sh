#! /bin/bash
# This file is part of the "klex" project, http://github.com/christianparpart/klex>
#   (c) 2018 Christian Parpart <christian@parpart.family>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

set -e

klex_file="$1"
lex_file="out.lex"
table_file="table.cc"
token_file="token.h"
typeName="Token"

echo klex file: ${klex_file}
echo table file: ${table_file}
echo token file: ${token_file}

generate_token_file() {
  awk >${token_file} <"${klex_file}" -f <(echo '
  BEGIN {
    rule_nr = 0;
    printf("#pragma once\n\n");
    printf("#include <cstdlib>      // abort()\n");
    printf("#include <string_view>\n\n");
    printf("enum class Token {\n");
  }

  match($0, /^(\w+)\(ignore\)\s*::=\s*(.*)$/, rule) {
  }

  match($0, /^(\w+)\s*::=\s*(.*)$/, rule) {
    name = rule[1];
    pattern = rule[2];
    rule_nr++;
    printf("  %-20s = %4s, // %s\n", name, rule_nr, pattern);
  }

  END {
    printf("};\n\n"); # end enum
  }
  ')

  awk >>${token_file} <"${klex_file}" -f <(echo "
  BEGIN {
    printf(\"inline constexpr std::string_view to_string(${typeName} t) {\n\");
    printf(\"  switch (t) { \n\");
  }
  match(\$0, /^(\w+)\s*::=\s*(.*)$/, rule) {
    name = rule[1];
    printf(\"    case ${typeName}::%s: return \\\"%s\\\";\n\", name, name);
  }
  END {
    printf(\"    default: abort();\n\");
    printf(\"  }\n\");
    printf(\"}\n\");
  }
  ")
}

generate_table_file() {
  awk >${lex_file} <"${klex_file}" -f <(echo '
  BEGIN {
    rule_nr = 0;
    printf("%%%%\n");
    printf("%%option noyywrap\n");
  }

  match($0, /^(\w+)\(ignore\)\s*::=\s*(.*)$/, rule) {
    name = rule[1];
    pattern = rule[2];
    printf("%-40s { /* %s */ }\n", pattern, name);
  }

  match($0, /^(\w+)\s*::=\s*(.*)$/, rule) {
    name = rule[1];
    pattern = rule[2];
    rule_nr++;
    printf("%-40s { return %d; /* %s */ }\n", pattern, rule_nr, name);
  }')
}

generate_table_file
generate_token_file

flex -t ${lex_file} >${table_file}
