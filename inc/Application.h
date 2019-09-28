// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Application.h
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

#ifndef DW_APPLICATION_H
#define DW_APPLICATION_H

#include "Renderer.h"
#include "Camera.h"

#include <chrono>
#include <memory>

namespace dw {
  class Application {
  public:
    int parseCommandArgs(int argc, char** argv);
    int run();

  private:
    int initialize();
    int loop();

    int shutdown();

    // TIME VARIABLES
    using ClockType = std::chrono::high_resolution_clock;
    using DurationType = std::chrono::duration<float, std::chrono::seconds::period>;
    ClockType::time_point m_startTime;

    // SCENE VARIABLES
    std::vector<util::ptr<Object>> m_scene;
    util::ptr<Renderer> m_renderer{ nullptr };
    Camera    m_camera;
  };
} // namespace dw
#endif
