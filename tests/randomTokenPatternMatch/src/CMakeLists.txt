cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

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

add_executable( testRandomTokenPatternMatch testRandomTokenPatternMatch.cpp )
target_link_libraries( testRandomTokenPatternMatch strus_error strus_base strus_stream ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

