#ifndef DW_WINDOW_H
#define DW_WINDOW_H

class IWindow {
public:
	virtual void init();
	virtual void shutdown();

	virtual void update();
	virtual bool shouldClose();

	virtual void show();

	// 0 = opengl, 1 = vulkan, 2 = dx11, 3 = dx12
	static IWindow* create(unsigned type);

	virtual void* getInternal() const;
	unsigned getType() const;

protected:
	unsigned m_type;
};

#endif

