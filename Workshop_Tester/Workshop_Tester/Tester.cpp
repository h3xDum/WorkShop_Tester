#include "Tester.h"

#define BUFFER_SIZE 128
#define INDENTATION_EXCEPTION_MESSAGE "IndentationError: unexpected indent\r\n>>> "
#define INDENTATION_EXCEPTION_SIZE 42
#define SYNTAX_EXCEPTION_MESSAGE "SyntaxError: invalid syntax\r\n>>> "
#define NAME_ERROR_END_INDEX 16
#define NAME_ERROR_EXCEPTION "NameError : name " 
#define SPACE " \n" // the new line is to ensure std::getline() on the interpreter will work correctly 
#define TAB   "\t\n"
#define RED 12
#define GREEN 2
#define WHITE 15

// API
Tester::Tester(const std::string& filePath) : _hChildStdOutRead(nullptr), _hChildStdOutWrite(nullptr),
				   _hChildStdInWrite(nullptr), _hChildStdInRead(nullptr),
				   _si({0}), _pi({0}), _filePath(filePath), _hConsole(GetStdHandle(STD_OUTPUT_HANDLE)) {
	
	// set text color 
	SetConsoleTextAttribute(_hConsole, WHITE);
	
	// assigning all the tests
	_generalTests.push_back(&Tester::test_part_1);
	_generalTests.push_back(&Tester::test_part_2);
	_generalTests.push_back(&Tester::test_part_3);

	_part1Tests.push_back(&Tester::flush_buffer);
	_part1Tests.push_back(&Tester::check_indentation);
	_part1Tests.push_back(&Tester::check_empty);
	_part1Tests.push_back(&Tester::check_quit);

	_part2Tests.push_back(&Tester::flush_buffer);
	_part2Tests.push_back(&Tester::check_bool);
	_part2Tests.push_back(&Tester::check_int);
	_part2Tests.push_back(&Tester::check_str);

	_part3Tests.push_back(&Tester::check_valid_var_assignment); 
	_part3Tests.push_back(&Tester::check_invalid_var_names); 
	_part3Tests.push_back(&Tester::check_redefinitions); 
	_part3Tests.push_back(&Tester::check_undefined_var); 

}

Tester::~Tester() {
	Tester::cleanup();
}

void Tester::run_tests() {
	// running part 1/2/3 tests 
	for (auto& test : _generalTests) {
		if (!(this->*test)()) {
			break;
		}
	}
}


// Communication pipelines
void Tester::init_pipes() {

	SECURITY_ATTRIBUTES sa;
	// Ensuring handle inheritance for spawn child process
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = true;

	// stdout & stdin pipes
	if (!CreatePipe(&_hChildStdInRead, &_hChildStdInWrite, &sa, 0)) {
		throw std::runtime_error("[!]ERROR::CreatePipe for child process STDIN faild");
	}
	if (!CreatePipe(&_hChildStdOutRead, &_hChildStdOutWrite, &sa, 0)) {
		throw std::runtime_error("[!]ERROR::CreatePipe for child process STDOUT faild");
	}

	// making sure theres no stalling 
	DWORD mode = PIPE_NOWAIT;
	SetNamedPipeHandleState(_hChildStdOutRead, &mode, nullptr, nullptr);


}

void Tester::init_process() {
	// Creates the mython interpreter process 
	ZeroMemory(&_si, sizeof(_si));
	ZeroMemory(&_pi, sizeof(_pi));

	_si.cb = sizeof(_si);
	_si.dwFlags = STARTF_USESTDHANDLES;
	_si.hStdInput = _hChildStdInRead;   // Redirect child STDIN
	_si.hStdOutput = _hChildStdOutWrite; // Redirect child STDOUT


	if (!CreateProcessA(_filePath.c_str(), nullptr, nullptr, nullptr, true, 0, nullptr, nullptr, &_si, &_pi)) {
		throw std::runtime_error("[!] Failed to Create child process, check input file path.");
	}

	// those pipe ends wont be used
	CloseHandle(_hChildStdInRead);
	CloseHandle(_hChildStdOutWrite);
	// prevent exception on double close at Tester::cleanup()
	_hChildStdInRead = nullptr;
	_hChildStdOutWrite = nullptr;

}

void Tester::init_communication() {
	// clean prev connection handles, Requierd duo to Part 1 tests closing the connection on "quit()"
	Tester::cleanup();
	// init connection again 
	Tester::init_pipes();
	Tester::init_process();
}


// General test calling functions
bool Tester::test_part_1() {
	std::cout << "Testing Part 1: " << std::endl;
	Tester::init_communication();

	// running all the part 1 tests 
	for (auto& test : _part1Tests) {
		if (!(this->*test)()) {
			SetConsoleTextAttribute(_hConsole, RED);
			std::cout << "[!] Test Failed" << std::endl;
			SetConsoleTextAttribute(_hConsole, WHITE);
			return false;
		}
	}
	
	SetConsoleTextAttribute(_hConsole, GREEN);
	std::cout << "\n[+] Part 1 Passed , congrats !\n" << std::endl;
	SetConsoleTextAttribute(_hConsole, WHITE);
	return true;
}

bool Tester::test_part_2() {
	std::cout << "Testing Part 2: " << std::endl;
	Tester::init_communication();

	// running all the part 2 tests
	for (auto& test : _part2Tests) {
		if (!(this->*test)()) {
			SetConsoleTextAttribute(_hConsole, RED);
			std::cout << "[!] Test Failed" << std::endl;
			SetConsoleTextAttribute(_hConsole, WHITE);
			return false;
		}
	}

	SetConsoleTextAttribute(_hConsole, GREEN);
	std::cout << "\n[+] Part 2 Passed , congrats !\n" << std::endl;
	SetConsoleTextAttribute(_hConsole, WHITE);
	return true;
}

bool Tester::test_part_3() {
	std::cout << "Testing Part 3: " << std::endl;
	// running all the part 3 tests 
	for (auto& test : _part3Tests) {
		if (!(this->*test)()) {
			SetConsoleTextAttribute(_hConsole, RED);
			std::cout << "[!] Test Failed" << std::endl;
			SetConsoleTextAttribute(_hConsole, WHITE);
			return false;
		}
	}

	SetConsoleTextAttribute(_hConsole, GREEN);
	std::cout << "\n[+] Part 3 Passed , congrats !\n" << std::endl;
	SetConsoleTextAttribute(_hConsole, WHITE);
	return true;
}


// Part 1 Tests 
bool Tester::check_quit() {
	std::cout << " - Testing quit() functionality" << std::endl;

	std::string message = "quit()\n"; // new line char for std::getline() ... 
	DWORD numOfBytesWritten;
	if (!WriteFile(_hChildStdInWrite, message.c_str(), DWORD(strlen(message.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}

	// Give it time to register the quit command
	WaitForSingleObject(_pi.hProcess, 1000);
	DWORD exitCode;
	if (!GetExitCodeProcess(_pi.hProcess, &exitCode)) {
		throw std::runtime_error("[!] Failed to verify exit code on the workshop interpreter process");
	}

	// check if quit() didnt kill the process
	if (exitCode == STILL_ACTIVE) {
		std::cout << "[!] quit() did not end the process" << std::endl;
		return false;
	}

	std::cout << "   [+] -> Test Passed" << std::endl;
	return true;

}

bool Tester::check_indentation() {
	if (!Tester::check_spaced_input(SPACE) || !Tester::check_spaced_input(TAB)) {
		return false;
	}
	return true;
}

bool Tester::check_empty() {
	std::cout << " - Testing for ENTER input" << std::endl;
	// send a new line char to validate the program doesnt crash
	const char* emptyString = "\n";
	DWORD numOfBytsWritten;
	if (!WriteFile(_hChildStdInWrite, emptyString, DWORD(strlen(emptyString)), &numOfBytsWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}

	// Read & Cmp 
	char buff[BUFFER_SIZE];
	DWORD numOfBytesRead;
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';
	if (strcmp(buff, ">>> ") != 0) {
		std::cout << "Expected:\n>>> " << std::endl <<
			"Got:\n" << buff << std::endl;
		return false;
	}

	std::cout << "   [+] -> Test Passed" << std::endl;
	return true;
}


// Part 2 Tests
bool Tester::check_bool() { // function that checks handling bool vals
	std::cout << " - Testing Bool type handling" << std::endl;
	if (!Tester::check_valid_bool() || !Tester::check_unvalid_bool()) {
		return false;
	}

	std::cout << "   [+] -> Test Passed" << std::endl;
	return true;
}

bool Tester::check_int() {
	std::cout << " - Testing Integer type handling" << std::endl;
	std::string payload;
	char buff[BUFFER_SIZE];
	DWORD numsOfBytesWritten;
	DWORD numsOfBytesRead;

	// should trim the leading zeros 
	payload = "-00054\n";
	if (!WriteFile(_hChildStdInWrite, payload.c_str(), DWORD(strlen(payload.c_str())), &numsOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // ensure theres time to process the input 
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numsOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numsOfBytesRead] = '\0';
	// cmp with expected output
	if (strcmp(buff, "-54\r\n>>> ") != 0) {
		std::cout << "Expected:\n-54\n>>> " <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}

	std::cout << "   [+] -> Test Passed" << std::endl;
	return true;
}

bool Tester::check_str() {

	std::cout << " - Testing String type handling" << std::endl;
	// Test syntax error inputs
	if (!Tester::check_str_syntax_error("\"acbd'\n") || !Tester::check_str_syntax_error("\"asf\"aaa\"\n")) {
		return false;
	}

	// Test for valid inputs
	std::map<std::string, std::string> validStrings = { {"\"Blabla\"\n" , "'Blabla'"},
		 { "'foo foo'\n" , "'foo foo'"},
		 {"\"Bla'Foo\"\n" , "\"Bla'Foo\""},
		 {"\"asf'ss'asf\"\n", "\"asf'ss'asf\""}
	};

	for (auto it = validStrings.begin(); it != validStrings.end(); it++) {
		if (!Tester::check_valid_str(it->first, it->second))
			return false;
	}

	std::cout << "   [+] -> Test Paseed" << std::endl;
	return true;
}


// Part 3 Tests
bool Tester::check_invalid_var_names() {
	std::cout << " - Testing for invalid variable names" << std::endl;

	std::vector<std::string> messages = { "3a = 3\n", "a#$ = 5\n"};
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;
	char buff[BUFFER_SIZE];

	// make sure they return an invalid syntax error 
	for (const auto& str : messages) {
		if (!WriteFile(_hChildStdInWrite, str.c_str(), DWORD(strlen(str.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(150));
		if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
			std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
			return false;
		}
		buff[numOfBytesRead] = '\0';

		if (strcmp(buff, SYNTAX_EXCEPTION_MESSAGE) != 0) {
			std::cout << "For input: " << str;
			std::cout << "Expected:\n" << SYNTAX_EXCEPTION_MESSAGE <<
				"\nGot:\n" << buff << std::endl;
			return false;
		}
	}

	std::cout << "   [+] Test Passed " << std::endl;
	return true;
}

bool Tester::check_no_error() {

	std::vector<std::string> messages = { "a1 = 100\n", "a2 = True\n", "a123 = 'bbb'\n", "a_b = 200\n", "a_3b = 25\n", "_a = 5\n"};
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;
	char buff[BUFFER_SIZE];

	// make sure they return an invalid syntax error 
	for (const auto& str : messages) {
		if (!WriteFile(_hChildStdInWrite, str.c_str(), DWORD(strlen(str.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(150));
		if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
			std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
			return false;
		}
		buff[numOfBytesRead] = '\0';

		if (strcmp(buff, ">>> ") != 0) {
			std::cout << "For input: " << str;
			std::cout << "Expected:\n" << "\r\n>>> " <<
				"\nGot:\n" << buff << std::endl;
			return false;
		}
	}

	return true;
	
}

bool Tester::check_redefinitions() {
	
	std::cout << " - Testing variables redefinitions " << std::endl;
	if (!Tester::check_basics_redefs() || !Tester::check_var_to_var_defs()) {
		std::cout << "[!] Test failed" << std::endl;
		return false;
	}

	std::cout << "   [+] Test Passed " << std::endl;
	return true;
}

bool Tester::check_basics_redefs() {
	std::multimap<const std::string, const std::string> vars = { {"a", "100"},
		{"a", "True"},
		{"b", "25"},
		{"b", "False"} };

	std::string msg;
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;
	char buff[BUFFER_SIZE];
	for (auto it = vars.begin(); it != vars.end(); it++) {
		// create the variables for the interprester
		msg = it->first + " = " + it->second + "\n";
		if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		Tester::flush_buffer();

		// check correct assignment
		msg = it->second + "\r\n>>> ";
		if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
			std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
			return false;
		}
		buff[numOfBytesRead] = '\0';
		// build the full expected output
		std::string expectedOutput(it->second + "\r\n>>> ");
		if (expectedOutput != buff) {
			std::cout << "For input: " << it->first << std::endl;
			std::cout << "Expected:\n" << expectedOutput <<
				"\nGot:\n" << buff << std::endl;
			return false;
		}
	}
	std::cout << "    * basic redefs passed " << std::endl;
	return true;
}

bool Tester::check_var_to_var_defs() {
	std::string msg;
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;
	char buff[BUFFER_SIZE];

	// assigning vars values
	msg = "a = True\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	Tester::flush_buffer();
	msg = "b = 25\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	Tester::flush_buffer();

	// assign var to var value
	msg = "a = b\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	Tester::flush_buffer();
	// check a & b 
	msg = "a\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';
	if (strcmp(buff, "25\r\n>>> ") != 0) {
		std::cout << "For input: " << msg << std::endl;
		std::cout << "Expected:\n" << "25" <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}

	msg = "b\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';
	if (strcmp(buff, "25\r\n>>> ") != 0) {
		std::cout << "For input: " << msg << std::endl;
		std::cout << "Expected:\n" << "25" <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}
	
	std::cout << "    * var to var defs passed" << std::endl;
	return true;
}

bool Tester::check_valid_var_assignment() {
	std::cout << " - Testing valid variable assignments " << std::endl;
	if (!Tester::check_no_error()) {
		return false;
	}

	std::map<std::string, std::string> vars = { {"a1", "5"},
		{"a2", "        False"},
		{"a123", "'check this'"}};
	
	// send the message
	DWORD numOfBytesWritten;
	for (auto it = vars.begin(); it != vars.end(); it++) {
		// create the variables for the interprester
		std::string msg(it->first + " = " + it->second + "\n");;
		if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		Tester::flush_buffer();
	}
	// check the output
	DWORD numOfBytesRead;
	char buff[BUFFER_SIZE];
	for (auto it = vars.begin(); it != vars.end(); it++) {
		// check the var values from the interpreter
		std::string msg(it->first + "\n");
		if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
			std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
			std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
			return false;
		}
		buff[numOfBytesRead] = '\0';
		// build the full expected output
		it->second = it->second.substr(it->second.find_first_not_of(" ")); // trim the leadin space on a2 value
		std::string expectedOutput(it->second + "\r\n>>> ");
		if (expectedOutput != buff) {
			std::cout << "For input: " << it->first << std::endl;
			std::cout << "Expected:\n" << expectedOutput <<
				"\nGot:\n" << buff << std::endl;
			return false;
		}
	}


	std::cout << "   [+] Test Passed " << std::endl;
	return true;
} 

bool Tester::check_undefined_var() {
	std::cout << " - Testing undefined variables " << std::endl;
	std::string msg;
	std::string expectedError;
	char buff[BUFFER_SIZE];
	DWORD numOfBytesRead;
	DWORD numOfBytesWritten;

	// send undefined variable
	msg = "z\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';
	
	// validate a name error response
	expectedError = NAME_ERROR_EXCEPTION;
	expectedError.append("'z' is not defined\r\n>>> ");
	if (strcmp(expectedError.c_str(), buff) != 0) {
		std::cout << "Expected:\n" << expectedError << "Got:\n" << buff << std::endl;
		return false;
	}

	// assign to a non existent var
	msg = "z=3\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	Tester::flush_buffer();
	msg = "z = x\n";
	if (!WriteFile(_hChildStdInWrite, msg.c_str(), DWORD(strlen(msg.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';
	// validate error message
	expectedError = NAME_ERROR_EXCEPTION;
	expectedError.append("'x' is not defined\r\n>>> ");
	if (strcmp(expectedError.c_str(), buff) != 0) {
		std::cout << "Expected:\n" << expectedError << "Got:\n" << buff << std::endl;
		return false;
	}


	std::cout << "   [+] Test Passed " << std::endl;
	return true;

}


// General helper functions/subtests
bool Tester::flush_buffer() {
	char buffer[BUFFER_SIZE];
	DWORD numOfBytesRead;

	// flush meaningless cout buffer 
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // ensure theres time for the interpreter to output
	if (!ReadFile(_hChildStdOutRead, buffer, sizeof(buffer) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout " << std::endl;
		return false;
	}
	return true;

}

bool Tester::check_spaced_input(const std::string& message) {
	std::cout << " - Testing for indentation error on " << ((message == SPACE) ? "SPACE\n" : "TAB\n");

	// Send payload to stdin
	DWORD numOfBytesWritten;
	if (!WriteFile(_hChildStdInWrite, message.c_str(), DWORD(strlen(message.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}

	// Read response
	char buffer[INDENTATION_EXCEPTION_SIZE] = { 0 };
	DWORD numOfBytesRead;
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buffer, sizeof(buffer) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buffer[numOfBytesRead] = '\0';


	// Cmp 
	if (strcmp(buffer, INDENTATION_EXCEPTION_MESSAGE) != 0) {
		std::cout << "Expected:\nIndentationError: unexpected indent\n>>> " <<
			"\nGot:\n" << buffer << std::endl;
		return false;
	}

	std::cout << "   [+] -> Test Passed" << std::endl;
	return true;

}

bool Tester::check_valid_bool() {
	// Test for True/False with a valid syntax(capital letter)
	std::string payload;
	char buff[BUFFER_SIZE];
	DWORD numsOfBytesWritten;
	DWORD numsOfBytesRead;


	payload = "True\n";
	if (!WriteFile(_hChildStdInWrite, payload.c_str(), DWORD(strlen(payload.c_str())), &numsOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // ensure theres time to process the input 
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numsOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numsOfBytesRead] = '\0';
	// cmp with expected output
	if (strcmp(buff, "True\r\n>>> ") != 0) {
		std::cout << "Expected:\nTrue\n>>> " <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}
	payload = "False\n";
	if (!WriteFile(_hChildStdInWrite, payload.c_str(), DWORD(strlen(payload.c_str())), &numsOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // ensure theres time to process the input 
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numsOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	// cmp with expected output
	buff[numsOfBytesRead] = '\0';
	if (strcmp(buff, "False\r\n>>> ") != 0) {
		std::cout << "Expected:\nFalse\n>>> " <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}
	return true;
}

bool Tester::check_unvalid_bool() {
	// Test for invalid true/false, no capital letter
	std::string payload;
	char buff[BUFFER_SIZE];
	DWORD numsOfBytesWritten;
	DWORD numsOfBytesRead;

	// Check for "true"
	payload = "true\n";
	if (!WriteFile(_hChildStdInWrite, payload.c_str(), DWORD(strlen(payload.c_str())), &numsOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Ensure time to process input
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numsOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numsOfBytesRead] = '\0';

	// Compare with expected output
	std::string errorMessage(buff);
	if (errorMessage != SYNTAX_EXCEPTION_MESSAGE) {
		// might have part 3 completed hence a name error exception
		if (errorMessage.find(NAME_ERROR_EXCEPTION) == 0) {  // Check if starts with "NameError : name "
			// build expcected error 
			std::string expectedError = NAME_ERROR_EXCEPTION;
			expectedError.append("'true' is not defined\r\n>>> ");
			if (errorMessage != expectedError) {
				std::cout << "Expected:\n" << expectedError << "\nGot:\n" << errorMessage << std::endl;
				return false;
			}
		}
		else {
			std::cout << "Expected:\n" << SYNTAX_EXCEPTION_MESSAGE << "\nGot:\n" << buff << std::endl;
			return false;
		}
	}

	// Check for "false"
	payload = "false\n";
	if (!WriteFile(_hChildStdInWrite, payload.c_str(), DWORD(strlen(payload.c_str())), &numsOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Ensure time to process input
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numsOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numsOfBytesRead] = '\0';

	// Compare with expected output
	errorMessage = buff;
	if (errorMessage != SYNTAX_EXCEPTION_MESSAGE) {
		// might have part 3 completed hence a name error exception
		if (errorMessage.find(NAME_ERROR_EXCEPTION) == 0) {  // Check if starts with "NameError : name "
			// build expected error
			std::string expectedError = NAME_ERROR_EXCEPTION;
			expectedError.append("'false' is not defined\r\n>>> ");
			if (expectedError != errorMessage) {
				std::cout << "Expected:\n" << expectedError << "\nGot:\n" << errorMessage << std::endl;
				return false;
			}
		}
		else {
			std::cout << "Expected:\n" << SYNTAX_EXCEPTION_MESSAGE << "\nGot:\n" << buff << std::endl;
			return false;
		}
	}

	return true;
}

bool Tester::check_str_syntax_error(const std::string& str) {
	char buff[BUFFER_SIZE];
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;

	// Send the invalid str
	if (!WriteFile(_hChildStdInWrite, str.c_str(), DWORD(strlen(str.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';

	// cmp it with expected output
	if (strcmp(buff, SYNTAX_EXCEPTION_MESSAGE) != 0) {
		std::cout << "Expected:\n" << SYNTAX_EXCEPTION_MESSAGE  <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}

	return true;
}

bool Tester::check_valid_str(const std::string& message, std::string& expectedOutput) {
	char buff[BUFFER_SIZE];
	DWORD numOfBytesWritten;
	DWORD numOfBytesRead;

	// Send the invalid str
	if (!WriteFile(_hChildStdInWrite, message.c_str(), DWORD(strlen(message.c_str())), &numOfBytesWritten, nullptr)) {
		std::cerr << "[!] Failed to Write input to the workshop interpreter stdin" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	if (!ReadFile(_hChildStdOutRead, buff, sizeof(buff) - 1, &numOfBytesRead, nullptr)) {
		std::cerr << "[!] Failed to read from the workshop interpreter stdout" << std::endl;
		return false;
	}
	buff[numOfBytesRead] = '\0';

	// build the full expectedOutput
	expectedOutput.append("\r\n>>> ");
	if (strcmp(buff, expectedOutput.c_str()) != 0) {
		std::cout << "For input: " << message;
		std::cout << "Expected:\n" << expectedOutput <<
			"\nGot:\n" << buff << std::endl;
		return false;
	}
	return true;
}


void Tester::cleanup() {

	// Close all the handels that might have been opened
	WaitForSingleObject(_pi.hProcess, 1000);
	DWORD exitCode;
	if (GetExitCodeProcess(_pi.hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
		TerminateProcess(_pi.hProcess, 1);
	}

	if (_hChildStdInWrite) {
		CloseHandle(_hChildStdInWrite);
		_hChildStdInWrite = nullptr;
	}
	if (_hChildStdInRead) {
		CloseHandle(_hChildStdInRead);
		_hChildStdInRead = nullptr;
	}
	if (_hChildStdOutRead) {
		CloseHandle(_hChildStdOutRead);
		_hChildStdOutRead = nullptr;
	}
	if (_hChildStdOutWrite) {
		CloseHandle(_hChildStdOutWrite);
		_hChildStdOutWrite = nullptr;
	}
	if (_pi.hThread) {
		CloseHandle(_pi.hThread);
	}

	if (_pi.hProcess) {
		CloseHandle(_pi.hProcess);
	}
	// clean to reuse structures for part 2 tests
	ZeroMemory(&_pi, sizeof(_pi));
	ZeroMemory(&_si, sizeof(_si));

}