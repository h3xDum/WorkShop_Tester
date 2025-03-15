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
	// initialization & cleanup
	Tester(const std::string& filePath); 
	~Tester();
	void run_tests();  
	
private:
	// interpreter child process input/output streams
	HANDLE _hChildStdOutRead, _hChildStdOutWrite; 
	HANDLE _hChildStdInWrite, _hChildStdInRead;
	// intrepreter process structures 
	STARTUPINFOA _si;
	PROCESS_INFORMATION _pi;
	std::string _filePath; // path to the interpreter exe 
	
	// test vectors for organization and easy expansion 
	using testFunction = bool (Tester::*)();
	std::vector<testFunction> _generalTests;
	std::vector<testFunction> _part1Tests;
	std::vector<testFunction> _part2Tests;
	std::vector<testFunction> _part3Tests;

	// general tests 
	bool test_part_1();
	bool test_part_2();
	bool test_part_3();

	// communication pipelines 
	void init_pipes();
	void init_process();
	void init_communication();  
	void cleanup();
	
	// tests by parts 
	bool check_indentation();
	bool check_empty();
	bool check_quit();
	
	bool check_bool();
	bool check_int();
	bool check_str();

	bool check_valid_var_assignment();
	bool check_invalid_var_names();
	bool check_redefinitions();
	bool check_undefined_var();
	
	// general helper functions/subtests
	bool flush_buffer();
	bool check_spaced_input(const std::string& message);
	bool check_valid_bool();
	bool check_unvalid_bool();
	bool check_str_syntax_error(const std::string& str);
	bool check_valid_str(const std::string& message , std::string& expectedOutput);
	bool check_no_error();
	bool check_basics_redefs();
	bool check_var_to_var_defs();
	

};

#endif // !TESTER_H

