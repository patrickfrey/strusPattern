# utility libs

CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} ${HS_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/../hyperscan
    ${PROJECT_SOURCE_DIR})

set_source_files_properties(
    ExpressionParser.cpp
    PROPERTIES
    COMPILE_FLAGS "${RAGEL_C_FLAGS}")

ragelmaker(ExpressionParser.rl)

set(expressionutil_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/expressions.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/expressions.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ExpressionParser.h
    ExpressionParser.cpp
    )
add_library(expressionutil STATIC ${expressionutil_SRCS})
add_dependencies(expressionutil ragel_ExpressionParser)

SET(corpusomatic_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_editor.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_editor.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_generator.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_generator.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_properties.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_corpus_properties.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_find_matches.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/ng_find_matches.cpp
)
add_library(corpusomatic STATIC ${corpusomatic_SRCS})

set(databaseutil_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/database_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/database_util.h
)
add_library(databaseutil STATIC ${databaseutil_SRCS})

set(crosscompileutil_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/cross_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/util/cross_compile.h
    )
add_library(crosscompileutil STATIC ${crosscompileutil_SRCS})
