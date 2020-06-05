cmake_minimum_required(VERSION 3.6)
project(klex VERSION "0.0.0" LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(mklex)
include(ClangTidy)
include(EnableCcache)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

option(KLEX_EMBEDDED_FMTLIB "Build against embedded fmtlib [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_EXAMPLES "Build examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(KLEX_TESTS "Build test suite [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})
option(MKLEX_LINK_STATIC "Build mklex as staticaly linked binary [default: OFF]" OFF)
option(KLEX_COVERAGE "Builds with codecov [default: OFF]" OFF)

if(KLEX_COVERAGE)
  add_compile_options(-g --coverage)
  set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
endif()

# fmtlib
if(KLEX_EMBEDDED_FMTLIB)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fmt" EXCLUDE_FROM_ALL)
  add_definitions(-DFMT_USE_WINDOWS_H=0)
else()
  # master project must provide its own fmtlib
endif()

# ----------------------------------------------------------------------------
if(NOT MSVC)
  add_definitions(-Wall)
  add_definitions(-pedantic)
endif()

# ----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/klex/sysconfig.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/src/klex/sysconfig.h)

# pkg-config support
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/klex.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/klex.pc
               @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/klex.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

# ----------------------------------------------------------------------------
add_library(klex STATIC
    src/klex/Report.cc
    src/klex/SourceLocation.cc
    src/klex/cfg/Grammar.cc
    src/klex/cfg/GrammarLexer.cc
    src/klex/cfg/GrammarParser.cc
    src/klex/cfg/GrammarValidator.cc
    src/klex/cfg/LeftRecursion.cc
    src/klex/cfg/ll/SyntaxTable.cc
    src/klex/regular/Alphabet.cc
    src/klex/regular/Compiler.cc
    src/klex/regular/DFA.cc
    src/klex/regular/DFABuilder.cc
    src/klex/regular/DFAMinimizer.cc
    src/klex/regular/DotWriter.cc
    src/klex/regular/MultiDFA.cc
    src/klex/regular/NFA.cc
    src/klex/regular/NFABuilder.cc
    src/klex/regular/RegExpr.cc
    src/klex/regular/RegExprParser.cc
    src/klex/regular/RuleParser.cc
    src/klex/regular/State.cc
    src/klex/regular/Symbols.cc
    src/klex/util/Flags.cc
    )

target_link_libraries(klex PUBLIC fmt::fmt-header-only)
if(MSVC)
  target_link_libraries(klex PUBLIC Shlwapi)
else()
  target_link_libraries(klex PUBLIC stdc++fs)
endif()
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

if(MKLEX_LINK_STATIC AND UNIX AND NOT APPLE)
  set_target_properties(mklex PROPERTIES
                        LINK_FLAGS -static
                        LINK_SEARCH_START_STATIC ON
                        LINK_SEARCH_END_STATIC ON)
endif()

# ----------------------------------------------------------------------------
if(KLEX_TESTS)
  add_executable(klex_test
      src/klex/cfg/GrammarLexer_test.cc
      src/klex/cfg/GrammarParser_test.cc
      src/klex/cfg/Grammar_test.cc
      src/klex/cfg/LeftRecursion_test.cc
      src/klex/cfg/ll/Analyzer_test.cc
      src/klex/cfg/ll/SyntaxTable_test.cc
      src/klex/klex_test.cc
      src/klex/regular/DFABuilder_test.cc
      src/klex/regular/DotWriter_test.cc
      src/klex/regular/Lexer_test.cc
      src/klex/regular/NFA_test.cc
      src/klex/regular/RegExprParser_test.cc
      src/klex/regular/RuleParser_test.cc
      src/klex/regular/State_test.cc
      src/klex/regular/Symbols_test.cc
      src/klex/util/iterator_test.cc
      src/klex/util/testing.cc
      )

  target_link_libraries(klex_test PUBLIC klex)
  target_link_libraries(klex_test PUBLIC fmt::fmt-header-only)
  set_target_properties(klex_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  if(CLANG_TIDY_EXE)
    set_target_properties(klex_test PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endif(KLEX_TESTS)

# ----------------------------------------------------------------------------
if(KLEX_EXAMPLES)
  set(FLOW_TOKEN_SRC "${CMAKE_CURRENT_BINARY_DIR}/examples/token.h")
  klex_generate_cpp(examples/flow.klex ${FLOW_TOKEN_SRC} FLOW_TABLE_SRC)
  message(STATUS "example: KLEX_TABLE_SRC: ${FLOW_TABLE_SRC}")

  add_executable(example_flowlexer examples/flowlexer.cc ${FLOW_TABLE_SRC})
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_include_directories(example_flowlexer PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
  target_link_libraries(example_flowlexer PUBLIC fmt::fmt-header-only)
  set_target_properties(example_flowlexer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

  add_executable(example_mathexpr examples/mathexpr.cc)
  set_target_properties(example_mathexpr PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_mathexpr klex)
  target_include_directories(example_mathexpr PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)

  add_executable(example_mathexpr_ll examples/mathexpr_ll.cc)
  set_target_properties(example_mathexpr_ll PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_mathexpr_ll klex)
  target_include_directories(example_mathexpr_ll PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)

  add_executable(example_wordcount examples/wordcount.cc)
  set_target_properties(example_wordcount PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(example_wordcount klex)
  target_include_directories(example_wordcount PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/examples)
endif(KLEX_EXAMPLES)
