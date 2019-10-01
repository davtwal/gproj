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
#include "Surface.h"
#include "Swapchain.h"
#include "Framebuffer.h"
#include "Trace.h"
#include "Queue.h"
#include "CommandBuffer.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Vertex.h"
#include "Buffer.h"
#include "Mesh.h"

#include "Application.h"

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
