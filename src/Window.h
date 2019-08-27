#ifndef DW_WINDOW_H
#define DW_WINDOW_H

#include "RenderAPIType.h"

class IWindow {
public:
    virtual ~IWindow();
    
    // Initializes a window.
	virtual void init() = 0;
	virtual void shutdown() = 0;

	virtual bool update() = 0;
	virtual bool shouldClose() = 0;

	virtual void show() = 0;

	//static IWindow* create(RenderAPI type);

    // TODO: These functions
    /*
     * virtual ??? focus();
     * virtual ??? getInput();
     * virtual ??? resize();
     */

	virtual void* getInternal() const = 0;
	RenderAPI getType() const;

protected:
	RenderAPI m_type {API_INVALID};
};

#endif

