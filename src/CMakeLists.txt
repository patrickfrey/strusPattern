cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	utils.cpp
	internationalization.cpp
	ruleMatcherAutomaton.cpp
	symbolTable.cpp
	patternLexer.cpp
	patternMatcher.cpp
	patternMatcherProgram.cpp
	tokenMarkup.cpp
	lexems.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/textwolf/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/hyperscan/src"
	"${PROJECT_SOURCE_DIR}/3rdParty/hyperscan"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/src"
	"${PROJECT_SOURCE_DIR}/3rdParty/hyperscan/lib"
	"${strusbase_LIBRARY_DIRS}"
)


# -------------------------------------------
# STRUS TRACE LIBRARIES
# -------------------------------------------
add_library( local_rulematch STATIC ${source_files} )
target_link_libraries( local_rulematch strus_base compactnodetrie_static hs ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

add_library( strus_stream SHARED libstrus_stream.cpp )
target_link_libraries( strus_stream local_rulematch strus_base  )

set_target_properties(
    strus_stream
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION} )

# -------------------------------------------
# STRUS TRACE PROGRAMS
# -------------------------------------------
add_executable( strusPatternMatcher  strusPatternMatcher.cpp )
target_link_libraries( strusPatternMatcher  strus_segmenter_textwolf strus_segmenter_cjson strus_detector_std local_rulematch strus_stream strus_base strus_error ${Boost_LIBRARIES} ${Intl_LIBRARIES})

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_stream
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

install( TARGETS strusPatternMatcher
	   RUNTIME DESTINATION bin )

