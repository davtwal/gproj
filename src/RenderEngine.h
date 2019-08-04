#ifndef DW_RENDERER_H
#define DW_RENDERER_H

#include "Window.h"

class IRenderEngine {
public:
	// 0 = GL, 1 = VK, 2 = DX11, 3 = DX12
	// Will not be initialized
	static IRenderEngine* Create(unsigned type);

	virtual void init() = 0;
	virtual void shutdown() = 0;

	virtual void update() = 0;

	IWindow* getWindow();

protected:
	IWindow* m_window {nullptr};
};

#endif

