#ifndef DW_RENDERAPI_H
#define DW_RENDERAPI_H

#include "RenderEngine.h"

class IRenderAPI {
public:
	// Will not be initialized
	static IRenderAPI* Create(RenderEngine& engine, RenderAPIType api);

	IRenderAPI(RenderEngine& engine);
	virtual ~IRenderAPI() {}

	// Initialize function
	// This initializes multiple things:
	//   1: The window
	//   2: The rendering API (GL, VK, ...) creating a context
	// If an error occurs, throws an error message.
	virtual void init() = 0;

	// Shutdown function
	// Closes all open windows, deletes contexts, frees resources, ...
	// Only to be called when program is shutting down.
	virtual void shutdown() = 0;

protected:
	RenderEngine& m_engine;
	bool m_initialized{ false };
};

#endif
