# mklex cmake integration

function(klex_generate_cpp KLEX_FILE TOKEN_FILE TABLE_FILE)
  set(${TABLE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/${KLEX_FILE}.table.cc")
  set(${TABLE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/${KLEX_FILE}.table.cc" PARENT_SCOPE)
  set(dot_file "${CMAKE_CURRENT_BINARY_DIR}/${KLEX_FILE}.dot")
  set(klex_file "${CMAKE_CURRENT_SOURCE_DIR}/${KLEX_FILE}")

  add_custom_command(
      OUTPUT "${TOKEN_FILE}" "${${TABLE_FILE}}"
      COMMAND mklex -f "${klex_file}" -t "${${TABLE_FILE}}" -T "${TOKEN_FILE}" -x "${dot_file}" -p
      DEPENDS mklex ${klex_file}
      COMMENT "Generating lexer table and tokens for ${KLEX_FILE}"
      VERBATIM)
  set_source_files_properties(${TOKEN_FILE} PROPERTIES GENERATED TRUE)
  set_source_files_properties(${${TABLE_FILE}} PROPERTIES GENERATED TRUE)
endfunction()

