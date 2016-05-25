cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( SimplePatternMatch src/testSimplePatternMatch 1000 1000 1000 1000000 )
# 1000 features [1], 1000 documents [2] of size 1000 [3] with 1000000 patterns [4]
