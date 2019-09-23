// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : GLFWWindow.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
//  Created     : 2019y 09m 15d
// * Last Altered: 2019y 09m 15d
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

#include "GLFWWindow.h"

#include <stdexcept>

namespace dw {
  GLFWWindow::GLFWWindow(int w, int h, std::string const& name)
    : m_width(w),
      m_height(h),
      m_name(name) {

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(w, h, name.c_str(), nullptr, nullptr);

    if (!m_window)
      throw std::bad_alloc();

    glfwSetKeyCallback(m_window, GLFWControl::KeyCB);
    glfwSetMouseButtonCallback(m_window, GLFWControl::MouseButtonCB);
    glfwSetCursorPosCallback(m_window, GLFWControl::MousePosCB);
    glfwSetFramebufferSizeCallback(m_window, GLFWControl::FramebufferCB);
    glfwSetWindowSizeCallback(m_window, GLFWControl::WindowSizeCB);
    glfwSetWindowUserPointer(m_window, this);
  }

  bool GLFWWindow::shouldClose() const {
    return m_window ? glfwWindowShouldClose(m_window) : true;
  }

  GLFWwindow* GLFWWindow::getHandle() const {
    return m_window;
  }

  unsigned GLFWWindow::getWidth() const {
    return m_width;
  }

  unsigned GLFWWindow::getHeight() const {
    return m_height;
  }

  void GLFWWindow::setInputHandler(InputHandler* handler) {
    m_inputHandler = handler;
  }

  void GLFWWindow::setOnResizeCB(OnResizeCB newCB) {
    m_resizeCB = newCB;
  }

  GLFWWindow::~GLFWWindow() {
    if (m_window)
      glfwDestroyWindow(m_window);
  }
}
