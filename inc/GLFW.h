/*
	GLFW.h
	Contains functions relevant to GLFW, and just GLFW.
*/

#ifndef DW_GLFW_H
#define DW_GLFW_H

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

namespace GLFW {
	class System {
	public:
		static void ErrorCB(int err, const char* desc);
		static void KeyCB(GLFWwindow* window, int key, int sc, int action, int mods);
		static void MousePosCB(GLFWwindow* window, double x, double y);
		static void MouseButtonCB(GLFWwindow* window, int button, int action, int mods);
		//static void MouseEnterCB(GLFWwindow* window, int entered);
		//static void ScrollCB(GLFWwindow* window, double xoff, double yoff);
		//static void FramebufferCB(GLFWwindow* window, int x, int y);
		//static void IconifyCB(GLFWwindow* window, int icon);
		//static void FocusCB(GLFWwindow* window, int focus);
		//static void CharCB(GLFWwindow* window, unsigned codepoint);
		//static void DropCB(GLFWwindow* window, int count, const chat** paths); // File dropping!

		// TODO: Bunch of joystick stuff if I reeaaallly want to do that

		static void EnableOpenGL(int major, int minor);
		static void DisableOpenGL();

		static void Init();
		static void Shutdown();

	private:
		static unsigned s_glMajor;
		static unsigned s_glMinor;
		static bool s_vulkanSupported;

	public:
		static unsigned const& GLMajor;
		static unsigned const& GLMinor;
		static bool const& VulkanSupported;
	};
}

#endif
