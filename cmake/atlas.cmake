################################################################################
# Project: ana
################################################################################
find_package( AnalysisBase QUIET )

atlas_subdir( ana )

find_package( Threads REQUIRED )

atlas_add_library( 
  ana ana/*.h src/*.cxx

  PUBLIC_HEADERS ana

  PRIVATE_INCLUDE_DIRS

  PRIVATE_LINK_LIBRARIES Threads::Threads

  # LINK_LIBRARIES Threads::Treads
)
