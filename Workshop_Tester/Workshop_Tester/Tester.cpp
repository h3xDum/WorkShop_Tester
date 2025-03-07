#include "Tester.h"

#define BUFFER_SIZE 128
#define INDENTATION_EXCEPTION_MESSAGE "IndentationError: unexpected indent\r\n>>> "
#define INDENTATION_EXCEPTION_SIZE 42
#define SYNTAX_EXCEPTION_MESSAGE "SyntaxError: invalid syntax\r\n>>> "
#define SYNTAX_EXCEPTION_SIZE 34
#define NAME_ERROR_EXCEPTION "NameError : name '" 
#define SPACE " \n" // the new line is to ensure std::getline() on the interpreter will work correctly 
#define TAB   "\t\n"

// static variabels 
HANDLE Tester::_hChildStdOutRead, Tester::_hChildStdOutWrite;
HANDLE Tester::_hChildStdInWrite, Tester::_hChildStdInRead;
STARTUPINFOA Tester::_si;
PROCESS_INFORMATION Tester::_pi;


void Tester::run_tests(const std::string& filePath) {

	// Test Part  1
	Tester::init_communication(filePath);
	if (!Tester::test_part_1()) {
		std::cout << "[!] Part 1 failed :( " << std::endl;
		return;
	}

	// Test Part 2
	Tester::init_communication(filePath);
	if (!Tester::test_part_2()) {
		std::cout << "[!] Part 2 failed :( " << std::endl;
		return;
	}


}

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

void Tester::init_process(const std::string& filePath) {
	// Creates the mython interpreter process 
	ZeroMemory(&_si, sizeof(_si));
	ZeroMemory(&_pi, sizeof(_pi));

	_si.cb = sizeof(_si);
	_si.dwFlags = STARTF_USESTDHANDLES;
	_si.hStdInput = _hChildStdInRead;   // Redirect child STDIN
	_si.hStdOutput = _hChildStdOutWrite; // Redirect child STDOUT


	if (!CreateProcessA(filePath.c_str(), nullptr, nullptr, nullptr, true, 0, nullptr, nullptr, &_si, &_pi)) {
		throw std::runtime_error("[!] Failed to Create child process, check input file path.");
	}

	// those pipe ends wont be used
	CloseHandle(_hChildStdInRead);
	CloseHandle(_hChildStdOutWrite);
	// prevent exception on double close at Tester::clean()
	_hChildStdInRead = nullptr;
	_hChildStdOutWrite = nullptr;

}

void Tester::init_communication(const std::string& filePath) {
	// clean prev connection handles, Requierd duo to Part 1 tests closing the connection on "quit()"
	Tester::cleanup();
	// init connection again 
	Tester::init_pipes();
	Tester::init_process(filePath);
}

bool Tester::test_part_1() {

	std::cout << "Testing Part 1: " << std::endl;
	// dont care about the "Welcome to Magshimim Python Interperter" message on start 
	if (!Tester::flush_buffer()) {
		std::cout << "Didnt print nothing at the start of executaion?" << std::endl;
		return false;
	}

	// Basic functionality tests
	if (!Tester::check_indentation(SPACE) || !Tester::check_indentation(TAB) || !Tester::check_empty()) {
		std::cout << "[!] Test Failed" << std::endl;
		return false;
	}

	if (!Tester::check_quit()) {
		std::cout << "[!] Test Failed" << std::endl;
		return false;
	}

	std::cout << "\n[+] Part 1 Passed , congrats !\n" << std::endl;
	return true;
}


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

bool Tester::check_quit() {
	std::cout << "Testing quit() functionality" << std::endl;

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

	std::cout << "[+] -> Test Passed " << std::endl;
	return true;

}

bool Tester::check_indentation(const std::string& message) {
	std::cout << "Testing for indentation error on " << ((message == SPACE) ? "SPACE\n" : "TAB\n");

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

	std::cout << "[+] -> Test Passed " << std::endl;
	return true;

}

bool Tester::check_empty() {
	std::cout << "Testing for ENTER input" << std::endl;
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

	std::cout << "[+] -> Test Passed " << std::endl;
	return true;
}

bool Tester::test_part_2() {

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