CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

vulcanTest: src/main.cpp
	g++ $(CFLAGS) -o vulcanTest src/main.cpp $(LDFLAGS)

vert.spv: src/shaders/shader.vert
	glslc src/shaders/shader.vert -o shaders/vert.spv

frag.spv: src/shaders/shader.frag
	glslc src/shaders/shader.frag -o shaders/frag.spv

.PHONY: test clean

test: vulcanTest
	./vulcanTest

clean:
	rm -f vulcanTest
