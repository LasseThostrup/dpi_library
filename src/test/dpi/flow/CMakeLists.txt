set(FLOW_DPI_TEST_SRC

TestFlowSource.h
TestFlowSource.cc
  ) # Adding headers required for portability reasons http://voices.canonical.com/jussi.pakkanen/2013/03/26/a-list-of-common-cmake-antipatterns/
  
add_library(test_dpi_flow ${FLOW_DPI_TEST_SRC})

target_include_directories(test_dpi_flow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_dpi_flow flow_dpi)
