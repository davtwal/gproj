#ifndef DW_RENDERER_H
#define DW_RENDERER_H

#include "Window.h"

class IRenderEngine {
public:    
	// Will not be initialized
	static IRenderEngine* Create(RenderAPI api);

    virtual ~IRenderEngine() {}

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

    // Polls events and input.
    // Retval is true if the program should continue updating. 
	virtual bool update() = 0;

	IWindow* getWindow();

protected:
	IWindow* m_window {nullptr};
    bool m_initialized {false};
};

#endif

