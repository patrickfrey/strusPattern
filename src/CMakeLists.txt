cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	utils.cpp
	internationalization.cpp
	ruleMatcherAutomaton.cpp
	symbolTable.cpp
	streamPatternMatch.cpp
	streamTermMatch.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/textwolf/include"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/src"
	"${strusbase_LIBRARY_DIRS}"
)


# -------------------------------------------
# STRUS TRACE LIBRARIES
# -------------------------------------------
add_library( rulematch STATIC ${source_files} )
target_link_libraries( rulematch strus_base compactnodetrie_static ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

add_library( strus_stream SHARED libstrus_stream.cpp )
target_link_libraries( strus_stream rulematch strus_base  )

set_target_properties(
    strus_stream
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION} )

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_stream
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

