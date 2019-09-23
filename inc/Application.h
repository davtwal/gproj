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
#include <vector>
#include "RenderPass.h"
#include "Utils.h"

namespace dw {
  class PhysicalDevice;
  class VulkanControl;
  class GLFWWindow;
  class InputHandler;
  class LogicalDevice;
  class Surface;
  class Swapchain;
  class CommandPool;

  class IShader;

  class Application {
  public:
    int parseCommandArgs(int argc, char** argv);
    int run();

  private:
    int initialize();
    // INITIALIZATION FUNCTIONS
    void openWindow();
    void setupInstance();
    void setupDevice(PhysicalDevice& device);
    void setupSurface();
    void setupSwapChain();
    void setupRenderpasses();
    void setupSwapChainFrameBuffers();
    void setupShaders();
    void setupPipeline();
    void setupCommandBuffers();

    void setupDebug();
    void fillInstanceExtLayerVecs(std::vector<const char*>& exts, std::vector<const char*>& layers);

    int loop() const;

    int shutdown();
    // SHUTDOWN FUNCTIONS
    void shutdownDebug();

    VulkanControl* m_control{nullptr};
    GLFWWindow*    m_window{nullptr};
    InputHandler*  m_inputHandler{nullptr};
    LogicalDevice* m_device{ nullptr };

    Surface* m_surface{ nullptr };
    Swapchain* m_swapchain{ nullptr };
    CommandPool* m_commandPool{ nullptr };
    RenderPass* m_renderPass{ nullptr };
    IShader* m_triangleVertShader{ nullptr };
    IShader* m_triangleFragShader{ nullptr };

    util::Ref<Queue>* m_graphicsQueue{nullptr};
    std::vector<util::Ref<CommandBuffer>> m_commandBuffers;
  };
} // namespace dw
#endif
