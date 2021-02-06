CFLAGS = -std=c++17
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

vulcanTest: src/main.cpp src/VCEngine.cpp
	g++ $(CFLAGS) -g -o vulcanTest src/main.cpp src/VCEngine.cpp $(LDFLAGS)

vcold: src/main.cpp src/engine.hpp
	g++ $(CFLAGS) -g -o vcold src/vcold.cpp $(LDFLAGS)

vert.spv: src/shaders/shader.vert
	glslc src/shaders/shader.vert -o shaders/vert.spv

frag.spv: src/shaders/shader.frag
	glslc src/shaders/shader.frag -o shaders/frag.spv

.PHONY: test clean all

all: vulcanTest vert.spv frag.spv

test: vulcanTest vert.spv frag.spv
	./vulcanTest

clean:
	rm -f vulcanTest vert.spv frag.spv
