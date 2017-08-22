# - Find the "tre" library
#

find_path ( TRE_INCLUDE_DIRS NAMES tre/tre.h )
find_library ( TRE_LIBRARIES NAMES tre )

# Handle the QUIETLY and REQUIRED arguments and set TRE_FOUND to TRUE if all listed variables are TRUE.
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args ( TRE DEFAULT_MSG TRE_LIBRARIES TRE_INCLUDE_DIRS )

if ( TRE_FOUND )
  MESSAGE( STATUS "Tre includes: ${TRE_INCLUDE_DIRS}" )
  MESSAGE( STATUS "Tre libraries: ${TRE_LIBRARIES}" )
else ( TRE_FOUND )
  message( STATUS "Tre library not found" )
endif ( TRE_FOUND )



