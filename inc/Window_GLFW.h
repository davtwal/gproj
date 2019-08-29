#ifndef DW_WINDOW_GLFW_H
#define DW_WINDOW_GLFW_H

#include "Window.h"
#include "GLFW.h"

namespace GLFW {
	class Window : public IWindow {
	public:
		Window();
		~Window();

		// Polls input. Note that presenting and VSync are not
		// handled by this module. Instead, they are handled by
		// the API that uses them.
		bool update() override;
		bool shouldClose() const override;

		//void show();

		// Init is automatically called in constructor.
		// Shutdown is automatically called in destructor.
		// These are here in case a window needs to be restarted.
		void init() override;
		void shutdown() override;

		bool isRawMouseSupported() const override;

		void setCursorEnabled(bool enabled, bool hidden = false) const override;
		void setRawMouseInput(bool enabled) const override;

		void* getHandle() const override;

	private:
		friend class System;
		virtual IInputHandler* createInputHandler() override;

		void inputKey(int key, int scancode, int action, int mods) override;
		void inputMousePos(double x, double y) override;
		void inputMouseButton(int button, int action, int mods) override;

		GLFWwindow* m_window{ nullptr };
	};
}

#endif
