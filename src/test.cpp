#include <iostream>
#include "GLFW/glfw3.h"

int main() {
	std::cout << "Hello world!" << std::endl;
	
	if(!glfwInit()) {
		std::cout << "Error: Could not init GLFW3" << std::endl;
		std::exit(1);
	}

	auto window = glfwCreateWindow(640, 480, "Hello Window!", NULL, NULL);
	if(!window) {
		std::cout << "Error: Could not create window!" << std::endl;

		glfwTerminate();
		return -1;
	}

	std::cout << "Successfully created window!" << std::endl;

	glfwMakeContextCurrent(window);

	while(!glfwWindowShouldClose(window)) {
		;
	}

	std::cout << "Successfully initialized GLFW!" << std::endl;
	
	glfwShowWindow(window);

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}
