// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : GLFWWindow.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 23d
// * Last Altered: 2019y 11m 23d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * Description :

#ifndef DW_WINDOW_GLFW_H
#define DW_WINDOW_GLFW_H

#include "GLFWControl.h"
#include "InputHandler.h"

#include <string>

namespace dw {
  class GLFWWindow {
  public:
    using OnResizeCB = std::function<void(GLFWWindow*, int, int)>;

    GLFWWindow(int width, int height, std::string const& name);
    ~GLFWWindow();

    NO_DISCARD bool shouldClose() const;

    //void show();

    NO_DISCARD unsigned    getWidth() const;
    NO_DISCARD unsigned    getHeight() const;
    NO_DISCARD GLFWwindow* getHandle() const;
  
    //NO_DISCARD bool isRawMouseSupported() const;

    //void setCursorEnabled(bool enabled, bool hidden = false) const;
    //void setRawMouseInput(bool enabled) const;
    void setOnResizeCB(OnResizeCB newCB);
    void setInputHandler(InputHandler* handler);

    void setShouldClose(bool close = true);

  private:
    friend class GLFWControl;

    //void inputKey(int key, int scancode, int action, int mods);
    //void inputMousePos(double x, double y);
    //void inputMouseButton(int button, int action, int mods);
    //void registerResize(int width, int height);

    GLFWwindow*   m_window{nullptr};
    InputHandler* m_inputHandler{nullptr};
    OnResizeCB    m_resizeCB{nullptr};

    unsigned    m_width{0};
    unsigned    m_height{0};
    std::string m_name;
  };
}

#endif
