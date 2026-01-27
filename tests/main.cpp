#include <iostream>
#include <Autolang.hpp>

int main(int argc, char *argv[])
{
	try{
		AVMReadFileMode mode = {
			"tests/test.txt",
			nullptr,
			0,
			true
		};
		AVM i = AVM(mode, false);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
	}
}