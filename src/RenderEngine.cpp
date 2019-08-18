#include "RenderEngine.h"

// OpenGL functions

#include <iostream>
#include <exception>

#include "GL/gl3w.h"
#include "GLFW/glfw3.h"

#include "Trace.h"

class GLVKWindow : public IWindow {
public:
	static void ErrorCallback(int err, const char* desc) {
		Trace::Report() << "Error #" << err << ": " 
			  << desc << Trace::EndRep();
	}

	static void KeyCallback(GLFWwindow* window,
				int key, int scancode,
				int action, int mods) {                    
		if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
        else Trace::Report() << "Key press: " << key << " " << action << Trace::EndRep();
	}

    GLVKWindow()
        : m_window(nullptr)
    {}

    ~GLVKWindow() override {
        if(getInternal())
            shutdown();
    }

    void init() override {
        m_window = glfwCreateWindow(640, 400, "GLVK Window", nullptr, nullptr);

        if(!m_window)
            throw "GLFW Error: Could not create window.";

        glfwSetKeyCallback(m_window, GLVKWindow::KeyCallback);
    }

    void shutdown() override {
        if(m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    bool update() override {
        if(m_window) {
            glfwPollEvents();
            return !glfwWindowShouldClose(m_window);
        }

        Trace::Report() << "Update called on non-initialized window." << Trace::EndRep();
        return false;
    }

    bool shouldClose() override {
        // Checking necessary?
        return !m_window || glfwWindowShouldClose(m_window);
    }

    void show() override {
        if(m_window) // Checking necessary?
            glfwShowWindow(m_window);
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

    ~GLRenderEngine() override {
        if(m_window)
            shutdown();
    }

	void init() override {
		if(!glfwInit()) {
			throw "Could not initialize GLFW";
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		glfwSetErrorCallback(GLVKWindow::ErrorCallback);

        Trace::Report() << "Initializing window..." << Trace::EndRep();

		m_window = new GLVKWindow();
		if(!m_window) {
			throw "Could not open window";
		}

        m_window->init();

		glfwMakeContextCurrent(window()->getInternalGLFW());

        // gl3wInit returns 0 on success
        if(gl3wInit()) {
            throw "Could not initialize gl3w";
        }
	}

	void shutdown() override {
        m_window->shutdown();
        
		delete m_window;
		m_window = nullptr;

        glfwTerminate();
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

IRenderEngine* IRenderEngine::Create(RenderAPI api) {
	switch(api) {
	case API_GL:
		return new GLRenderEngine;
	default:
		// Invalid / unsupported type requested
		return nullptr;
	}
}

RenderAPI IWindow::getType() const {
    return m_type;
}

IWindow* IRenderEngine::getWindow() {
	return m_window;
}

IWindow::~IWindow() {}

