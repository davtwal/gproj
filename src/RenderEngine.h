#ifndef DW_RENDERER_H
#define DW_RENDERER_H

#include "Window.h"

#define RENDER_ENGINE_GL 0u
#define RENDER_ENGINE_VK 1u

#ifdef _WIN32
	#define RENDER_ENGINE_DX11 2u
	#define RENDER_ENGINE_DX12 3u
#endif

class IRenderEngine {
public:
	// 0 = GL, 1 = VK, 2 = DX11, 3 = DX12
	// Will not be initialized
	static IRenderEngine* Create(unsigned type);

    // Initialize function
    // This initializes multiple things:
    //   1: The window
    //   2: The rendering API (GL, VK, ...) creating a context
	virtual void init() = 0;

    // Shutdown function
    // Closes all open windows, deletes contexts, frees resources, ...
    // Only to be called when program is shutting down.
	virtual void shutdown() = 0;

    // 
	virtual void update() = 0;

	IWindow* getWindow();

protected:
	IWindow* m_window {nullptr};
};

#endif

