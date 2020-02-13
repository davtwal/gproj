// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj2 : Main.cpp
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 13d
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

// All of these includes need to be here because otherwise unique_ptr will yell at me
#include "render/Surface.h"
#include "render/Swapchain.h"
#include "render/Framebuffer.h"
#include "render/Queue.h"
#include "render/CommandBuffer.h"
#include "render/RenderPass.h"
#include "render/Shader.h"
#include "render/Vertex.h"
#include "render/Buffer.h"
#include "render/Mesh.h"

#include "app/Application.h"
#include "util/Trace.h"

#include <exception>
#include <iostream>
using namespace dw;
int main(int argc, char** argv) {
  Application app;

  try {
    if (app.parseCommandArgs(argc, argv) == EXIT_FAILURE || app.run() == EXIT_FAILURE)
      return EXIT_FAILURE;
  }
  catch (std::exception& e) {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
    getchar();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
