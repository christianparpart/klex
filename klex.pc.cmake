# klex library
Name: klex
Description: klex compiler frontend library
Version: @klex_VERSION@
# Requires:
# Conflicts: 
Libs: -L@CMAKE_INSTALL_PREFIX@/lib -lklex @LDFLAGS@
Cflags: -I@CMAKE_INSTALL_PREFIX@/include @CXXFLAGS@
