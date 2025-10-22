#include <iostream>
#include <filesystem>
#include <Autolang.hpp>

int main(int argc, char *argv[])
{
	try{
		std::cout << std::filesystem::current_path() << std::endl;

		AVM i = AVM("tests/source.txt");
	}
	catch (const std::exception& e) {
		std::cerr<<e.what();
	}
}