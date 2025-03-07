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
	
private:
	static HANDLE _hChildStdOutRead, _hChildStdOutWrite;
	static HANDLE _hChildStdInWrite, _hChildStdInRead;
	static STARTUPINFOA _si;
	static PROCESS_INFORMATION _pi;
	;

	static void init_pipes();
	static void init_process(const std::string& filePath);
	static void init_communication(const std::string& filePath); // Create the communication pipeline 

	static bool test_part_1();
	static bool test_part_2();

	static bool flush_buffer();
	static bool check_indentation(const std::string& message);
	static bool check_empty();
	static bool check_quit();
};

#endif // !TESTER_H

