cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	compactNodeTrie.cpp
)

include_directories(
	"${PATTERN_INCLUDE_DIRS}"
	"${CNODETRIE_INCLUDE_DIRS}"
)

# -------------------------------------------
# SNOWBALL STEMMER LIBRARY
# -------------------------------------------
add_library( compactnodetrie_static STATIC ${source_files})
set_property( TARGET compactnodetrie_static PROPERTY POSITION_INDEPENDENT_CODE TRUE )



