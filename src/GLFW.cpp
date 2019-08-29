#include "GLFW.h"
#include "Trace.h"

#include "Window_GLFW.h"

#include <cassert>

namespace GLFW {
	unsigned System::s_glMajor = 4;
	unsigned System::s_glMinor = 0;
	bool System::s_vulkanSupported = false;

	unsigned const& System::GLMajor = System::s_glMajor;
	unsigned const& System::GLMinor = System::s_glMinor;
	bool const& System::VulkanSupported = System::s_vulkanSupported;

	void System::ErrorCB(int err, const char* desc) {
		Trace::Report() << "GLFW Error #" << err << ": "
			<< desc << Trace::EndRep();
	}

	void System::KeyCB(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
		Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

		assert(window);
		window->inputKey(key, scancode, action, mods);
	}

	void System::MousePosCB(GLFWwindow* glfw_window, double x, double y) {
		Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
		assert(window);

		window->inputMousePos(x, y);
	}

	void System::MouseButtonCB(GLFWwindow* glfw_window, int button, int action, int mods) {
		Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
		assert(window);

		window->inputMouseButton(button, action, mods);
	}

	void System::Init() {
		if (!glfwInit())
			throw "Could not initialize GLFW";

		glfwSetErrorCallback(ErrorCB);

		// Query vulkan support
		if (glfwVulkanSupported()) {
			s_vulkanSupported = true;
		}
	}

	void System::Shutdown() {
		glfwTerminate();
	}

	void System::EnableOpenGL(int major, int minor) {
		s_glMajor = major;
		s_glMinor = minor;

		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	}

	void System::DisableOpenGL() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}
}