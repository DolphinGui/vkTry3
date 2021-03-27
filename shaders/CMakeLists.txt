add_custom_target(shader.vert ALL DEPENDS
            COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/shader.vert -o vert.spv
            DEPENDS shader.vert
            )
add_custom_target(shader.frag ALL DEPENDS
            COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/shader.frag -o frag.spv
            DEPENDS shader.frag
            )

