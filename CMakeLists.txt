cmake_minimum_required(VERSION 3.6)
project(klex VERSION "0.0.0" LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

option(KLEX_EMBEDDED_FMTLIB "Build against embedded fmtlib [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_EXAMPLES "Build examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})

# fmtlib
if(KLEX_EMBEDDED_FMTLIB)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fmt" EXCLUDE_FROM_ALL)
  add_definitions(-DFMT_USE_WINDOWS_H=0)
else()
  # master project must provide its own fmtlib
endif()

# ----------------------------------------------------------------------------
add_definitions(-Wall)
add_definitions(-pedantic)

# ----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/klex/sysconfig.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/src/klex/sysconfig.h)

# ----------------------------------------------------------------------------
add_executable(klex_test
    src/klex/util/AnsiColor.cc
    src/klex/util/testing.cc
    src/klex/klex_test.cc
    )

target_link_libraries(klex_test PUBLIC klex)
target_link_libraries(klex_test PUBLIC fmt::fmt-header-only stdc++fs)
set_target_properties(klex_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(CLANG_TIDY_EXE)
  set_target_properties(klex_test PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()

# ----------------------------------------------------------------------------
add_library(klex STATIC
    src/klex/Alphabet.cc
    src/klex/Compiler.cc
    src/klex/DFA.cc
    src/klex/DFABuilder.cc
    src/klex/DFAMinimizer.cc
    src/klex/DotWriter.cc
    src/klex/Lexer.cc
    src/klex/NFA.cc
    src/klex/NFABuilder.cc
    src/klex/RegExpr.cc
    src/klex/RuleParser.cc
    src/klex/State.cc
    src/klex/TransitionMap.cc
    src/klex/util/Flags.cc
    )

target_link_libraries(klex PUBLIC fmt::fmt-header-only stdc++fs)
set_target_properties(klex PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(CLANG_TIDY_EXE)
  set_target_properties(klex PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()

install(TARGETS klex DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/klex/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include/klex"
    FILES_MATCHING PATTERN "*.h")

target_include_directories(klex PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>)

# ----------------------------------------------------------------------------
add_executable(mklex src/mklex.cc)
set_target_properties(mklex PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(mklex klex)

# ----------------------------------------------------------------------------
function(klex_generate_cpp KLEX_FILE TOKEN_FILE TABLE_FILE)
  set(${TABLE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/${KLEX_FILE}.table.cc")
  set(${TABLE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/${KLEX_FILE}.table.cc" PARENT_SCOPE)

  set(klex_file "${CMAKE_CURRENT_SOURCE_DIR}/${KLEX_FILE}")

  message(STATUS "klex_generate_cpp: klex file: ${klex_file}")
  message(STATUS "klex_generate_cpp: token out: ${TOKEN_FILE}")
  message(STATUS "klex_generate_cpp: table out: ${TABLE_FILE} ${${TABLE_FILE}}")

  add_custom_command(
      OUTPUT "${TOKEN_FILE}" "${${TABLE_FILE}}"
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/mklex
      ARGS -f "${klex_file}" -t "${${TABLE_FILE}}" -T "${TOKEN_FILE}" -p
      DEPENDS mklex ${klex_file}
      COMMENT "Generating lexer table and tokens for ${KLEX_FILE}"
      VERBATIM)
  set_source_files_properties(${TOKEN_FILE} PROPERTIES GENERATED TRUE)
  set_source_files_properties(${${TABLE_FILE}} PROPERTIES GENERATED TRUE)
endfunction()

# ----------------------------------------------------------------------------
if(KLEX_EXAMPLES)
  set(FLOW_TOKEN_SRC "${CMAKE_CURRENT_BINARY_DIR}/examples/token.h")
  klex_generate_cpp(examples/flow.klex ${FLOW_TOKEN_SRC} FLOW_TABLE_SRC)
  message(STATUS "example: KLEX_TABLE_SRC: ${FLOW_TABLE_SRC}")

  add_executable(example_flowlexer examples/flowlexer.cc ${FLOW_TABLE_SRC})
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_flowlexer klex)
  target_include_directories(example_flowlexer PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
  add_dependencies(example_flowlexer mklex)
endif(KLEX_EXAMPLES)
