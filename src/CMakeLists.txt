cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	internationalization.cpp
	ruleMatcherAutomaton.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusanalyzer_INCLUDE_DIRS}" )
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${strusanalyzer_LIBRARY_DIRS}" )


# -------------------------------------------
# STRUS TRACE LIBRARIES
# -------------------------------------------
add_library( rulematch STATIC ${source_files} )
target_link_libraries( rulematch strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

