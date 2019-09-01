#ifndef DW_WINDOW_H
#define DW_WINDOW_H

#include "RenderAPIType.h"

class IInputHandler;

class IWindow {
public:
	virtual ~IWindow() {};

	// Polls input. Note that presenting and VSync are not
	// handled by this module. Instead, they are handled by
	// the API that uses them.
	virtual bool update() = 0;
	virtual bool shouldClose() const = 0;

	//void show();
	virtual void init() = 0;
	virtual void shutdown() = 0;

	virtual bool isRawMouseSupported() const = 0;

	virtual void setCursorEnabled(bool enabled, bool hidden = false) const = 0;
	virtual void setRawMouseInput(bool enabled) const = 0;

	virtual void* getHandle() const = 0;

protected:
	virtual IInputHandler* createInputHandler() = 0;

	virtual void inputKey(int key, int scancode, int action, int mods) = 0;
	virtual void inputMousePos(double x, double y) = 0;
	virtual void inputMouseButton(int button, int action, int mods) = 0;

	IInputHandler* m_inputHandler{ nullptr };
};

#endif

