#include <Engine.h>

class Executable : public Application
{

};

Application* CreateApplication()
{
	return new Executable();
}