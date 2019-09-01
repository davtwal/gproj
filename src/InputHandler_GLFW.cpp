#include "InputHandler_GLFW.h"
#include "Window_GLFW.h"

namespace GLFW {
	static GLFWwindow* getWinHandle(IWindow* window) {
		return reinterpret_cast<GLFWwindow*>(window->getHandle());
	}

	InputHandler::InputHandler(Window* window)
		: IInputHandler(window) {}

	bool InputHandler::getKeyState(int key) const {
		return glfwGetKey(getWinHandle(m_window), key);
	}

	bool InputHandler::getMouseState(int button) const {
		return glfwGetMouseButton(getWinHandle(m_window), button);
	}

	void InputHandler::getMousePos(double& out_x, double& out_y) const {
		glfwGetCursorPos(getWinHandle(m_window), &out_x, &out_y);
	}
}