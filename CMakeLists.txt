################################################################################
# RAnalysis
################################################################################

if($ENV{AnalysisBase_SET_UP})
  set(STANDALONE_BUILD 0)
  set(ATLAS_BUILD 1)  
else()
  set(STANDALONE_BUILD 1)
  set(ATLAS_BUILD 0)
endif()

if(ATLAS_BUILD OR NOT STANDALONE_BUILD)
  include(./cmake/atlas.cmake)
endif()

if(STANDALONE_BUILD OR NOT ATLAS_BUILD)
  include(./cmake/standalone.cmake)
endif()