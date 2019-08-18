#include "RenderEngine.h"
#include "Trace.h"

int main() {
    Trace::Init();
    
	try {
	Trace::Report() << "Starting up program..." << Trace::EndRep();
	
	IRenderEngine* renderer = IRenderEngine::Create(RenderAPI::API_GL);

	if(!renderer) {
		Trace::Report() << "Could not create render engine." << Trace::EndRep();
		return -1;
	}

    Trace::Report() << "Managed to start program." << Trace::EndRep();

    renderer->init();

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
