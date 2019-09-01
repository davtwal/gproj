#ifndef DW_INPUTHANDLER_GLFW_H
#define DW_INPUTHANDLER_GLFW_H

#include "InputHandler.h"

namespace GLFW {
	class Window;
	class InputHandler : public IInputHandler {
	public:
		InputHandler(Window* window);

		bool getKeyState(int button) const override;
		bool getMouseState(int button) const override;
		void getMousePos(double& out_x, double& out_y) const override;
	};
}

#endif
