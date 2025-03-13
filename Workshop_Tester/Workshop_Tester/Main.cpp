#include "Tester.h"

int main(int argc, char* argv[]) {

	system("cls");

	if (argc < 2) {
		std::cout << "[!] Did not enter the file path as an argument" << std::endl;
		return 0;
	}
	try {
		Tester t(argv[1]);
		t.run_tests();
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}

	return 0;
}