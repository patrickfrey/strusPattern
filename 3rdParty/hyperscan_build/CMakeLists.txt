cmake_minimum_required (VERSION 2.8.11)
project (hyperscan C CXX)

set (HS_MAJOR_VERSION 4)
set (HS_MINOR_VERSION 5)
set (HS_PATCH_VERSION 2)
set (HS_VERSION ${HS_MAJOR_VERSION}.${HS_MINOR_VERSION}.${HS_PATCH_VERSION})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSymbolExists)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
INCLUDE (CheckLibraryExists)
INCLUDE (CheckSymbolExists)
include (CMakeDependentOption)
include (GNUInstallDirs)
include( cmake/platform.cmake)
include( cmake/ragel.cmake)

find_package(PkgConfig QUIET)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Default build type 'Release with debug info'")
    set(CMAKE_BUILD_TYPE RELWITHDEBINFO CACHE STRING "" FORCE )
else()
    string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
    message(STATUS "Build type ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_BUILD_TYPE MATCHES RELEASE|RELWITHDEBINFO)
    set(RELEASE_BUILD TRUE)
else()
    set(RELEASE_BUILD FALSE)
endif()

set(BINDIR "${PROJECT_BINARY_DIR}/bin")
set(LIBDIR "${PROJECT_BINARY_DIR}/lib")
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

# First for the generic no-config case
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${LIBDIR}")
# Second, for multi-config builds (e.g. msvc)
foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string (TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${BINDIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${LIBDIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${LIBDIR}")
endforeach (OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)


if(CMAKE_GENERATOR STREQUAL Xcode)
    set(XCODE TRUE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR 1)
include_directories(${PROJECT_SOURCE_DIR}/../hyperscan/src)
include_directories(${PROJECT_SOURCE_DIR}/../hyperscan/include)
include_directories(${PROJECT_SOURCE_DIR}/../hyperscan/util)
include_directories(${PROJECT_BINARY_DIR})
include_directories(SYSTEM include)

include (${CMAKE_MODULE_PATH}/boost.cmake)

# -- make this work? set(python_ADDITIONAL_VERSIONS 2.7 2.6)
find_package(PythonInterp)
find_program(RAGEL ragel)

if(PYTHONINTERP_FOUND)
    set(PYTHON ${PYTHON_EXECUTABLE})
else()
    message(FATAL_ERROR "No python interpreter found")
endif()

# allow for reproducible builds - python for portability
if (DEFINED ENV{SOURCE_DATE_EPOCH})
      execute_process(
          COMMAND "${PYTHON}" "${CMAKE_MODULE_PATH}/formatdate.py" "$ENV{SOURCE_DATE_EPOCH}"
          OUTPUT_VARIABLE BUILD_DATE
          OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    string (TIMESTAMP BUILD_DATE "%Y-%m-%d")
endif ()
message(STATUS "Build date: ${BUILD_DATE}")


if(${RAGEL} STREQUAL "RAGEL-NOTFOUND")
    message(FATAL_ERROR "Ragel state machine compiler not found")
endif()

option(OPTIMISE "Turns off compiler optimizations (on by default unless debug output enabled or coverage testing)" TRUE)

option(DEBUG_OUTPUT "Enable debug output (warning: very verbose)" FALSE)

if(DEBUG_OUTPUT)
    add_definitions(-DDEBUG)
    set(OPTIMISE FALSE)
endif(DEBUG_OUTPUT)

option(BUILD_SHARED_LIBS "Build shared libs instead of static" OFF)
option(BUILD_STATIC_AND_SHARED "Build shared libs as well as static" OFF)

if (BUILD_STATIC_AND_SHARED OR BUILD_SHARED_LIBS)
    if (WIN32)
        message(FATAL_ERROR "Windows DLLs currently not supported")
    else()
        message(STATUS "Building shared libraries")
    endif()
endif()

if (NOT BUILD_SHARED_LIBS)
    # build static libs
    set(BUILD_STATIC_LIBS ON)
    mark_as_advanced(BUILD_STATIC_LIBS)
endif ()

#for config
if (OPTIMISE)
    set(HS_OPTIMIZE ON)
endif()

CMAKE_DEPENDENT_OPTION(DUMP_SUPPORT "Dump code support; normally on, except in release builds" ON "NOT RELEASE_BUILD" OFF)

CMAKE_DEPENDENT_OPTION(DISABLE_ASSERTS "Disable assert(); Asserts are enabled in debug builds, disabled in release builds" OFF "NOT RELEASE_BUILD" ON)

option(BUILD_AVX512 "Experimental: support avx512 in the fat runtime"
    OFF)

option(WINDOWS_ICC "Use Intel C++ Compiler on Windows, default off, requires ICC to be set in project" OFF)

# TODO: per platform config files?

# TODO: windows generator on cmake always uses msvc, even if we plan to build with icc
if(MSVC OR MSVC_IDE)
    message(STATUS "Building for Windows")

    if (MSVC_VERSION LESS 1700)
        message(FATAL_ERROR "The project requires C++11 features.")
    else()
        if (WINDOWS_ICC)
            set(ARCH_C_FLAGS "/QxHost")
            set(ARCH_CXX_FLAGS "/QxHost")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O3 /Qstd=c99 /Qrestrict /wd4267 /Qdiag-disable:remark")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /Qstd=c++11 /Qrestrict /QxHost /wd4267 /wd4800 /Qdiag-disable:remark -DBOOST_DETAIL_NO_CONTAINER_FWD -D_SCL_SECURE_NO_WARNINGS")
        else()
            # todo: change these as required
            set(ARCH_C_FLAGS "/arch:AVX2")
            set(ARCH_CXX_FLAGS "/arch:AVX2")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  /O2 /wd4244 /wd4267")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /wd4244 /wd4267 /wd4800 -DBOOST_DETAIL_NO_CONTAINER_FWD -D_SCL_SECURE_NO_WARNINGS")
        endif()
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        if (DISABLE_ASSERTS)
            set(CMAKE_C_FLAGS_DEBUG "/DNDEBUG ${CMAKE_C_FLAGS_DEBUG}")
            set(CMAKE_CXX_FLAGS_DEBUG "/DNDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")
        endif ()

        # flags only used to build hs libs
        set(HS_C_FLAGS "/Gv")
        set(HS_CXX_FLAGS "/Gv")
    endif()

else()

    # remove CMake's idea of optimisation
    foreach (CONFIG ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
        string(REGEX REPLACE "-O[^ ]*" "" CMAKE_C_FLAGS_${CONFIG} "${CMAKE_C_FLAGS_${CONFIG}}")
        string(REGEX REPLACE "-O[^ ]*" "" CMAKE_CXX_FLAGS_${CONFIG} "${CMAKE_CXX_FLAGS_${CONFIG}}")
    endforeach ()

    if (CMAKE_COMPILER_IS_GNUCC)
        message(STATUS "gcc version ${CMAKE_C_COMPILER_VERSION}")
        # If gcc doesn't recognise the host cpu, then mtune=native becomes
        # generic, which isn't very good in some cases. march=native looks at
        # cpuid info and then chooses the best microarch it can (and replaces
        # the flag), so use that for tune.

        # arg1 might exist if using ccache
        string (STRIP "${CMAKE_C_COMPILER_ARG1}" CC_ARG1)
        set (EXEC_ARGS ${CC_ARG1} -c -Q --help=target -march=native -mtune=native)
        execute_process(COMMAND ${CMAKE_C_COMPILER} ${EXEC_ARGS}
            OUTPUT_VARIABLE _GCC_OUTPUT)
        string(REGEX REPLACE ".*march=[ \t]*([^ \n]*)[ \n].*" "\\1"
            GNUCC_ARCH "${_GCC_OUTPUT}")

        # test the parsed flag
        set (EXEC_ARGS ${CC_ARG1} -E - -mtune=${GNUCC_ARCH})
        execute_process(COMMAND ${CMAKE_C_COMPILER} ${EXEC_ARGS}
            OUTPUT_QUIET ERROR_QUIET
            INPUT_FILE /dev/null
            RESULT_VARIABLE GNUCC_TUNE_TEST)
        if (NOT GNUCC_TUNE_TEST EQUAL 0)
            message(SEND_ERROR "Something went wrong determining gcc tune: -mtune=${GNUCC_ARCH} not valid")
        endif()
        set(TUNE_FLAG ${GNUCC_ARCH})
    else ()
        set(TUNE_FLAG native)
    endif()

    # compiler version checks TODO: test more compilers
    if (CMAKE_COMPILER_IS_GNUCXX)
        set(GNUCXX_MINVER "4.8.1")
        message(STATUS "g++ version ${CMAKE_CXX_COMPILER_VERSION}")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS GNUCXX_MINVER)
            message(FATAL_ERROR "A minimum of g++ ${GNUCXX_MINVER} is required for C++11 support")
        endif()
    endif()

    if(OPTIMISE)
        set(OPT_C_FLAG "-O3")
        set(OPT_CXX_FLAG "-O2")
    else()
        set(OPT_C_FLAG "-O0")
        set(OPT_CXX_FLAG "-O0")
    endif(OPTIMISE)

    # set compiler flags - more are tested and added later
    set(EXTRA_C_FLAGS "${OPT_C_FLAG} -std=c99 -Wall -Wextra -Wshadow -Wcast-qual -fno-strict-aliasing")
    set(EXTRA_CXX_FLAGS "${OPT_CXX_FLAG} -std=c++11 -Wall -Wextra -Wshadow -Wswitch -Wreturn-type -Wcast-qual -Wno-deprecated -Wnon-virtual-dtor -fno-strict-aliasing")

    if (NOT RELEASE_BUILD)
        # -Werror is most useful during development, don't potentially break
        # release builds
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS}")
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS}")
    endif()

    if (DISABLE_ASSERTS)
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -DNDEBUG")
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -DNDEBUG")
    endif()

    if (NOT CMAKE_C_FLAGS MATCHES .*march.* AND NOT CMAKE_C_FLAGS MATCHES .*mtune.*)
        set(ARCH_C_FLAGS "-march=native -mtune=${TUNE_FLAG}")
    endif()

    if (NOT CMAKE_CXX_FLAGS MATCHES .*march.* AND NOT CMAKE_CXX_FLAGS MATCHES .*mtune.*)
        set(ARCH_CXX_FLAGS "-march=native -mtune=${TUNE_FLAG}")
    endif()

    if(CMAKE_COMPILER_IS_GNUCC)
        # spurious warnings?
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -Wno-array-bounds -Wno-maybe-uninitialized")
    endif()

    if(CMAKE_COMPILER_IS_GNUCXX)
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -fabi-version=0 -Wno-unused-local-typedefs -Wno-maybe-uninitialized")
    endif()

    if (NOT(ARCH_IA32 AND RELEASE_BUILD))
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -fno-omit-frame-pointer")
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -fno-omit-frame-pointer")
    endif()

    if (RELEASE_BUILD)
        # we don't need the noise of ABI warnings in a release build
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -Wno-abi")
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wno-abi")
    endif ()

    if (CMAKE_C_COMPILER_ID MATCHES "Intel")
        set(SKYLAKE_FLAG "-xCORE-AVX512")
    else ()
        set(SKYLAKE_FLAG "-march=skylake-avx512")
    endif ()
endif()

CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_C_INTRIN_H)
CHECK_INCLUDE_FILE_CXX(intrin.h HAVE_CXX_INTRIN_H)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_C_X86INTRIN_H)
CHECK_INCLUDE_FILE_CXX(x86intrin.h HAVE_CXX_X86INTRIN_H)

CHECK_FUNCTION_EXISTS(posix_memalign HAVE_POSIX_MEMALIGN)
CHECK_FUNCTION_EXISTS(_aligned_malloc HAVE__ALIGNED_MALLOC)

# these end up in the config file
CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAS_C_HIDDEN)
CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden HAS_CXX_HIDDEN)

# are we using libc++
CHECK_CXX_SYMBOL_EXISTS(_LIBCPP_VERSION ciso646 HAVE_LIBCPP)

if (RELEASE_BUILD)
    if (HAS_C_HIDDEN)
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -fvisibility=hidden")
    endif()
    if (HAS_CXX_HIDDEN)
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    # This is a Linux-only feature for now - requires platform support
    # elsewhere
    message(STATUS "generator is ${CMAKE_GENERATOR}")
    if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND
        CMAKE_C_COMPILER_VERSION VERSION_LESS "3.9")
        message (STATUS "Clang v3.9 or higher required for fat runtime, cannot build fat runtime")
        set (FAT_RUNTIME_REQUISITES FALSE)
    elseif (NOT (CMAKE_GENERATOR MATCHES "Unix Makefiles" OR
            (CMAKE_VERSION VERSION_GREATER "3.0" AND CMAKE_GENERATOR MATCHES "Ninja")))
        message (STATUS "Building the fat runtime requires the Unix Makefiles generator, or Ninja with CMake v3.0 or higher")
        set (FAT_RUNTIME_REQUISITES FALSE)
    else()
        include (${CMAKE_MODULE_PATH}/attrib.cmake)
        if (NOT HAS_C_ATTR_IFUNC)
            message(STATUS "Compiler does not support ifunc attribute, cannot build fat runtime")
            set (FAT_RUNTIME_REQUISITES FALSE)
        else ()
            set (FAT_RUNTIME_REQUISITES TRUE)
        endif()
    endif()
    CMAKE_DEPENDENT_OPTION(FAT_RUNTIME "Build a library that supports multiple microarchitecures" ${RELEASE_BUILD} "FAT_RUNTIME_REQUISITES" OFF)
endif ()

include (${CMAKE_MODULE_PATH}/arch.cmake)

# testing a builtin takes a little more work
CHECK_C_SOURCE_COMPILES("void *aa_test(void *x) { return __builtin_assume_aligned(x, 16);}\nint main(void) { return 0; }" HAVE_CC_BUILTIN_ASSUME_ALIGNED)
CHECK_CXX_SOURCE_COMPILES("void *aa_test(void *x) { return __builtin_assume_aligned(x, 16);}\nint main(void) { return 0; }" HAVE_CXX_BUILTIN_ASSUME_ALIGNED)
CHECK_C_SOURCE_COMPILES("int main(void) { __builtin_constant_p(0); }" HAVE__BUILTIN_CONSTANT_P)

if (NOT WIN32)
set(C_FLAGS_TO_CHECK
# Variable length arrays are way bad, most especially at run time
"-Wvla"
# Pointer arith on void pointers is doing it wong.
 "-Wpointer-arith"
# Build our C code with -Wstrict-prototypes -Wmissing-prototypes
 "-Wstrict-prototypes"
 "-Wmissing-prototypes"
)
foreach (FLAG ${C_FLAGS_TO_CHECK})
    # munge the name so it doesn't break things
    string(REPLACE "-" "_" FNAME C_FLAG${FLAG})
    CHECK_C_COMPILER_FLAG("${FLAG}" ${FNAME})
    if (${FNAME})
        set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} ${FLAG}")
    endif()
endforeach()

set(CXX_FLAGS_TO_CHECK
"-Wvla"
"-Wpointer-arith"
)
foreach (FLAG ${CXX_FLAGS_TO_CHECK})
    string(REPLACE "-" "_" FNAME CXX_FLAG${FLAG})
    CHECK_CXX_COMPILER_FLAG("${FLAG}" ${FNAME})
    if (${FNAME})
        set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} ${FLAG}")
    endif()
endforeach()

# self-assign should be thrown away, but clang whinges
CHECK_C_COMPILER_FLAG("-Wself-assign" CC_SELF_ASSIGN)
if (CC_SELF_ASSIGN)
    set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -Wno-self-assign")
endif()
CHECK_CXX_COMPILER_FLAG("-Wself-assign" CXX_SELF_ASSIGN)
if (CXX_SELF_ASSIGN)
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wno-self-assign")
endif()

# clang gets up in our face for going paren crazy with macros
CHECK_C_COMPILER_FLAG("-Wparentheses-equality" CC_PAREN_EQUALITY)
if (CC_PAREN_EQUALITY)
    set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -Wno-parentheses-equality")
endif()

# clang compains about unused const vars in our Ragel-generated code.
CHECK_CXX_COMPILER_FLAG("-Wunused-const-variable" CXX_UNUSED_CONST_VAR)
if (CXX_UNUSED_CONST_VAR)
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wno-unused-const-variable")
endif()

# gcc 6 complains about type attributes that get ignored, like alignment
CHECK_CXX_COMPILER_FLAG("-Wignored-attributes" CXX_IGNORED_ATTR)
if (CXX_IGNORED_ATTR)
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wno-ignored-attributes")
endif()

# note this for later
# g++ doesn't have this flag but clang does
CHECK_CXX_COMPILER_FLAG("-Wweak-vtables" CXX_WEAK_VTABLES)
if (CXX_WEAK_VTABLES)
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wweak-vtables")
endif()

CHECK_CXX_COMPILER_FLAG("-Wmissing-declarations" CXX_MISSING_DECLARATIONS)
if (CXX_MISSING_DECLARATIONS)
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -Wmissing-declarations")
endif()

# gcc5 complains about this
CHECK_CXX_COMPILER_FLAG("-Wunused-variable" CXX_WUNUSED_VARIABLE)

endif()

if (NOT XCODE)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
else()
    # cmake doesn't think Xcode supports isystem
    set(EXTRA_CXX_FLAGS "${EXTRA_CXX_FLAGS} -isystem ${Boost_INCLUDE_DIRS}")
endif()


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX TRUE)
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(FREEBSD true)
endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")

if(NOT WIN32)
if(CMAKE_C_COMPILER_ID MATCHES "Intel")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -diag-error 10006 -diag-disable 177 -diag-disable 2304 -diag-disable 2305 -diag-disable 2338 -diag-disable 1418 -diag-disable=remark")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -diag-error 10006 -diag-disable 177 -diag-disable 2304 -diag-disable 2305 -diag-disable 2338 -diag-disable 1418 -diag-disable 1170 -diag-disable 3373 -diag-disable=remark")
endif()
endif()

if (NOT FAT_RUNTIME)
    message(STATUS "Building for current host CPU: ${ARCH_C_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_CXX_FLAGS}")
else()
    message(STATUS "Building runtime for multiple microarchitectures")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory( util)
# add_subdirectory(${PROJECT_SOURCE_DIR}/../hyperscan/unit)
# add_subdirectory(${PROJECT_SOURCE_DIR}/../hyperscan/doc/dev-reference)
# if (EXISTS ${CMAKE_SOURCE_DIR}/tools/CMakeLists.txt)
#     add_subdirectory(${PROJECT_SOURCE_DIR}/../hyperscan/tools)
# endif()

# do substitutions
configure_file(${CMAKE_MODULE_PATH}/config.h.in ${PROJECT_BINARY_DIR}/config.h)
configure_file(${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.h.in ${PROJECT_BINARY_DIR}/hs_version.h)

if (NOT WIN32)
    # expand out library names for pkgconfig static link info
    foreach (LIB ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
        # this is fragile, but protects us from toolchain specific files
        if (NOT EXISTS ${LIB})
            set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
        endif()
    endforeach()

    configure_file(libhs.pc.in ${PROJECT_BINARY_DIR}/libhs.pc @ONLY) # only replace @ quoted vars
#    install(FILES ${PROJECT_BINARY_DIR}/libhs.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

# only set these after all tests are done
if (NOT FAT_RUNTIME)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_C_FLAGS} ${HS_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} ${HS_CXX_FLAGS}")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")
endif()

if(NOT WIN32)
set(RAGEL_C_FLAGS "-Wno-unused")
endif()

set_source_files_properties(
    src/parser/Parser.cpp
    PROPERTIES
        COMPILE_FLAGS "${RAGEL_C_FLAGS}")

ragelmaker(src/parser/Parser.rl)

set_source_files_properties(
    src/parser/control_verbs.cpp
    PROPERTIES
        COMPILE_FLAGS "${RAGEL_C_FLAGS}")

ragelmaker(src/parser/control_verbs.rl)

SET(hs_HEADERS
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_common.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_compile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_runtime.h
)
install(FILES ${hs_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hs")

set (hs_exec_common_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/alloc.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/scratch.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/cpuid_flags.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/cpuid_flags.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit.c
    )

set (hs_exec_SRCS
    ${hs_HEADERS}
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/ue2common.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/allocator.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/crc32.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/crc32.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/report.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/runtime.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_confirm.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_confirm_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/flood_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_loadval.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_runtime_common.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_engine.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_engine.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castle.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castle.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castle_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/gough.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/gough_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/lbr.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/lbr.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/lbr_common_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/lbr_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_accel.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_exceptional.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_native.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_ring.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_64.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_simd128.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_simd256.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_simd384.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_simd512.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_common_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_context.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_runtime_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_shuffle.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_state_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan_common_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_data.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpv.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpv.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpv_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_api.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_api_dispatch.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_rev_api.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeat.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeat.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeat_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng_defs.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng_impl4.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shufti.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shufti.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamarama.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamarama.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamarama_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/truffle.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/truffle.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/vermicelli.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/vermicelli_run.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/vermicelli_sse.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_operation.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_runtime.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_stream.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_stream.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/block.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/catchup.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/catchup.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/infix.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/init.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/init.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/stream.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/stream_long_lit.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/stream_long_lit_hash.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/match.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/match.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/miracle.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/program_runtime.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/program_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_program.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_types.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_common.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/validate_mask.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/validate_shufti.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/bitutils.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/copybytes.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/exhaust.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/fatbit.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/join.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/masked_move.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/pack_bits.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/popcount.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/pqueue.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/scatter.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/scatter_runtime.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/simd_utils.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/simd_utils.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/state_compress.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/state_compress.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/unaligned.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/uniform_ops.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/database.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/database.h
)

set (hs_exec_avx2_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_avx2.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/masked_move.c
)


SET (hs_SRCS
    ${hs_HEADERS}
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/crc32.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/database.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/grey.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/grey.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/scratch.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/state.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/ue2common.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/asserts.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/asserts.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/compiler.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/compiler.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/error.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/error.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/compiler/expression_info.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/engine_description.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/engine_description.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_compile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_compile_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_compile_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_confirm_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_confirm.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_engine_description.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_engine_description.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/flood_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_compile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_engine_description.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_engine_description.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/teddy_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_build.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_literal.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_literal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_build.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/noodle_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel_dfa_build_strat.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel_dfa_build_strat.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accelcompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accelcompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/callback.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castlecompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castlecompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/dfa_build_strat.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/dfa_build_strat.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/dfa_min.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/dfa_min.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile_accel.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile_reg.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellan_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellancompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellancompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellancompile_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellancompile_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_compile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_compile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpv_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpvcompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpvcompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_api.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_api_queue.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_api_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_build_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_build_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_kind.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/rdfa.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/rdfa_graph.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/rdfa_graph.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/rdfa_merge.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/rdfa_merge.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeat_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeatcompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/repeatcompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/sheng_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shengcompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shengcompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shufticompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shufticompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamaramacompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamaramacompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/trufflecompile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/trufflecompile.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_anchored_acyclic.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_anchored_acyclic.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_anchored_dots.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_anchored_dots.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_asserts.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_asserts.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_builder.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_builder.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_calc_components.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_calc_components.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_cyclic_redundancy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_cyclic_redundancy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_depth.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_depth.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_dominators.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_dominators.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_edge_redundancy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_edge_redundancy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_equivalence.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_equivalence.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_execute.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_execute.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_expr_info.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_expr_info.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_extparam.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_extparam.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_fixed_width.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_fixed_width.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_fuzzy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_fuzzy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_haig.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_haig.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_holder.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_holder.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_is_equal.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_is_equal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_lbr.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_lbr.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_analysis.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_analysis.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_component.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_component.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_decorated.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_literal_decorated.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_mcclellan.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_mcclellan.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_mcclellan_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_limex.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_limex.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_limex_accel.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_limex_accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_misc_opt.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_misc_opt.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_netflow.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_netflow.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_prefilter.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_prefilter.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_prune.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_prune.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_puff.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_puff.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_redundancy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_redundancy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_region.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_region.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_region_redundancy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_region_redundancy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_repeat.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_repeat.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_reports.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_reports.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_restructuring.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_restructuring.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_revacc.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_revacc.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_sep.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_sep.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_small_literal_set.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_small_literal_set.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som_add_redundancy.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som_add_redundancy.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_som_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_split.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_split.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_squash.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_squash.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_stop.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_stop.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_uncalc_components.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_uncalc_components.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_undirected.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_utf8.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_utf8.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_vacuous.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_vacuous.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_violet.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_violet.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_width.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_width.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/AsciiComponentClass.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/AsciiComponentClass.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/Component.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/Component.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAlternation.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAlternation.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAssertion.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAssertion.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAtomicGroup.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentAtomicGroup.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentBackReference.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentBackReference.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentBoundary.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentBoundary.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentByte.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentByte.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentClass.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentClass.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentCondReference.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentCondReference.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentEUS.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentEUS.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentEmpty.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentEmpty.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentRepeat.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentRepeat.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentSequence.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentSequence.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentVisitor.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentVisitor.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentWordBoundary.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ComponentWordBoundary.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ConstComponentVisitor.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ConstComponentVisitor.h
    src/parser/Parser.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/Parser.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/Utf8ComponentClass.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/Utf8ComponentClass.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/buildstate.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/buildstate.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/check_refs.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/check_refs.h
    src/parser/control_verbs.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/control_verbs.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/parse_error.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/parse_error.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/parser_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/position.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/position_info.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/prefilter.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/prefilter.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/shortcut_literal.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/shortcut_literal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ucp_table.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/ucp_table.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/unsupported.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/unsupported.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/utf8_validate.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/utf8_validate.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/smallwrite/smallwrite_build.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/smallwrite/smallwrite_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/smallwrite/smallwrite_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/slot_manager.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/slot_manager.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/slot_manager_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/som_operation.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_add.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_add_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_add_mask.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_anchored.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_anchored.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_bytecode.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_castle.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_castle.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_compile.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_convert.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_convert.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_dedupe.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_engine_blob.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_engine_blob.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_exclusive.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_exclusive.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_groups.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_groups.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_impl.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_infix.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_infix.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_instructions.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_instructions.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_lit_accel.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_lit_accel.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_long_lit.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_long_lit.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_lookaround.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_lookaround.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_matchers.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_matchers.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_merge.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_merge.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_misc.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_program.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_program.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_resources.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_role_aliasing.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_scatter.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_scatter.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_width.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_width.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_graph.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_in_graph.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_in_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_in_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/accel_scheme.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/alloc.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/alloc.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/bitfield.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/boundary_reports.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/charreach.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/charreach.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/charreach_util.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/clique.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/clique.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/compare.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/compile_context.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/compile_context.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/compile_error.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/compile_error.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/container.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/depth.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/depth.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/determinise.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_mask.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_mask.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/fatbit_build.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/fatbit_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/graph.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/hash.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/hash_dynamic_bitset.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/math.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit_build.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/multibit_build.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/noncopyable.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/operators.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/order_check.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/partial_store.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/partitioned_set.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/popcount.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/queue_index_factory.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/report.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/report_manager.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/report_manager.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/simd_utils.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/small_vector.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/target_info.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/target_info.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/ue2_containers.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/ue2_graph.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/ue2string.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/ue2string.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/unaligned.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/unicode_def.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/unicode_set.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/uniform_ops.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/verify_types.h
)

set(hs_dump_SRCS
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/scratch_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/scratch_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/fdr/fdr_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/hwlm/hwlm_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/accel_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castle_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/castle_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfagraph/ng_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughcompile_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughdump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/goughdump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/lbr_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/limex_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellandump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcclellandump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mcsheng_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/mpv_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_dump_api.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_dump_dispatch.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_dump_internal.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/nfa_dump_internal.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shengdump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/shengdump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamarama_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/nfa/tamarama_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/parser/position_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/smallwrite/smallwrite_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/smallwrite/smallwrite_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/slot_manager_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/som/slot_manager_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_build_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_in_dump.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/rose/rose_in_dump.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_charclass.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_charclass.h
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_util.cpp
    ${PROJECT_SOURCE_DIR}/../hyperscan/src/util/dump_util.h
)

if (DUMP_SUPPORT)
    set(hs_SRCS ${hs_SRCS} ${hs_dump_SRCS})
endif()

# we group things by sublibraries, specifying shared and static and then
# choose which ones to build

set (LIB_VERSION ${HS_VERSION})
set (LIB_SOVERSION ${HS_MAJOR_VERSION})

if (NOT FAT_RUNTIME)

    set(hs_exec_SRCS ${hs_exec_SRCS} ${hs_exec_common_SRCS})

    if (HAVE_AVX2)
        set(hs_exec_SRCS ${hs_exec_SRCS} ${hs_exec_avx2_SRCS})
    endif()

    if (BUILD_STATIC_LIBS)
        add_library(hs_exec OBJECT ${hs_exec_SRCS})

        add_library(hs_runtime STATIC ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c $<TARGET_OBJECTS:hs_exec>)
        set_target_properties(hs_runtime PROPERTIES LINKER_LANGUAGE C)

        add_library(hs STATIC ${hs_SRCS} ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c $<TARGET_OBJECTS:hs_exec>)
    endif (BUILD_STATIC_LIBS)

    if (BUILD_STATIC_AND_SHARED OR BUILD_SHARED_LIBS)
        add_library(hs_exec_shared OBJECT ${hs_exec_SRCS})
        set_target_properties(hs_exec_shared PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    endif()

else (FAT_RUNTIME)

    set(BUILD_WRAPPER "${PROJECT_SOURCE_DIR}/cmake/build_wrapper.sh")
    if (NOT BUILD_AVX512)
        set (DISPATCHER_DEFINE "-DDISABLE_AVX512_DISPATCH")
    endif (NOT BUILD_AVX512)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/../hyperscan/src/dispatcher.c PROPERTIES
        COMPILE_FLAGS "-Wno-unused-parameter -Wno-unused-function ${DISPATCHER_DEFINE}")

    if (BUILD_STATIC_LIBS)
       add_library(hs_exec_core2 OBJECT ${hs_exec_SRCS})
       list(APPEND RUNTIME_LIBS $<TARGET_OBJECTS:hs_exec_core2>)
       set_target_properties(hs_exec_core2 PROPERTIES
           COMPILE_FLAGS "-march=core2"
           RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} core2 ${CMAKE_MODULE_PATH}/keep.syms.in"
           )

       add_library(hs_exec_corei7 OBJECT ${hs_exec_SRCS})
       list(APPEND RUNTIME_LIBS $<TARGET_OBJECTS:hs_exec_corei7>)
       set_target_properties(hs_exec_corei7 PROPERTIES
           COMPILE_FLAGS "-march=corei7"
           RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} corei7 ${CMAKE_MODULE_PATH}/keep.syms.in"
           )

       add_library(hs_exec_avx2 OBJECT ${hs_exec_SRCS} ${hs_exec_avx2_SRCS})
       list(APPEND RUNTIME_LIBS $<TARGET_OBJECTS:hs_exec_avx2>)
       set_target_properties(hs_exec_avx2 PROPERTIES
           COMPILE_FLAGS "-march=core-avx2"
           RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} avx2 ${CMAKE_MODULE_PATH}/keep.syms.in"
           )
       if (BUILD_AVX512)
           add_library(hs_exec_avx512 OBJECT ${hs_exec_SRCS} ${hs_exec_avx2_SRCS})
           list(APPEND RUNTIME_LIBS $<TARGET_OBJECTS:hs_exec_avx512>)
           set_target_properties(hs_exec_avx512 PROPERTIES
               COMPILE_FLAGS "${SKYLAKE_FLAG}"
               RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} avx512 ${CMAKE_MODULE_PATH}/keep.syms.in"
               )
       endif (BUILD_AVX512)

       add_library(hs_exec_common OBJECT
           ${hs_exec_common_SRCS}
           ${PROJECT_SOURCE_DIR}/../hyperscan/src/dispatcher.c
           )

       # hs_version.c is added explicitly to avoid some build systems that refuse to
       # create a lib without any src (I'm looking at you Xcode)

       add_library(hs_runtime STATIC ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c
           $<TARGET_OBJECTS:hs_exec_common>
           ${RUNTIME_LIBS})
       set_target_properties(hs_runtime PROPERTIES LINKER_LANGUAGE C)

       # we want the static lib for testing
       add_library(hs STATIC ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c
           ${hs_SRCS}
           $<TARGET_OBJECTS:hs_exec_common>
           ${RUNTIME_LIBS})

    endif (BUILD_STATIC_LIBS)

    if (BUILD_STATIC_AND_SHARED OR BUILD_SHARED_LIBS)
        # build shared libs
        add_library(hs_exec_shared_core2 OBJECT ${hs_exec_SRCS})
        list(APPEND RUNTIME_SHLIBS $<TARGET_OBJECTS:hs_exec_shared_core2>)
        set_target_properties(hs_exec_shared_core2 PROPERTIES
            COMPILE_FLAGS "-march=core2"
            POSITION_INDEPENDENT_CODE TRUE
            RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} core2 ${CMAKE_MODULE_PATH}/keep.syms.in"
            )
        add_library(hs_exec_shared_corei7 OBJECT ${hs_exec_SRCS})
        list(APPEND RUNTIME_SHLIBS $<TARGET_OBJECTS:hs_exec_shared_corei7>)
        set_target_properties(hs_exec_shared_corei7 PROPERTIES
            COMPILE_FLAGS "-march=corei7"
            POSITION_INDEPENDENT_CODE TRUE
            RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} corei7 ${CMAKE_MODULE_PATH}/keep.syms.in"
            )
        add_library(hs_exec_shared_avx2 OBJECT ${hs_exec_SRCS} ${hs_exec_avx2_SRCS})
        list(APPEND RUNTIME_SHLIBS $<TARGET_OBJECTS:hs_exec_shared_avx2>)
        set_target_properties(hs_exec_shared_avx2 PROPERTIES
            COMPILE_FLAGS "-march=core-avx2"
            POSITION_INDEPENDENT_CODE TRUE
            RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} avx2 ${CMAKE_MODULE_PATH}/keep.syms.in"
            )

        if (BUILD_AVX512)
            add_library(hs_exec_shared_avx512 OBJECT ${hs_exec_SRCS} ${hs_exec_avx2_SRCS})
            list(APPEND RUNTIME_SHLIBS $<TARGET_OBJECTS:hs_exec_shared_avx512>)
            set_target_properties(hs_exec_shared_avx512 PROPERTIES
                COMPILE_FLAGS "${SKYLAKE_FLAG}"
                POSITION_INDEPENDENT_CODE TRUE
                RULE_LAUNCH_COMPILE "${BUILD_WRAPPER} avx512 ${CMAKE_MODULE_PATH}/keep.syms.in"
                )
        endif (BUILD_AVX512)
        add_library(hs_exec_common_shared OBJECT
        ${hs_exec_common_SRCS}
        ${PROJECT_SOURCE_DIR}/../hyperscan/src/dispatcher.c
        )
        set_target_properties(hs_exec_common_shared PROPERTIES
            OUTPUT_NAME hs_exec_common
            POSITION_INDEPENDENT_CODE TRUE)
    endif() # SHARED


endif (NOT FAT_RUNTIME)

if (NOT BUILD_SHARED_LIBS)
    install(TARGETS hs_runtime DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_STATIC_AND_SHARED OR BUILD_SHARED_LIBS)
    if (NOT FAT_RUNTIME)
        add_library(hs_runtime_shared SHARED ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c
            ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c $<TARGET_OBJECTS:hs_exec_shared>)
    else()
        add_library(hs_runtime_shared SHARED ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c
            ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c
            $<TARGET_OBJECTS:hs_exec_common_shared>
            ${RUNTIME_SHLIBS})
    endif()
    set_target_properties(hs_runtime_shared PROPERTIES
        VERSION ${LIB_VERSION}
        SOVERSION ${LIB_SOVERSION}
        OUTPUT_NAME hs_runtime
        MACOSX_RPATH ON
        LINKER_LANGUAGE C)
    install(TARGETS hs_runtime_shared
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_STATIC_LIBS)
    add_dependencies(hs ragel_Parser)
endif ()

if (NOT BUILD_SHARED_LIBS)
    install(TARGETS hs DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (BUILD_STATIC_AND_SHARED OR BUILD_SHARED_LIBS)
    if (NOT FAT_RUNTIME)
        add_library(hs_shared SHARED ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c
            ${hs_SRCS} $<TARGET_OBJECTS:hs_exec_shared>)
    else()
        add_library(hs_shared SHARED ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_version.c ${PROJECT_SOURCE_DIR}/../hyperscan/src/hs_valid_platform.c
            ${hs_SRCS} $<TARGET_OBJECTS:hs_exec_common_shared>
            ${RUNTIME_SHLIBS})

    endif()
    add_dependencies(hs_shared ragel_Parser)
    set_target_properties(hs_shared PROPERTIES
        OUTPUT_NAME hs
        VERSION ${LIB_VERSION}
        SOVERSION ${LIB_SOVERSION}
        MACOSX_RPATH ON)
install(TARGETS hs_shared
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# used by tools and other targets
if (NOT BUILD_STATIC_LIBS)
    # use shared lib without having to change all the targets
    add_library(hs ALIAS hs_shared)
endif ()


# if(NOT WIN32)
#    add_subdirectory(examples)
# endif()
