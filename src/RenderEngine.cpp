#include "RenderEngine.h"

// OpenGL functions

#include <iostream>
#include <exception>

#include "GL/gl3w.h"
#include "GLFW/glfw3.h"

#include "Trace.h"

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// WINDOWING: GLFW
////////////////////////////////////////////////////////////////////////

class GLVKWindow : public IWindow {
public:
	static void ErrorCallback(int err, const char* desc) {
		Trace::Report() << "Error #" << err << ": " 
			  << desc << Trace::EndRep();
	}

	static void KeyCallback(GLFWwindow* window,
				int key, int scancode,
				int action, int mods) {                    
		if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
        else Trace::Report() << "Key press: " << key << " " << action << Trace::EndRep();
	}

    GLVKWindow()
        : m_window(nullptr)
    {}

    ~GLVKWindow() override {
        if(getInternal())
            shutdown();
    }

    void init() override {
        m_window = glfwCreateWindow(640, 400, "GLVK Window", nullptr, nullptr);

        if(!m_window)
            throw "GLFW Error: Could not create window.";

        glfwSetKeyCallback(m_window, GLVKWindow::KeyCallback);
    }

    void shutdown() override {
        if(m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    bool update() override {
        if(m_window) {
            glfwPollEvents();
            return !glfwWindowShouldClose(m_window);
        }

        Trace::Report() << "Update called on non-initialized window." << Trace::EndRep();
        return false;
    }

    bool shouldClose() override {
        // Checking necessary?
        return !m_window || glfwWindowShouldClose(m_window);
    }

    void show() override {
        if(m_window) // Checking necessary?
            glfwShowWindow(m_window);
    }

	void* getInternal() const override {
		return m_window;
	}

	GLFWwindow* getInternalGLFW() const {
		return m_window;
	}

private:
	GLFWwindow* m_window {nullptr};
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// RENDERING: OPENGL
////////////////////////////////////////////////////////////////////////

class GLRenderEngine : public IRenderEngine {
public:

    ~GLRenderEngine() override {
        if(m_window)
            shutdown();
    }

	void init() override {
		if(!glfwInit()) {
			throw "Could not initialize GLFW";
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		glfwSetErrorCallback(GLVKWindow::ErrorCallback);

        Trace::Report() << "Initializing window..." << Trace::EndRep();

		m_window = new GLVKWindow();
		if(!m_window) {
			throw "Could not open window";
		}

        m_window->init();

		glfwMakeContextCurrent(window()->getInternalGLFW());

        // gl3wInit returns 0 on success
        if(gl3wInit()) {
            throw "Could not initialize gl3w";
        }
	}

	void shutdown() override {
        m_window->shutdown();
        
		delete m_window;
		m_window = nullptr;

        glfwTerminate();
	}

	bool update() override {
		return m_window->update();
	}

private:
	GLVKWindow* window() const {
		return reinterpret_cast<GLVKWindow*>(m_window);
	}
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// WINDOWING: WINDOWS
////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <Windows.h>

class DXWindow : public IWindow {
public:
	~DXWindow() override {
	}
	
	void init() override;
	void shutdown() override;
	bool update() override;
	bool shouldClose() override;
	void show() override;
	
	void* getInternal() const override;
	
	HWND getInternalHWND() const;
	
private:
	friend class DXWindowManager;

	HWND m_window {nullptr};
	bool m_shouldClose {false};
};

class DXWindowManager {
public:
	static LRESULT CALLBACK DXWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		if(s_dxWindow) {
			switch(msg) {
				case WM_CLOSE:
				case WM_DESTROY:
					s_dxWindow->m_shouldClose = true;
					PostQuitMessage(0);
					break;
			}
		}
		return DefWindowProc(hwnd, msg, wp, lp);
	}

	static constexpr const char* WINDOW_CLASS_NAME = "Windows Class";

	void init() {
		if(s_dxWindow)
			throw "Attempted to initialize a DX window while there is already a DX window alive";
		
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize 			= sizeof(wc);
		wc.lpfnWndProc 		= DXWindowProc;
		wc.lpszClassName 	= WINDOW_CLASS_NAME;
		wc.hInstance 		= GetModuleHandle(NULL);
		wc.hIcon			= LoadIcon(NULL, IDI_ERROR);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			// Double clicks seem to be in an interesting spot in X11/GLFW land, so
			// I'm not 100% sure if I should put it in the class styles here.
			// Leaving it in for now just in case it becomes important.
			// Requires more research
			
		if(!RegisterClassEx(&wc))
			throw "Could not register windows class for the DXWindow.";
	}
	
	void shutdown() {
		if(s_dxWindow)
			s_dxWindow->shutdown();
		
		s_dxWindow = nullptr;
		UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(NULL));
	}
	
	void registerWindow(DXWindow* window) {
		if(s_dxWindow)
			throw "Attempted to register new DXWindow when there is already one open";
		
		s_dxWindow = window;
	}
		
	
private:
	// TODO: If we want multiple windows as a possibility, then this needs to go
	// turn into a tree based on HWND as the key?
	static DXWindow* s_dxWindow;
};

DXWindow* DXWindowManager::s_dxWindow = nullptr;

void DXWindow::init() {
	const HWND 	desktopWindow = GetDesktopWindow();
	RECT		desktopRect;
	
	GetWindowRect(desktopWindow, &desktopRect);
		
	int posX = 0, posY = 0;
		
	//if(!beginFullscreen)
	//{
		posX = (GetSystemMetrics(SM_CXSCREEN) - 500);//m_width) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - 500);//m_height) / 2;
	//}
	
	m_window = CreateWindowEx(
		0, // WS_EX_ACCEPTFILES,
		DXWindowManager::WINDOW_CLASS_NAME,
		"Window",
		WS_OVERLAPPEDWINDOW,
		posX,
		posY,
		500,
		500,
		NULL, NULL,
		GetModuleHandle(NULL),
		NULL
	);
		
	if(!m_window) {
		throw "Could not create WindowEx with the Windows Class.";
	}
}

void DXWindow::shutdown()  {
	// If fullscreen, then call this:
	// ChangeDisplaySettings(NULL, 0);
	if(m_window)
		DestroyWindow(m_window);
		
	m_window = nullptr;
}

bool DXWindow::update() {
	return shouldClose();
}

bool DXWindow::shouldClose() {
	return m_shouldClose;
}

void DXWindow::show() {
	ShowWindow(m_window, SW_SHOW);
	SetForegroundWindow(m_window);
	SetFocus(m_window);
}

void* DXWindow::getInternal() const {
	return m_window;
}

HWND DXWindow::getInternalHWND() const {
	return m_window;
}

class DX11RenderEngine : public IRenderEngine {
public:
	~DX11RenderEngine() override {
		//if(m_window)
		//	shutdown();
	}
	
	void init() override {
		m_winman.init();
		
		m_window = new DXWindow;
		if(!m_window)
			throw "Could not create new DXWindow";
		
		m_window->init();
		
		m_winman.registerWindow(m_window);
	}
	
	void shutdown() override {
		// automatically shuts down any and all windows associated with 
		// this window manager
		m_winman.shutdown();
		
		m_window = nullptr;
	}
	
	bool update() override {
		return m_window->update();
	}
	
private:
	DXWindowManager m_winman;
	DXWindow* m_window {nullptr};
};

#endif

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Interface functions
////////////////////////////////////////////////////////////////////////

IRenderEngine* IRenderEngine::Create(RenderAPI api) {
	switch(api) {
	case API_GL:
		return new GLRenderEngine;
#ifdef _WIN32
	case API_DX11:
		return new DX11RenderEngine;
#endif
	default:
		// Invalid / unsupported type requested
		return nullptr;
	}
}

RenderAPI IWindow::getType() const {
    return m_type;
}

IWindow* IRenderEngine::getWindow() {
	return m_window;
}

IWindow::~IWindow() {}

