// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : InputHandler.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 13d
// * Last Altered: 2019y 09m 13d
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

#ifndef DW_INPUTHANDLER_GLFW_H
#define DW_INPUTHANDLER_GLFW_H

#include <map>

#ifndef NO_DISCARD
#define NO_DISCARD [[nodiscard]]
#endif

namespace dw {
  class GLFWWindow;

  class InputHandler {
  public:
    using callbackFn = void(*)();
    using moveCBFn = void(*)(double dx, double dy);

    struct KeyCallback {
      callbackFn onPress{nullptr};
      callbackFn onRelease{nullptr};
    };

    struct MouseCallback {
      moveCBFn onMove{ nullptr };
    };

    InputHandler(GLFWWindow& w);

    NO_DISCARD bool getKeyState(int button) const;
    NO_DISCARD bool getMouseState(int button) const;
    void getMousePos(double& out_x, double& out_y) const;

    void registerKeyFunction(int button, KeyCallback callbacks);
    void registerMouseKeyFunction(int button, KeyCallback callbacks);
    void registerMouseMoveFunction(MouseCallback callback);
    void onKeyPress(int button) const;
    void onKeyRelease(int button) const;

    void onMousePress(int button) const;
    void onMouseRelease(int button) const;

  protected:
    using KeyMap = ::std::map<int /*key*/, KeyCallback>;

    void inputKey(int key, int scancode, int action, int mods) const;
    void inputMouseButton(int key, int action, int mods) const;
    void inputMousePos(double x, double y);

    GLFWWindow& m_window;
    MouseCallback m_mouseCB {nullptr};
    KeyMap m_keyMap;
    KeyMap m_mouseMap;
    double m_mouseX{ 0.0f };
    double m_mouseY{ 0.0f };
  };
}

#endif
