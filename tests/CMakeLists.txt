add_executable(vulcanTest.out vulkanTutorial.cpp)
target_link_libraries(vulcanTest.out 
  vulkan 
  glfw 
  ${CMAKE_DL_LIBS} 
  Threads::Threads 
  X11 
  Xxf86vm 
  Xrandr 
  Xi 
  vaccumCleaner)
