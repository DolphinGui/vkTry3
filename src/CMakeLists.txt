set(SOURCEFILES "main.cpp" "VCEngine.cpp" "Setup.cpp" "vkobjects/ImageBundle.cpp" "vkobjects/Buffer.cpp" "SingleTimeCmdBuffer.cpp")

add_executable(vulcanTest.out ${SOURCEFILES})

target_link_libraries(vulcanTest.out vulkan glfw ${CMAKE_DL_LIBS} Threads::Threads X11 Xxf86vm Xrandr Xi)
