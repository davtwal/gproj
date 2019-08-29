#ifndef DW_INPUTHANDLER_H
#define DW_INPUTHANDLER_H

#include <map>

class IWindow;

class IInputHandler {
public:
	struct KeyCallback {
		void(*onPress)() { nullptr };
		void(*onRelease)() { nullptr };
	};

	IInputHandler(IWindow* window);

	virtual bool getKeyState(int button) const = 0;
	virtual bool getMouseState(int button) const = 0;
	virtual void getMousePos(double& out_x, double& out_y) const = 0;

	void registerKeyFunction(int button, KeyCallback callbacks);
	void onPress(int button) const;
	void onRelease(int button) const;

protected:
	using KeyMap = std::map<int /*key*/, KeyCallback>;
	IWindow* m_window;
	KeyMap m_map;
};

#endif
