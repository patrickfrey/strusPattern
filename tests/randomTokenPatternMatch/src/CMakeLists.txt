cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/tests/utils"
	"${strusbase_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
)
link_directories(
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src"
	"${CMAKE_CURRENT_BINARY_DIR}/../../utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testRandomTokenPatternMatch testRandomTokenPatternMatch.cpp )
target_link_libraries( testRandomTokenPatternMatch strus_error strus_base strus_pattern local_test_utils ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

