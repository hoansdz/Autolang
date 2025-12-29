#include <iostream>
#include <Autolang.hpp>

int main(int argc, char *argv[])
{
	try{
		AVM i = AVM("tests/stress_1000_classes.txt", false);
	}
	catch (const std::exception& e) {
		std::cerr<<e.what();
	}
}