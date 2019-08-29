#include "Window_GLFW.h"
#include "InputHandler_GLFW.h"

#include "Trace.h"

namespace GLFW {
	IInputHandler* Window::createInputHandler() {
		IInputHandler* ret = new InputHandler(this);

		if (!ret)
			throw "Could not create input handler";

		return ret;
	}

	Window::Window() {
		init();
	}

	Window::~Window() {
		shutdown();
	}

	void Window::init() {
		if (!m_window) {
			m_window = glfwCreateWindow(640, 400, "GLFW Window", NULL, NULL);

			if (!m_window)
				throw "Could not create GLFW window";

			// Really cool thing about GLFW: User pointers.
			// You can use these to give a window a pointer that stays with the GLFWwindow
			// for its entire existance without ever being modified by GLFW itself.
			// So you can give GLFW a container pointer to the GLFWwindow which stays with it
			// even during callbacks. This is how I track input from each window without
			// needing to use polling.
			glfwSetWindowUserPointer(m_window, this);
			glfwSetKeyCallback(m_window, System::KeyCB);
			glfwSetInputMode(m_window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

			m_inputHandler = createInputHandler();
		}
	}

	bool Window::update() {
		if (m_window) {
			// glfwSwapBuffers(m_window);
			glfwPollEvents();

			// VSync and presenting/buffer swapping is handled by API

			return !glfwWindowShouldClose(m_window);
		}

		return false;
	}

	bool Window::shouldClose() const {
		return glfwWindowShouldClose(m_window);
	}

	void Window::shutdown() {
		if (m_window) {
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}
	}

	void* Window::getHandle() const {
		return m_window;
	}

	bool Window::isRawMouseSupported() const {
		return glfwRawMouseMotionSupported();
	}

	void Window::setRawMouseInput(bool enabled) const {
		if (isRawMouseSupported())
			glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	void Window::setCursorEnabled(bool enabled, bool hidden) const {
		if (enabled)
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else if (hidden)
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		else
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	static InputHandler* inputGLFW(IInputHandler* input) {
		return reinterpret_cast<InputHandler*>(input);
	}

	void Window::inputKey(int key, int scancode, int action, int mods) {
		// TODO remove this once normal exit is planned
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		if (action == GLFW_PRESS) {
			m_inputHandler->onPress(key);
		}

		else m_inputHandler->onRelease(key);
	}

	void Window::inputMousePos(double /*x*/, double /*y*/) {
		//inputGLFW(m_inputHandler)->onMouseMove(x, y);
	}

	void Window::inputMouseButton(int button, int action, int /*mods*/) {
		if (action == GLFW_PRESS) {
			m_inputHandler->onPress(button);
		}

		else
			m_inputHandler->onRelease(button);
	}


}