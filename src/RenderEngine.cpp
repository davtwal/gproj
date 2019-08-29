#include "GL/gl3w.h"

#include "RenderEngine.h"
#include "RenderAPI.h"
#include "Trace.h"

#include <exception>

#include "Window_GLFW.h"

#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#endif

void RenderEngine::init(RenderAPIType api) {
	GLFW::System::Init();

	changeAPI(api);
}

void RenderEngine::shutdown() {
	if (m_window) {
		delete m_window;
		m_window = nullptr;
	}

	if (m_api) {
		delete m_api;
		m_api = nullptr;
	}

	m_currentAPI = API_INVALID;

	GLFW::System::Shutdown();
}

void RenderEngine::changeAPI(RenderAPIType api) {
	if (api == API_INVALID)
		throw std::runtime_error("RenderEngine: Passed API_INVALID to changeAPI");

	if (api != m_currentAPI) {
		// Restart window if changing from OpenGL
		// This is because GLFW window hints don't apply retroactively.
		if (m_window && m_currentAPI == API_GL) {
			delete m_window;
			m_window = nullptr;
		}

		delete m_api;
		m_api = nullptr;

		switch (api) {
		case API_GL:
			// Restart window if switching to OpenGL
			if (m_window) {
				delete m_window;
				m_window = nullptr;
			}

			GLFW::System::EnableOpenGL(4, 0);
			break;
		
		case API_VK:
			GLFW::System::DisableOpenGL();
			//if(!GLFW::System::VulkanSupported)
				throw std::runtime_error("RenderEngine: Invalid or Unsupported API chosen");

			//break;

		case API_VKR:
			GLFW::System::DisableOpenGL();
			throw std::runtime_error("RenderEngine: Invalid or Unsupported API chosen");
			//break;

#ifdef WIN32
		case API_DX11:
			GLFW::System::DisableOpenGL();
			break;

		case API_DX12:
			GLFW::System::DisableOpenGL();
			throw std::runtime_error("RenderEngine: Invalid or Unsupported API chosen");
			//break;

		case API_DXR:
			GLFW::System::DisableOpenGL();
			throw std::runtime_error("RenderEngine: Invalid or Unsupported API chosen");
			//break;
#endif
		default:
			throw std::runtime_error("RenderEngine: Invalid or Unsupported API chosen");
		}
	
		m_window = new GLFW::Window();
		m_currentAPI = api;

		m_api = IRenderAPI::Create(*this, m_currentAPI);
	}
}

bool RenderEngine::update() {
	return m_window->update();
}

IRenderAPI* RenderEngine::getAPI() const {
	return m_api;
}

IWindow* RenderEngine::getWindow() const {
	return m_window;
}

RenderAPIType RenderEngine::getAPIType() const {
	return m_currentAPI;
}