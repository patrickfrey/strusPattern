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
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${PROJECT_SOURCE_DIR}/src"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# STRUS TEST UTILS LIBRARY
# -------------------------------------------
add_library( local_test_utils STATIC ${source_files} )
target_link_libraries( local_test_utils  strus_base strus_pattern ${Boost_LIBRARIES} "${Intl_LIBRARIES}" )

