#ifndef DW_RENDERER_H
#define DW_RENDERER_H

#include "GLFW.h"
#include "Window.h"
#include "RenderAPIType.h"

class IRenderAPI;

class RenderEngine {
public:
	void init(RenderAPIType api);
	void shutdown();

	bool update();

	void changeAPI(RenderAPIType api);

	IWindow* getWindow() const;
	IRenderAPI* getAPI() const;

	RenderAPIType getAPIType() const;

private:
	IWindow*	  m_window{ nullptr };
	RenderAPIType m_currentAPI{ API_INVALID };
	IRenderAPI*	  m_api{ nullptr };
};

#endif

