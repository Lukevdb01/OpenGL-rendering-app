#include "Core.h"

class ENGINE_API Application
{
public:
	Application();
	virtual ~Application();

	void Run();
};

Application* CreateApplication();