#include "RenderAPI.h"

#include <cassert>
#include <exception>

#include "GL/gl3w.h"

//#include "GLFW.h"
#include "Window_GLFW.h"

namespace GL {
	class RenderAPI : public IRenderAPI {
	public:
		RenderAPI(RenderEngine& engine);
		~RenderAPI() override;

		void init() override;
		void shutdown() override;

		//void create();

	private:
		static bool s_gl3wInitialized;
	};

	bool RenderAPI::s_gl3wInitialized = false;

	RenderAPI::RenderAPI(RenderEngine& engine) : IRenderAPI(engine) {
		init();
	}

	void RenderAPI::init() {
		if (!m_initialized) {
			glfwMakeContextCurrent(reinterpret_cast<GLFWwindow*>(m_engine.getWindow()->getHandle()));

			// gl3wInit returns 0 on success
			if (!s_gl3wInitialized && gl3wInit()) {
				throw std::exception("Could not initialize GL3W");
			}



			s_gl3wInitialized = true;
			assert(gl3wIsSupported(4, 0));
		}
	}

	void RenderAPI::shutdown() {
		if (m_initialized)
			m_initialized = false;
	}

	RenderAPI::~RenderAPI() {
		shutdown();
	}
}

namespace DX11 {
	class RenderAPI : public IRenderAPI {
	public:
	};
}


IRenderAPI::IRenderAPI(RenderEngine& engine) : m_engine(engine) {
}

IRenderAPI* IRenderAPI::Create(RenderEngine& engine, RenderAPIType type) {
	assert(type != API_INVALID);

	switch (type) {
	case API_GL:
		return new GL::RenderAPI(engine);
	default:
		throw std::exception("RenderAPI: Invalid type called for creation");
	}
}