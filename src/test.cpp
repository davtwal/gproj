#include <iostream>
#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include "RenderEngine.h"

int main() {
	using std::cout;
	using std::endl;
	
	cout << "Starting up program..." << endl;
	
	IRenderEngine* renderer = IRenderEngine::Create(RENDER_ENGINE_GL);

	if(!renderer) {
		std::cout << "Could not create render engine." << endl;
		return -1;
	}

    renderer->init();

	auto window = glfwCreateWindow(640, 480, "Hello Window!", NULL, NULL);
	if(!window) {
		std::cout << "Error: Could not create window!" << std::endl;

		glfwTerminate();
		return -1;
	}

	std::cout << "Successfully created window!" << std::endl;

	glfwMakeContextCurrent(window);

	if(gl3wInit()) {
		std::cout << "Failed loading gl3w?" << std::endl;
		return -1;
	}

	std::cout << "Successfully initialized gl3w." << std::endl;

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) <<std::endl;

	while(!glfwWindowShouldClose(window)) {
		;
	}

	std::cout << "Successfully initialized GLFW!" << std::endl;
	
	glfwShowWindow(window);

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}
