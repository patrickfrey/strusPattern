# - Find the "hyperscan" library
#

find_path ( HYPERSCAN_INCLUDE_DIRS NAMES hs/hs.h )
find_library ( HYPERSCAN_LIBRARIES NAMES hs )

# Handle the QUIETLY and REQUIRED arguments and set TRE_FOUND to TRUE if all listed variables are TRUE.
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args ( Hyperscan DEFAULT_MSG HYPERSCAN_LIBRARIES HYPERSCAN_INCLUDE_DIRS )

if ( HYPERSCAN_FOUND )
  MESSAGE( STATUS "Hyperscan includes: ${HYPERSCAN_INCLUDE_DIRS}" )
  MESSAGE( STATUS "Hyperscan libraries: ${HYPERSCAN_LIBRARIES}" )
else ( HYPERSCAN_FOUND )
  message( STATUS "Intel hyperscan library not found" )
endif ( HYPERSCAN_FOUND )

