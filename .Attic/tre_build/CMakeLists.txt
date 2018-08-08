cmake_minimum_required (VERSION 2.8)
project ( tre C CXX)

set (TRE_MAJOR_VERSION 0)
set (TRE_MINOR_VERSION 8)
set (TRE_PATCH_VERSION 8)
set (TRE_VERSION ${TRE_MAJOR_VERSION}.${TRE_MINOR_VERSION}.${TRE_PATCH_VERSION})

set( source_files
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-compile.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-match-parallel.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/xmalloc.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/regerror.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-filter.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-mem.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/regexec.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/regcomp.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-match-approx.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-parse.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-ast.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-match-backtrack.c
    ${PROJECT_SOURCE_DIR}/../tre/lib/tre-stack.c
)

include_directories(
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/../tre/lib"
	"${PROJECT_SOURCE_DIR}/../tre/include/tre"
)

link_directories(
	"${CMAKE_BINARY_DIR}/3rdParty/tre/lib"
)

# -------------------------------------------
# TRE LIBRARY
# -------------------------------------------
add_library( local_tre STATIC ${source_files} )
target_link_libraries( local_tre  ${Intl_LIBRARIES} )
set_property( TARGET local_tre PROPERTY POSITION_INDEPENDENT_CODE TRUE )


