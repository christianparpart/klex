
option(ENABLE_TIDY "Enable clang-tidy [default: OFF]" OFF)
if(ENABLE_TIDY)
  set(CLANG_TIDY_CHECKS "\
,bugprone-*\
,clang-analyzer-*\
,cppcoreguidelines-*\
,modernize-*\
,performance-*\
,-bugprone-macro-parentheses\
,-cppcoreguidelines-owning-memory\
,-cppcoreguidelines-pro-bounds-array-to-pointer-decay\
,-cppcoreguidelines-pro-bounds-constant-array-index\
,-cppcoreguidelines-pro-bounds-pointer-arithmetic\
,-cppcoreguidelines-pro-type-cstyle-cast\
,-cppcoreguidelines-pro-type-reinterpret-cast\
,-cppcoreguidelines-pro-type-static-cast-downcast\
,-cppcoreguidelines-pro-type-union-access\
,-cppcoreguidelines-pro-type-vararg\
,-modernize-use-auto\
")

  find_program(CLANG_TIDY_EXE
    NAMES clang-tidy-8 clang-tidy-7 clang-tidy-6.0 clang-tidy
    DOC "Path to clang-tidy executable")
  if(NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=${CLANG_TIDY_CHECKS}")
  endif()
endif()
