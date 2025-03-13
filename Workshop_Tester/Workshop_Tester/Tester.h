#ifndef TESTER_H
#define TESTER_H

#include <iostream>
#include <windows.h>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

class Tester {

public:
	Tester();
	~Tester();
	void run_tests(const std::string& filePath);
	
private:
	HANDLE _hChildStdOutRead, _hChildStdOutWrite;
	HANDLE _hChildStdInWrite, _hChildStdInRead;
	STARTUPINFOA _si;
	PROCESS_INFORMATION _pi;
	

	void init_pipes();
	void init_process(const std::string& filePath);
	void init_communication(const std::string& filePath); // Create the communication pipeline 
	void cleanup();

	bool test_part_1();
	bool test_part_2();
	bool test_part_3();

	bool flush_buffer();
	bool check_indentation(const std::string& message);
	bool check_empty();
	bool check_quit();
	bool check_bool();
	bool check_int();
	bool check_str();
	bool check_correct_var_assignment();

	bool check_valid_bool();
	bool check_unvalid_bool();
	bool check_str_syntax_error(const std::string& str);
	bool check_valid_str(const std::string& message , std::string& expectedOutput);
	bool check_invalid_var_names();
	bool check_invalid_var();
	bool check_no_error();
	bool check_redefinitions();


};

#endif // !TESTER_H

