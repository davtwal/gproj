#include "RenderEngine.h"
#include "Trace.h"

int main() {
    Trace::Init();
    
	try {
	Trace::Report() << "Starting up program..." << Trace::EndRep();
	
	RenderAPIType api = 
#ifdef WIN32
		RenderAPIType::API_GL
#else
		RenderAPI::API_GL
#endif
	;
	RenderEngine* renderer = new RenderEngine;

	if(!renderer) {
		Trace::Report() << "Could not create render engine." << Trace::EndRep();
		return -1;
	}

    Trace::Report() << "Managed to start program." << Trace::EndRep();

    renderer->init(api);

    Trace::Report() << "Initialized." << Trace::EndRep();

    while(renderer->update()) {
    }

    Trace::Report() << "Finished update loop" << Trace::EndRep();

    renderer->shutdown();

    Trace::Report() << "Shutdown" << Trace::EndRep();

    delete renderer;
    }
    catch(const char* err) {
        Trace::Report() << "Error caught: " << err << Trace::EndRep();
    }

    Trace::Shutdown();
    
	return 0;
}
