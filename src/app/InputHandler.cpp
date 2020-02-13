// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : InputHandler.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 16d
// * Last Altered: 2019y 09m 16d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#include "app/InputHandler.h"
#include "render/GLFWWindow.h"
#include "util/Trace.h"

#include <stdexcept>

namespace dw {
  InputHandler::InputHandler(GLFWWindow& window)
    : m_window(window) {
  }

  void InputHandler::registerKeyFunction(int button, callbackFn onPress, callbackFn onRelease) {
    m_keyMap.insert_or_assign(button, KeyCallback{onPress, onRelease});
  }

  void InputHandler::registerMouseKeyFunction(int button, callbackFn onPress, callbackFn onRelease) {
    m_mouseMap.insert_or_assign(button, KeyCallback{ onPress, onRelease });
  }

  void InputHandler::registerMouseMoveFunction(moveCBFn callback) {
    m_mouseCB = { callback };
  }

  void InputHandler::getMousePos(double& out_x, double& out_y) const {
    out_x = m_mouseX;
    out_y = m_mouseY;
  }

  bool InputHandler::getKeyState(int button) const {
    return glfwGetKey(m_window.getHandle(), button);
  }

  bool InputHandler::getMouseState(int button) const {
    return glfwGetMouseButton(m_window.getHandle(), button);
  }

  void InputHandler::onKeyPress(int button) const {
    callbackFn pFunc = nullptr;
    try {
      pFunc = m_keyMap.at(button).onPress;
    }
    catch (std::out_of_range&) {
      return;
    }

    if (pFunc)
      pFunc();
  }

  void InputHandler::onKeyRelease(int button) const {
    callbackFn pFunc = nullptr;
    try {
      pFunc = m_keyMap.at(button).onRelease;
    }
    catch (std::out_of_range&) {
      return;
    }

    if (pFunc)
      pFunc();
  }

  void InputHandler::onMousePress(int button) const {
    callbackFn pFunc = nullptr;
    try {
      pFunc = m_mouseMap.at(button).onPress;
    }
    catch (std::out_of_range&) {
      return;
    }

    if (pFunc)
      pFunc();
  }

  void InputHandler::onMouseRelease(int button) const {
    callbackFn pFunc = nullptr;
    try {
      pFunc = m_mouseMap.at(button).onRelease;
    }
    catch (std::out_of_range&) {
      return;
    }

    if (pFunc)
      pFunc();
  }

  void InputHandler::inputKey(int key, int, int action, int) const {
    switch (action) {
      case GLFW_PRESS:
        onKeyPress(key);
      case GLFW_RELEASE:
        onKeyRelease(key);
      default:
        Trace::All << "Somehow a key did something other than press/release" << Trace::Stop;
    }
  }

  void InputHandler::inputMouseButton(int key, int action, int) const {
    switch (action) {
      case GLFW_PRESS:
        onMousePress(key);
      case GLFW_RELEASE:
        onMouseRelease(key);
      default:
        Trace::All << "Somehow an mkey did something other than press/release" << Trace::Stop;
    }
  }

  void InputHandler::inputMousePos(double x, double y) {
    double dx = x - m_mouseX;
    double dy = y - m_mouseY;

    m_mouseX = x;
    m_mouseY = y;

    if (m_mouseCB.onMove)
      m_mouseCB.onMove(dx, dy);
  }


}
