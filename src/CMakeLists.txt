cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	${CMAKE_CURRENT_BINARY_DIR}/internationalization.cpp
	ruleMatcherAutomaton.cpp
	unicodeUtils.cpp
	patternLexer.cpp
	patternMatcher.cpp
	lexems.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PATTERN_INCLUDE_DIRS}"
	"${CNODETRIE_INCLUDE_DIRS}"
	"${TEXTWOLF_INCLUDE_DIRS}"
	"${HYPERSCAN_INCLUDE_DIRS}"
	"${TRE_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
	"${strusmodule_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${CMAKE_CURRENT_BINARY_DIR}/../3rdParty/compactNodeTrie/src"
	"${CMAKE_CURRENT_BINARY_DIR}/../3rdParty/hyperscan/lib"
	"${strusbase_LIBRARY_DIRS}"
)


# -------------------------------------------
# STRUS TRACE LIBRARIES
# -------------------------------------------
add_library( local_rulematch STATIC ${source_files} )
target_link_libraries( local_rulematch strus_base compactnodetrie_static hs ${Boost_LIBRARIES} ${TRE_LIBRARIES} ${Intl_LIBRARIES} )
set_property( TARGET local_rulematch PROPERTY POSITION_INDEPENDENT_CODE TRUE )

add_library( strus_pattern SHARED libstrus_pattern.cpp )
target_link_libraries( strus_pattern local_rulematch strus_base  ${Intl_LIBRARIES} )

set_target_properties(
    strus_pattern
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION} )

add_library( modstrus_analyzer_pattern  MODULE  modstrus_analyzer_pattern.cpp )
set_target_properties( modstrus_analyzer_pattern PROPERTIES PREFIX "")
target_link_libraries( modstrus_analyzer_pattern strus_module_analyzer strus_pattern ${Intl_LIBRARIES} )

# Copy pattern module into a created directory easy relocatable by tests:
add_custom_command( TARGET modstrus_analyzer_pattern POST_BUILD
					  COMMAND ${CMAKE_COMMAND} -E make_directory  ${CMAKE_BINARY_DIR}/modules/strus
					  COMMENT "Create local module directory '${CMAKE_BINARY_DIR}/modules/strus/' " )
add_custom_command( TARGET modstrus_analyzer_pattern POST_BUILD
					  COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:modstrus_analyzer_pattern>  ${CMAKE_BINARY_DIR}/modules/strus/
					  COMMENT "Copy strus pattern module built to local module directory" )

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_pattern
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

install( TARGETS modstrus_analyzer_pattern
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus/modules
	   RUNTIME DESTINATION bin )


