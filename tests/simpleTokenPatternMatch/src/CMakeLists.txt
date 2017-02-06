cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/tests/utils"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${PROJECT_SOURCE_DIR}/src"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testSimpleTokenPatternMatch testSimpleTokenPatternMatch.cpp )
target_link_libraries( testSimpleTokenPatternMatch local_test_utils strus_error strus_base strus_pattern ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

