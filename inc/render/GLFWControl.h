// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : GLFW.h
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

#ifndef DW_GLFW_H
#define DW_GLFW_H

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <vector>

namespace dw {
  class GLFWControl {
  public:
    static void ErrorCB(int err, const char* desc);
    static void KeyCB(GLFWwindow* window, int key, int sc, int action, int mods);
    static void MousePosCB(GLFWwindow* window, double x, double y);
    static void MouseButtonCB(GLFWwindow* window, int button, int action, int mods);
    //static void MouseEnterCB(GLFWwindow* window, int entered);
    //static void ScrollCB(GLFWwindow* window, double xoff, double yoff);
    static void FramebufferCB(GLFWwindow* window, int x, int y);
    //static void IconifyCB(GLFWwindow* window, int icon);
    //static void FocusCB(GLFWwindow* window, int focus);
    //static void CharCB(GLFWwindow* window, unsigned codepoint);
    //static void DropCB(GLFWwindow* window, int count, const chat** paths); // File dropping!
    static void WindowSizeCB(GLFWwindow* window, int w, int h);

    static int Init();
    static void Shutdown();
    static void Poll();

    static std::vector<const char*> GetRequiredVKExtensions();

  private:
    
    static int s_glfwInitialized;
  };
}

#endif
