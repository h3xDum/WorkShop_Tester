#include "Tester.h"

int main(int argc, char* argv[]) {

	system("cls");

	if (argc < 2) {
		std::cout << "[!] Did not enter the file path as an argument" << std::endl;
		return 0;
	}
	try {
		Tester::run_tests(argv[1]);
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}

	Tester::cleanup();
	return 0;
}