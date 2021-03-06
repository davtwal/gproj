// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : GLFWControl.cpp
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

#include "render/Vulkan.h"
#include "render/GLFWControl.h"
#include "render/GLFWWindow.h"
#include "util/Trace.h"
#include <cassert>

namespace dw {
  void GLFWControl::ErrorCB(int err, const char* desc) {
    Trace::Error << "GLFW Error #" << err << ": " << desc << Trace::Stop;
  }

  void GLFWControl::FramebufferCB(GLFWwindow* window, int x, int y) {
    auto win = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    assert(win);

    Trace::All << "Window FramebufferCB called" << Trace::Stop;
  }

  void GLFWControl::KeyCB(GLFWwindow* window, int key, int sc, int action, int mods) {
    auto win = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    assert(win);

    if (action == GLFW_PRESS)
      win->m_inputHandler->onKeyPress(key);
    else win->m_inputHandler->onKeyRelease(key);
  }

  void GLFWControl::MouseButtonCB(GLFWwindow* window, int button, int action, int mods) {
    auto win = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    assert(win);

    if (action == GLFW_PRESS)
      win->m_inputHandler->onMousePress(button);
    else win->m_inputHandler->onMouseRelease(button);
  }

  void GLFWControl::MousePosCB(GLFWwindow* window, double x, double y) {
    auto win = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    assert(win);

    // TODO
    win->m_inputHandler->inputMousePos(x, y);
  }

  void GLFWControl::WindowSizeCB(GLFWwindow* window, int w, int h) {
    auto win = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    assert(win);

    Trace::All << "Window WindowSizeCB called" << Trace::Stop;

    win->m_width = w;
    win->m_height = h;

    if (win->m_resizeCB)
      win->m_resizeCB(win, w, h);
  }

  void GLFWControl::Poll() {
    glfwPollEvents();
  }

  int GLFWControl::Init() {
    if (!glfwInit()) {
      Trace::Error << "GLFW Could not initialize: " << Trace::Stop;
      return 1;
    }

    if (!glfwVulkanSupported()) {
      Trace::Error << "Vulkan not supported" << Trace::Stop;
      return 1;
    }

    glfwSetErrorCallback(ErrorCB);
    return 0;
  }



  void GLFWControl::Shutdown() {
    glfwTerminate();
  }

  std::vector<const char*> GLFWControl::GetRequiredVKExtensions() {
    uint32_t numVkExt = 0;
    const char** vkExt = glfwGetRequiredInstanceExtensions(&numVkExt);

    std::vector<const char*> ret;

    if (vkExt) {
      ret.resize(numVkExt);
      memcpy(ret.data(), vkExt, numVkExt * sizeof(char*));
    }
    return ret;
  }
}
