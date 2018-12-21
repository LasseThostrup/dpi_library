set(FLOW_SRC
  FlowHandle.h
  FlowRegistryClient.cc
  FlowRegistryClient.h
  FlowRegistryServer.cc
  FlowRegistryServer.h
  FlowSource.h
  FlowTarget.h
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
add_library(flow_dpi ${FLOW_SRC})
target_include_directories(flow_dpi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(flow_dpi
  memory_dpi 
  )