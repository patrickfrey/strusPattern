cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# add_test( RandomExpressionTreeMatch src/testRandomExpressionTreeMatch -o 100 100 100 100 "${PROJECT_SOURCE_DIR}/tests/randomExpressionTreeMatch" )
# 100 features [1], 100 documents [2] of size 100 [3] with 100 patterns [4]
