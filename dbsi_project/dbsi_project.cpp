#include "dbsi_project.h"


using namespace dbsi;
int main()
{
	Literal l{ "test" };
	Resource r = l;
	std::cout << "Hello CMake." << std::get<Literal>(r).val << std::endl;
	return 0;
}
