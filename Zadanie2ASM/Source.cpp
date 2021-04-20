#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

#include "prompt.h"

#pragma warning(disable:4996)
using namespace std;


string prompt = "";
bool prompt_changed = false;

/* Part 1: custom shell functions =======================================================================================================
source: https://brennan.io/2015/01/16/write-a-shell-in-c/  */
/*
  Function Declarations for builtin shell commands:
 */
int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);
int shell_halt(char** args);
int shell_prompt(char** args);

/*
  List of builtin commands, followed by their corresponding functions.
 */

vector<string> builtin_str = {
  "cd",
  "help",
  "quit",
  "halt",
  "prompt"
};

vector <int (*) (char**)> pointers = {
  &shell_cd,
  &shell_help,
  &shell_exit,
  &shell_halt,
  &shell_prompt,
};

/*
  Builtin function implementations 
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int shell_cd(char** args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "shell: expected argument to \"cd\"\n");
	}
	else {
		if (chdir(args[1]) != 0) {
			perror("shell");
		}
	}
	return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int shell_help(char** args)
{
	cout << "Enter commands as in normal shell\n";
	cout << "Custom functions are :\n";
	for (size_t i = 0; i < builtin_str.size(); i++) {
		cout << builtin_str[i] << "\n";
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int shell_exit(char** args)
{
	exit(0);
	return 0;
}

int shell_halt(char** args)
{
	system("shutdown -P now");
	return 1;
}

int shell_prompt(char** args)
{
	string input;
	cout << "Enter letters T,C,U in order which u want the prompt to look like\n";
	cout << "T stands for time\n";
	cout << "C stands for computer name\n";
	cout << "U stands for username\n";
	cout << "You dont have to use every letter.\n";
	cout << "Default mode is TCU\n";
	cin >> input;
	prompt = make_prompt(input);
	prompt_changed = true;
	return 1;
}

/* Part 2 - shell arguments given at start =====================================================================================================
*/

class ShellArguments final {
public:
	string mode = "";
	string port = "";
	string socket_path = "";
};

ShellArguments shell_arguments;


bool init_arguments_object(int length, char* arguments[]) {
	for (int i = 1; i < length; i++) {
		if (string{ arguments[i] } == "-h") {
			if (shell_arguments.mode != "")
				return false;
			shell_arguments.mode = "HELP";
		}
		else if (string{ arguments[i] } == "-c") {
			if (shell_arguments.mode != "")
				return false;
			shell_arguments.mode = "CLIENT";
		}
		else if (string{ arguments[i] } == "-s") {
			if (shell_arguments.mode != "")
				return false;
			shell_arguments.mode = "SERVER";
		}
		else if (string{ arguments[i] } == "-a") {
			if (shell_arguments.mode != "")
				return false;
			shell_arguments.mode = "STANDALONE";
		}
		else if (string{ arguments[i] } == "-p") {
			i++;
			if (i < length && shell_arguments.port == "")
				shell_arguments.port = string{ arguments[i] };
			else
				return false;
		}
		else if (string{ arguments[i] } == "-u") {
			i++;
			if (i < length && shell_arguments.socket_path == "")
				shell_arguments.socket_path = string{ arguments[i] };
			else
				return false;
		}
		else
			return false;
	}
	return true;
}

void print_arguments(char** arguments) {
	int i = 0;
	while (true) {
		if (arguments[i] == nullptr) {
			cout << i << ": " << "NULL" << "\n";
			break;
		}
		cout << i << ": " << arguments[i] << "\n";
		i++;
	}

}

/*Part 3 - Custom prompt - moved to file prompt.h ==============================================================================
*/

/* Part 4 - CommandObject class, represents 1 line of command entered by user ================================================
*/
class CommandObject final {
public:
	CommandObject(string command_string) {
		string word = "";
		bool word_is_input_file = false, word_is_output_file = false, word_is_first = true, after_pipe = false;
		for (size_t i = 0; i < command_string.size(); i++) {
			if (command_string[i] == '|') {
				m_has_pipe = true;
				after_pipe = true;
				word_is_first = true;
				continue;
			}
			else if (command_string[i] == ' ') {
				if (!word.empty()) {
					if (word_is_first) {
						if (after_pipe)
							m_second_command = word;
						else
							m_command = word;
						word_is_first = false;
					}
					else if (word_is_input_file) {
						m_input_file_name = word;
						m_has_input_file = true;
						word_is_input_file = false;
					}
					else if (word_is_output_file) {
						m_output_file_name = word;
						m_has_output_file = true;
						word_is_output_file = false;
					}
					else {
						if (after_pipe)
							m_second_arguments.push_back(word);
						else
							m_arguments.push_back(word);
					}
					word = "";
				}
			}
			else if (command_string[i] == '>')
				word_is_output_file = true;
			else if (command_string[i] == '<')
				word_is_input_file = true;
			else
				word += command_string[i];
		}
	}

	string m_command = "";
	vector<string> m_arguments;
	string m_input_file_name = "";
	string m_output_file_name = "";
	bool m_has_output_file = false;
	bool m_has_input_file = false;
	bool m_has_pipe = false;
	string m_second_command = "";
	vector<string> m_second_arguments;

	friend std::ostream& operator<<(std::ostream& lhs, const CommandObject& rhs);

	char* get_command_as_c() {
		char* c_string = new char[m_command.length() + 1];
		strcpy(c_string, m_command.c_str());
		return c_string;
	}

	char** get_arguments_as_c() {
		size_t array_size = m_arguments.size() + 2;
		char** arguments = new char* [array_size];

		size_t i = 0;
		arguments[i] = new char[m_command.length() + 1];
		strcpy(arguments[i], m_command.c_str());
		for (i = 0; i < m_arguments.size(); i++) {
			arguments[i + 1] = new char[m_arguments[i].length() + 1];
			strcpy(arguments[i + 1], m_arguments[i].c_str());
		}
		i++;
		arguments[i] = nullptr;
		return arguments;
	}

	char* get_second_command_as_c() {
		char* c_string = new char[m_second_command.length() + 1];
		strcpy(c_string, m_second_command.c_str());
		return c_string;
	}

	char** get_second_arguments_as_c() {
		size_t array_size = m_second_arguments.size() + 2;
		char** arguments = new char* [array_size];

		size_t i = 0;
		arguments[i] = new char[m_second_command.length() + 1];
		strcpy(arguments[i], m_second_command.c_str());
		for (i = 0; i < m_second_arguments.size(); i++) {
			arguments[i + 1] = new char[m_second_arguments[i].length() + 1];
			strcpy(arguments[i + 1], m_second_arguments[i].c_str());
		}
		i++;
		arguments[i] = nullptr;
		return arguments;
	}
};


std::ostream& operator<<(std::ostream& lhs, const CommandObject& rhs)
{
	lhs << "COMMAND: " << '\"' << rhs.m_command << '\"' << "\n";
	lhs << "ARGUMENTS: ";
	for (size_t i = 0; i < rhs.m_arguments.size(); i++)
		lhs << '\"' << rhs.m_arguments[i] << '\"' << ", ";
	lhs << "\n";
	if (rhs.m_has_pipe) {
		lhs << "PIPE TO: \n";
		lhs << "COMMAND: " << '\"' << rhs.m_second_command << '\"' << "\n";
		lhs << "ARGUMENTS: ";
		for (size_t i = 0; i < rhs.m_second_arguments.size(); i++)
			lhs << '\"' << rhs.m_second_arguments[i] << '\"' << ", ";
		lhs << "\n";
	}
	lhs << "INPUT FILE: " << rhs.m_input_file_name << "\n";
	lhs << "OUTPUT FILE: " << rhs.m_output_file_name << "\n";
	return lhs;
}


vector<CommandObject> separate_commands(string input_line) {
	vector<char> delimiters = { ' ','\t','\r','\n','\a' };
	vector<char> special_characters = { '#',';','<','>','|' };
	char escape_character = '\\';
	string one_command = "";
	vector<string> all_commands;
	bool last_was_delimiter = false;

	for (size_t i = 0; i < input_line.size(); i++) {
		bool is_delimiter = false;
		for (size_t j = 0; j < delimiters.size(); j++) {
			if (input_line[i] == delimiters[j]) {
				if (!last_was_delimiter) {
					one_command += " ";
					last_was_delimiter = true;

				}
				is_delimiter = true;
			}
		}
		if (is_delimiter)
			continue;

		if (input_line[i] == '>') {
			if (last_was_delimiter)
				one_command += "> ";
			else
				one_command += " > ";
			last_was_delimiter = true;
			continue;
		}
		else if (input_line[i] == '>') {
			if (last_was_delimiter)
				one_command += "< ";
			else
				one_command += " < ";
			last_was_delimiter = true;
			continue;
		}
		else if (input_line[i] == '|') {
			if (last_was_delimiter)
				one_command += "| ";
			else
				one_command += " | ";
			last_was_delimiter = true;
			continue;
		}

		last_was_delimiter = false;

		if (input_line[i] == ';') {
			one_command += " ";
			all_commands.push_back(one_command);
			one_command = "";
		}
		else if (input_line[i] == '#') {
			break;
		}
		else if (input_line[i] == '\\') {
			if (i + 1 < input_line.size()) {
				bool next_is_special = false;
				for (size_t j = 0; j < special_characters.size(); j++)
					if (input_line[i + 1] == special_characters[j])
						next_is_special = true;

				if (next_is_special) {
					i++;
					one_command += input_line[i];
					continue;
				}
				else {
					one_command += '\\';
					continue;
				}
			}
		}
		else {
			one_command += input_line[i];
		}
	}
	if (one_command != "" || one_command != " ") {
		one_command += " ";
		all_commands.push_back(one_command);
	}

	vector<CommandObject> command_objects;
	for (size_t i = 0; i < all_commands.size(); i++)
		command_objects.push_back({ all_commands[i] });
	return command_objects;
}

/* Part 5 - command execution ========================================================================
*/

int execute_commands(CommandObject command) {
	for (size_t i = 0; i < builtin_str.size(); i++) {
		if (command.m_command == builtin_str[i]) {
			return pointers[i](command.get_arguments_as_c());
		}
	}
	int pid, wpid, status, file_desc, file_desc2;
	int pipefd[2];
	pipe(pipefd);

	pid = fork();
	if (pid == 0) {
		// child process
		if (command.m_has_pipe) {
			// replace standard input with input part of pipe
			if (dup2(pipefd[0], 0) < 0)
				perror("dub2child");
			// close unused hald of pipe
			if (close(pipefd[1]) < 0)
				perror("closechild");

			if (command.m_has_output_file) {
				file_desc = open(command.m_output_file_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}
			/*else {
				file_desc = open("./TEMP.TXT", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}*/

			if (command.m_has_input_file) {
				file_desc2 = open(command.m_input_file_name.c_str(), O_RDONLY);
				dup2(file_desc2, 0);
			}

			if (execvp(command.get_second_command_as_c(), command.get_second_arguments_as_c()) == -1)
				perror("shell");


			/*if (command.m_has_input_file)
				close(file_desc2);
			close(file_desc);*/
		}
		else {
			if (command.m_has_output_file) {
				file_desc = open(command.m_output_file_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}
			/*else {
				file_desc = open("./TEMP.TXT", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}*/

			if (command.m_has_input_file) {
				file_desc2 = open(command.m_input_file_name.c_str(), O_RDONLY);
				dup2(file_desc2, 0);
			}

			if (execvp(command.get_command_as_c(), command.get_arguments_as_c()) == -1)
				perror("shell");

			/*if (command.m_has_input_file)
				close(file_desc2);
			close(file_desc);*/

		}
		exit(-1);
		//return 1;
	}
	else if (pid > 0) {
		// Parent process

		if (command.m_has_pipe) {
			// replace standard output with output part of pipe
			if (dup2(pipefd[1], 1) < 0)
				perror("dup2parent");
			// close unused unput half of pipe
			if (close(pipefd[0]) < 0)
				perror("closeparent");

			if (execvp(command.get_command_as_c(), command.get_arguments_as_c()) == -1)
				perror("shell");
		}
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

/* Part 6 - server mode ==================================================================================
*/

//server from classmate

void launch_as_server() {
	string socket_path = "./sck";
	if (!shell_arguments.socket_path.empty())
		socket_path = shell_arguments.socket_path;

	int i, s, ns, r;
	char buff[1000] = "server ready\n";
	struct sockaddr_un ad;

	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, socket_path.c_str());
	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket: ");
		exit(2);
	}

	unlink("./sck");
	bind(s, (struct sockaddr*)&ad, sizeof(ad));
	listen(s, 5);

	// obsluzime len jedneho klienta
	ns = accept(s, NULL, NULL);

	prompt = make_prompt("TCU");
	write(ns, prompt.c_str(), static_cast<int>(prompt.size()));

	cout << "Mode : SERVER\n";
	while ((r = read(ns, buff, 64)) > 0)	// blokuju citanie, cakanie na poziadavku od klienta
	{
		buff[r] = 0;			// za poslednym prijatym znakom

		vector<CommandObject> commands;
		commands = separate_commands(string{ buff });
		int is_exit = 1;
		for (size_t i = 0; i < commands.size(); i++) {
			int file_desc = open("./TEMP.TXT", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(file_desc, 1);
			is_exit = execute_commands(commands[i]);
			close(file_desc);
		}

		std::ifstream file("./TEMP.TXT");
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (prompt_changed == false)
			content += prompt;
		else
			prompt_changed = false;
		write(ns,content.c_str(), static_cast<int>(content.size()));
		content.clear();
	}
	perror("read");	// ak klient skonci (uzavrie soket), nemusi ist o chybu

	close(ns);
	close(s);
}

/* Part 7 - client mode ========================================================================================
*/

void launch_as_client() {

	string socket_path = "./sck";
	if (!shell_arguments.socket_path.empty())
		socket_path = shell_arguments.socket_path;

	int s, r;
	fd_set rs;	// deskriptory pre select()
	char msg[1000] = "hello world!";
	struct sockaddr_un ad;

	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, socket_path.c_str());
	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket: ");
		exit(2);
	}

	connect(s, (struct sockaddr*)&ad, sizeof(ad));	// pripojenie na server
	FD_ZERO(&rs);
	FD_SET(0, &rs);
	FD_SET(s, &rs);
	cout << "Mode : CLIENT\n";
	// toto umoznuje klientovi cakat na vstup z terminalu (stdin) alebo zo soketu
	// co je prave pripravene, to sa obsluzi (nezalezi na poradi v akom to pride)

	while (select(s + 1, &rs, NULL, NULL, NULL) > 0)
	{

		if (FD_ISSET(0, &rs))		// je to deskriptor 0 = stdin?
		{
			r = read(0, msg, 64);	// precitaj zo stdin (terminal)
//if (msg[r-1]=='\n') msg[r-1]=0;
			write(s, msg, r);	// posli serveru (cez soket s)
		}
		if (FD_ISSET(s, &rs))		// je to deskriptor s - soket spojenia na server?
		{
			r = read(s, msg, 1);	// precitaj zo soketu (od servera)
			msg[r] = 0;
			write(1, msg, r);	// zapis na deskriptor 1 = stdout (terminal)
		
		}
		FD_ZERO(&rs);	// connect() mnoziny meni, takze ich treba znova nastavit
		FD_SET(0, &rs);
		FD_SET(s, &rs);
	}
	perror("select");	// ak server skonci, nemusi ist o chybu
	close(s);
}

/*Part 8 - Standalone mode =================================================================================
*/
void launch_as_standalone() {
	int is_exit = 1;
	prompt = make_prompt("TCU");
	cout << "Mode : STANDALONE\n";
	while (true) {
		string input_line;
		vector<CommandObject> commands;
		if (prompt_changed == false)
			cout << prompt;
		else
			prompt_changed = false;
		getline(std::cin, input_line);
		commands = separate_commands(input_line);

		for (size_t i = 0; i < commands.size(); i++) {
			is_exit = execute_commands(commands[i]);
		}
	}
}

/*Part 9 - main function ==============================================================================
*/

int main(int argc, char* argv[]) {
	cout << "Shell started\n";
	bool no_error = init_arguments_object(argc, argv); 
	int success = 0;
	if (!no_error) {
		cout << "ERROR\n";
		return 0;
	}
	/*cout << "MODE: " << shell_arguments.mode << "\n";
	cout << "PORT: " << shell_arguments.port << "\n";
	cout << "SOCKET PATH:" << shell_arguments.socket_path << "\n";*/

	if (shell_arguments.mode == "HELP") {
		shell_help(argv);
	}
	else if (shell_arguments.mode == "STANDALONE" || shell_arguments.mode == "") {
		launch_as_standalone();
	}
	else if (shell_arguments.mode == "SERVER") {
		launch_as_server();
	}
	else if (shell_arguments.mode == "CLIENT") {
		launch_as_client();
	}
	cout << "Shell ended\n";
	return 0;
}