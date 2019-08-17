#include "RenderEngine.h"

// OpenGL functions

#include <iostream>

class GLVKWindow : public IWindow {
public:
	static void ErrorCallback(int err, const char* desc) {
		std::cout << "Error #" << err << ": " 
			  << desc << std::endl;
	}

	static void KeyCallback(GLFWwindow* window,
				int key, int scancode,
				int action, int mods) {
		if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	GLVKWindow() {
		
	}

	void* getInternal() const override {
		return m_window;
	}

	GLFWwindow* getInternalGLFW() const {
		return m_window;
	}

private:
	GLFWwindow* m_window {nullptr};
};

class GLRenderEngine : public IRenderEngine {
public:
	bool init() override {
		if(!glfwInit()) {
			throw "Could not initialize GLFW";
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		glfwSetErrorCallback(GLVKWindow::ErrorCallback);

		m_window = new GLVKWindow();
		if(!m_window) {
			throw "Could not open window";
		}

		glfwMakeContextCurrent(m_window->getInternal());
	}

	void shutdown() override {
		delete m_window;
		m_window = nullptr;
	}

	bool update() override {
		return m_window->update();
	}

private:
	GLVKWindow* window() const {
		return reinterpret_cast<GLVKWindow*>(m_window);
	}
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Interface functions
////////////////////////////////////////////////////////////////////////

IRenderEngine* IRenderEngine::Create(unsigned type) {
	switch(type) {
	case RENDER_ENGINE_GL:
		return new GLRenderEngine;
	default:
		// Invalid / unsupported type requested
		return nullptr;
	}
}

IWindow* IRenderEngine::getWindow() {
	return m_window;
}
