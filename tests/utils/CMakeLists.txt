cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	testUtils.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PATTERN_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
)

link_directories(
	"${PATTERN_LIBRARY_DIRS}"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# STRUS TEST UTILS LIBRARY
# -------------------------------------------
add_library( local_test_utils STATIC ${source_files} )
target_link_libraries( local_test_utils  strus_base strus_pattern ${Boost_LIBRARIES} ${Intl_LIBRARIES} )
set_property( TARGET local_test_utils PROPERTY POSITION_INDEPENDENT_CODE TRUE )

