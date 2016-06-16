cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( RandomExpressionTreeMatch src/testRandomExpressionTreeMatch -o 10000 10 1000 10000 )
# 10000 features [1], 10 documents [2] of size 1000 [3] with 10000 patterns [4]
