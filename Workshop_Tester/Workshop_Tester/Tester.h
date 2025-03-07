#ifndef TESTER_H
#define TESTER_H

#include <iostream>
#include <windows.h>
#include <string>
#include <thread>

class Tester {

public:
	static void run_tests(const std::string& filePath);
	static void cleanup();

};

#endif // !TESTER_H

