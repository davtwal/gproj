#include "InputHandler.h"
#include <cassert>

IInputHandler::IInputHandler(IWindow* window) : m_window(window) {}

void IInputHandler::registerKeyFunction(int button, KeyCallback cb) {
	assert(button > 0);
	assert(cb.onPress != NULL || cb.onRelease != NULL);

	m_map.insert_or_assign(button, cb);
}

void IInputHandler::onPress(int button) const {
	try {
		auto cb = m_map.at(button);
		if (cb.onPress)
			cb.onPress();
	}
	catch (std::exception&) {}
}

void IInputHandler::onRelease(int button) const {
	try {
		auto cb = m_map.at(button);
		if (cb.onRelease)
			cb.onRelease();
	}
	catch (std::exception&) {}
}