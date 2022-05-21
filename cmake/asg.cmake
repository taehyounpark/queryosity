################################################################################
# Project: ana
################################################################################
find_package( AnalysisBase QUIET)

if(${AnalysisBase_FOUND})
  set(STANDALONE_BUILD 0)
  set(ATLAS_BUILD 1)  

  atlas_subdir( ana )
  find_package( Threads REQUIRED )
  atlas_add_library( ana ${anaSources} ${anaDictSource}
    PUBLIC_HEADERS ana
    LINK_LIBRARIES Threads::Threads
  )
  
else()
  set(STANDALONE_BUILD 1)
  set(ATLAS_BUILD 0)
endif()