file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

if(SRC_FILES)
  # If there are .cpp files, create a static library
  add_library(cpputils STATIC ${SRC_FILES})
  target_include_directories(cpputils PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
else()
  # If there are no .cpp files, create an interface library
  add_library(cpputils INTERFACE)
  target_include_directories(cpputils INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()

