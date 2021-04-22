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

using namespace std;


string prompt = "";
bool prompt_changed = false;

/* Part 1: custom shell functions =======================================================================================================
 */

//  Function Declarations for builtin shell commands:
void shell_cd(char** args);
void shell_help(char** args);
void shell_exit(char** args);
void shell_halt(char** args);
void shell_prompt(char** args);

 // Vector of builtin commands, followed by their corresponding functions.
vector<string> builtin_str = {
  "cd",
  "help",
  "quit",
  "halt",
  "prompt"
};

vector <void (*) (char**)> pointers = {
  &shell_cd,
  &shell_help,
  &shell_exit,
  &shell_halt,
  &shell_prompt,
};

/*
  Builtin function implementations 
*/

//change directory
void shell_cd(char** args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "shell: expected argument to \"cd\"\n");
	}
	else {
		if (chdir(args[1]) != 0) {
			perror("shell");
		}
	}
}

//print help messages
void shell_help(char** args)
{
	cout << "Enter commands as in normal shell\n";
	cout << "Custom functions are :\n";
	for (size_t i = 0; i < builtin_str.size(); i++) {
		cout << builtin_str[i] << "\n";
	}
	cout << "Use the man command for information on other programs.\n";
	cout << "Run the program with argument -c for client mode, or -s for server mode \n";
	cout << "When running as client or server, use argument -u followed with path to socket, to specify the socket used or communication\n";
}

//exit program
void shell_exit(char** args)
{
	exit(0);
}

//turn off computer
void shell_halt(char** args)
{
	system("shutdown -P now");
}

//modify the custom prompt
void shell_prompt(char** args)
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
}

/* Part 2 - program arguments given at start =====================================================================================================
*/

//this object will hold the arguments. Since we need to use them at various parts of code, it is stored as global variable.
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

//Line can contain multiple commands separated by ;
//Each command will be stored in separate object

class CommandObject final {
public:
	//construct object from string
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

	//these are the properties of command object
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

	//the execvp needs arguments in char**, not string, so we must convert  them

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

//print command object
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

//parse the line and create vector of commmand objects
vector<CommandObject> separate_commands(string input_line) {
	vector<char> delimiters = { ' ','\t','\r','\n','\a' };
	vector<char> special_characters = { '#',';','<','>','|' };
	//char escape_character = '\\';
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
void execute_commands(CommandObject command) {
	//check if command is one of the builtin fuctions
	for (size_t i = 0; i < builtin_str.size(); i++) {
		if (command.m_command == builtin_str[i]) {
			pointers[i](command.get_arguments_as_c());
		}
	}
	int pid, file_desc, file_desc2;
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
				//replace standard output with output file
				file_desc = open(command.m_output_file_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}

			if (command.m_has_input_file) {
				//replace standard input with input file, if such file exists
				if ((file_desc2 = open(command.m_input_file_name.c_str(), O_RDONLY)) < 0){
					perror("ERROR: File does not exist");
					return;
				}
				dup2(file_desc2, 0);
			}

			//exectute the part after pipe
			if (execvp(command.get_second_command_as_c(), command.get_second_arguments_as_c()) == -1)
				perror("shell");

		}
		else {
			if (command.m_has_output_file) {
				//replace standard output with output file
				file_desc = open(command.m_output_file_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(file_desc, 1);
			}

			if (command.m_has_input_file) {
				//replace standard input with input file, if such file exists
				if ((file_desc2 = open(command.m_input_file_name.c_str(), O_RDONLY)) < 0) {
					perror("ERROR: File does not exist");
					return;
				}
				dup2(file_desc2, 0);
			}

			//execute the command
			if (execvp(command.get_command_as_c(), command.get_arguments_as_c()) == -1)
				perror("shell");
		}
		exit(-1);
	}
	else if (pid > 0) {
		// Parent process
		int status;

		if (command.m_has_pipe) {
			// replace standard output with output part of pipe
			if (dup2(pipefd[1], 1) < 0)
				perror("dup2parent");
			// close unused unput half of pipe
			if (close(pipefd[0]) < 0)
				perror("closeparent");

			//execute the part before pipe
			if (execvp(command.get_command_as_c(), command.get_arguments_as_c()) == -1)
				perror("shell");
		}
		//wait for the child process to finish
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}


/* Part 6 - server mode ==================================================================================
*/
//Server will open a socket and wait for client to join. 
//Server executes the commands that are comming from client, and sends back the output

void launch_as_server() {

	//default socket
	string socket_path = "./sck";
	if (!shell_arguments.socket_path.empty())
		socket_path = shell_arguments.socket_path;

	int s, ns, r;
	char buff[1000] = "server ready\n";
	struct sockaddr_un ad;

	//open socket
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

	// connect only 1 client
	ns = accept(s, NULL, NULL);

	prompt = make_prompt("TCU");
	write(ns, prompt.c_str(), static_cast<int>(prompt.size()));

	cout << "Mode : SERVER\n";
	while ((r = read(ns, buff, 64)) > 0)	// wait for client input
	{
		buff[r] = 0;			

		//if the command has no output file, store its ouput in temprorary file, rather than stdout
		vector<CommandObject> commands;
		commands = separate_commands(string{ buff });
		for (size_t i = 0; i < commands.size(); i++) {
			int file_desc = open("./TEMP.TXT", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
			dup2(file_desc, 1);
			execute_commands(commands[i]);
			close(file_desc);
		}

		//now load the file to string and send it to client
		std::ifstream file("./TEMP.TXT");
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (prompt_changed == false)
			content += prompt;
		else
			prompt_changed = false;
		write(ns,content.c_str(), static_cast<int>(content.size()));
		content.clear();
	}
	perror("read");

	close(ns);
	close(s);
}

/* Part 7 - client mode ========================================================================================
*/

//client will connect to the servers socket
//client sends user input to server, and prints the output that is comming from the server

void launch_as_client() {

	//default socket
	string socket_path = "./sck";
	if (!shell_arguments.socket_path.empty())
		socket_path = shell_arguments.socket_path;

	int s, r;
	fd_set rs;
	char msg[1000] = "hello world!";
	struct sockaddr_un ad;

	//create socket
	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, socket_path.c_str());
	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket: ");
		exit(2);
	}

	//connect to server
	connect(s, (struct sockaddr*)&ad, sizeof(ad));	
	FD_ZERO(&rs);
	FD_SET(0, &rs);
	FD_SET(s, &rs);
	cout << "Mode : CLIENT\n";

	while (select(s + 1, &rs, NULL, NULL, NULL) > 0)
	{

		//read from stdin, send to server
		if (FD_ISSET(0, &rs))		
		{
			r = read(0, msg, 1000);
			write(s, msg, r);	
			if (strcmp(msg, "quit") == 0)
				exit(0);
		}
		//read from socket, print to stdout
		if (FD_ISSET(s, &rs))		
		{
			r = read(s, msg, 1);	
			msg[r] = 0;
			write(1, msg, r);	
		
		}
		FD_ZERO(&rs);	
		FD_SET(0, &rs);
		FD_SET(s, &rs);
	}
	perror("select");	
	close(s);
}

/*Part 8 - Standalone mode =================================================================================
*/
//read commands from stdin and print output to stdout
void launch_as_standalone() {
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
			execute_commands(commands[i]);
		}
	}
}

/*Part 9 - main function ==============================================================================
*/

//launch the mode selected in arguments
int main(int argc, char* argv[]) {
	cout << "Shell started\n";
	bool no_error = init_arguments_object(argc, argv); 
	if (!no_error) {
		cout << "ERROR\n";
		return 0;
	}
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