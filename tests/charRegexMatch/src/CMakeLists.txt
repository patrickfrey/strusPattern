cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
)
link_directories(
	"${CMAKE_BINARY_DIR}/tests/charRegexMatch/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testCharRegexMatch testCharRegexMatch.cpp )
target_link_libraries( testCharRegexMatch strus_error strus_base strus_pattern ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

