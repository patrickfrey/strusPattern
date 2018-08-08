cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Intl_INCLUDE_DIRS}"
	"${TRE_INCLUDE_DIRS}"
)
link_directories(
)

add_executable( testTre testTre.cpp )
target_link_libraries( testTre ${TRE_LIBRARIES} "${Intl_LIBRARIES}"  )

