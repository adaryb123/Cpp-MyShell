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

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#pragma warning(disable:4996)
using namespace std;


/* Part 1: custom shell functions =======================================================================================================
source: https://brennan.io/2015/01/16/write-a-shell-in-c/  */
/*
  Function Declarations for builtin shell commands:
 */
int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
string builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char**) = {
  &shell_cd,
  &shell_help,
  &shell_exit
};

int shell_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}

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
	int i;
	printf("Stephen Brennan's shell\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < shell_num_builtins(); i++) {
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
	return 0;
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

/*Part 3 - my command prompt text ==============================================================================
*/

string make_prompt() {
	time_t mytime = time(NULL);
	char* time_str_c = ctime(&mytime);
	time_str_c[strlen(time_str_c) - 1] = '\0';
	string time_string{ time_str_c };
	time_string = time_string.substr(10, 10);

	struct passwd* p = getpwuid(getuid());  
	string username{ p->pw_name };

	char hostname[15];
	gethostname(hostname, sizeof(hostname));  
	string computer_name{ hostname };
	string output = time_string + " " + username + "@" + hostname + "#";

	return output;
}

/* Part 4 - CommandObject class, represents 1 line of command entered by user ================================================
*/

class CommandObject final {
public:
	CommandObject(string command, vector<string> arguments,
		string output_file_name, string input_file_name, std::shared_ptr<CommandObject> reciever) {
		m_command = command;
		for (size_t i = 0; i < arguments.size(); i++)
			m_arguments.push_back(arguments[i]);
		m_input_file_name = input_file_name;
		m_output_file_name = output_file_name;
		m_reciever = reciever;
	}

	string m_command;
	vector<string> m_arguments;
	string m_input_file_name;
	string m_output_file_name;
	std::shared_ptr<CommandObject> m_reciever;

	friend std::ostream& operator<<(std::ostream& lhs, const CommandObject& rhs);
	
	char* get_command_as_c() {
		char* c_string = new char[m_command.length() + 1];
		strcpy(c_string, m_command.c_str());
		return c_string;
	}

	char** get_arguments_as_c() {
		size_t array_size = m_arguments.size() + 2;
		/*if (!m_input_file_name.empty())
			array_size += 2;
		if (!m_output_file_name.empty())
			array_size += 2;*/
		char** arguments = new char* [array_size];

		size_t i = 0;
		arguments[i] = new char[m_command.length() + 1];
		strcpy(arguments[i], m_command.c_str());
		for (i = 0; i < m_arguments.size(); i++) {
			arguments[i+1] = new char[m_arguments[i].length() + 1];
			strcpy(arguments[i+1], m_arguments[i].c_str());
		}
		i++;
		/*if (!m_input_file_name.empty()) {
			arguments[i] = new char[2];
			strcpy(arguments[i], "<");
			i++;
			arguments[i] = new char[m_input_file_name.length() + 1];
			strcpy(arguments[i], m_input_file_name.c_str());
			i++;
		}
		if (!m_output_file_name.empty()) {
			arguments[i] = new char[2];
			strcpy(arguments[i], ">");
			i++;
			arguments[i] = new char[m_output_file_name.length() + 1];
			strcpy(arguments[i], m_output_file_name.c_str());
			i++;
		}*/
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
	lhs << "INPUT FILE: " << rhs.m_input_file_name << "\n";
	lhs << "OUTPUT FILE: " << rhs.m_output_file_name << "\n";
	if (rhs.m_reciever != nullptr)
		lhs << "PIPE TO COMMAND : " << rhs.m_reciever.get()->m_command << "\n";
	return lhs;
}

vector<CommandObject> createCommandObjects(string input_line) {

	vector<CommandObject> result, other_objects;
	vector<char> delimiters = { ' ','\t','\r','\n','\a' };
	vector<char> special_characters = { '#',';','<','>','|' };
	char escape_character = '\\';
	string first_word = "", word = "" , input_file = "", output_file = "";
	vector<string> other_words;
	bool first_done = false, output_file_mode = false, input_file_mode = false, filename_ended = false;
	std::shared_ptr<CommandObject> sharedObject = nullptr;



	for (size_t i = 0; i < input_line.size(); i++) {
		bool is_delimiter = false;
		for (size_t j = 0; j < delimiters.size(); j++) 
			if (input_line[i] == delimiters[j])
				is_delimiter = true;

		if (!is_delimiter && input_line[i] != ';' && input_line[i] != '|' && filename_ended) break;

		if (is_delimiter) {
			if ((output_file_mode && !output_file.empty()) || (input_file_mode && !input_file.empty()))
				filename_ended = true;
			else{
				if (word.empty())
					continue;
				else {
					if (first_done)
						other_words.push_back(word);
					else {
						first_word = word;
						first_done = true;
					}
					word = "";
				}
			}
		}
		else if( input_line[i] == '\\') {
			if (i + 1 < input_line.size()) {
				bool next_is_special = false;
				for (size_t j = 0; j < special_characters.size(); j++)
					if (input_line[i+1] == special_characters[j])
						next_is_special = true;

				if (next_is_special) {
					i++;
					word += input_line[i];
					continue;
				}
				else {
					word += '\\';
					continue;
				}
			}
		}
		else if (input_line[i] == '#') {
			break;
		}
		else if (input_line[i] == ';') {
			string other_command = input_line.substr(i + 1);
			other_objects = createCommandObjects(other_command);
			break;
		}
		else if (input_line[i] == '|') {
			string other_command = input_line.substr(i + 1);
			other_objects = createCommandObjects(other_command);
			sharedObject = make_shared<CommandObject>(other_objects[0]);
			break;
		}	

		else if (input_line[i] == '>' && !output_file_mode && !input_file_mode) {
			output_file_mode = true;
		}
		else if (input_line[i] == '<'&& !output_file_mode && !input_file_mode) {
			input_file_mode = true;
		}
		else {
			if (input_file_mode)
				input_file += input_line[i];
			else if (output_file_mode)
				output_file += input_line[i];
			else
				word += input_line[i];
		}
	}
	if (word != "") {
		if (first_done)
			other_words.push_back(word);
		else
			first_word = word;
	}
	
	if (!first_word.empty()) {
		CommandObject commandObject{ first_word,other_words,output_file,input_file,sharedObject };
		result.push_back(commandObject);
	}
	for (size_t i = 0; i < other_objects.size(); i++)
		result.push_back(other_objects[i]);

	return result;
}

/* Part 5 - command execution ========================================================================
*/

bool execute_command(CommandObject command) {
	int pid, wpid;
	int status;
	char* command_as_c = command.get_command_as_c();
	char** arguments_as_c = command.get_arguments_as_c();
	int i = 0;

	for (i = 0; i < shell_num_builtins(); i++) {
		if (command.m_command == builtin_str[i]) {
			return (*builtin_func[i])(arguments_as_c);
		}
	}
	i = 0;
	pid = fork();
	if (pid == 0) {
		// Child process
		if (execvp(command.get_command_as_c(), command.get_arguments_as_c()) == -1) {
			perror("shell");
		}
		delete[] command_as_c;
		while (true) {
			if (arguments_as_c[i] == NULL)
				break;
			delete[] arguments_as_c[i];
			i++;
		}
		delete[] arguments_as_c;
		exit(-1);
	}
	else if (pid < 0) {
		// Error forking
		perror("shell");
	}
	else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	
	delete[] command_as_c;
	i = 0;
	while (true) {
		if (arguments_as_c[i] == NULL)
			break;
		delete[] arguments_as_c[i];
		i++;
	}
	delete[] arguments_as_c;
	return true;
}

bool execute_line(vector <CommandObject> commands) {
	bool error = false;
	for (size_t i = 0; i < commands.size(); i++) {
		error = execute_command(commands[i]);
		if (error)
			return false;
	}
	return true;
}

/* Part 6 - server mode ==================================================================================
*/

//server from classmate

int launch_as_server() {
	int i, s, ns, r;
	fd_set rs;
	char buff[64], msg[] = "server ready\n";
	struct sockaddr_un ad;

	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, "./sck");
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

	// toto je nieco ako uvitaci banner servera
	// bude to fungovat vdaka tomu, ze klient je schopny citatat aj zo servera, 
	// aj z terminalu sucasne (v lubovolnom poradi)
////	strcpy(buff, "Hello from server\nSend a string and I'll send you back the upper case...\n");
////	write(ns, buff, strlen(buff)+1);

	while ((r = read(ns, buff, 64)) > 0)	// blokuju citanie, cakanie na poziadavku od klienta
	{
		buff[r] = 0;			// za poslednym prijatym znakom
		printf("received: %d bytes, string: %s\n", r, buff);
		for (i = 0; i < r; i++) buff[i] = toupper(buff[i]);
		printf("sending back: %s\n", buff);
		write(ns, buff, r);		// zaslanie odpovede
	}
	perror("read");	// ak klient skonci (uzavrie soket), nemusi ist o chybu

	close(ns);
	close(s);
	return 0;
}

/* Part 7 - client mode ========================================================================================
*/

int launch_as_client() {
	int s, r;
	fd_set rs;	// deskriptory pre select()
	char msg[64] = "hello world!";
	struct sockaddr_un ad;

	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, "./sck");
	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket: ");
		exit(2);
	}

	printf("running as client\n");
	connect(s, (struct sockaddr*)&ad, sizeof(ad));	// pripojenie na server
	FD_ZERO(&rs);
	FD_SET(0, &rs);
	FD_SET(s, &rs);
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
			write(1, msg, r);	// zapis na deskriptor 1 = stdout (terminal)
		}
		FD_ZERO(&rs);	// connect() mnoziny meni, takze ich treba znova nastavit
		FD_SET(0, &rs);
		FD_SET(s, &rs);
	}
	perror("select");	// ak server skonci, nemusi ist o chybu
	close(s);
	return 0;
}

/*Part 8 - Standalone mode =================================================================================
*/
int launch_as_standalone() {
	bool exit = false;
	while (!exit) {
		cout << make_prompt();
		string input_line;
		vector<CommandObject> commands;
		getline(std::cin, input_line);
		commands = createCommandObjects(input_line);
		exit = execute_line(commands);
		//for (size_t i = 0; i < commands.size(); i++)
			//cout << i << ":\n" << commands[i] << "\n";
	}
	return 0;
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
	cout << "MODE: " << shell_arguments.mode << "\n";
	cout << "PORT: " << shell_arguments.port << "\n";
	cout << "SOCKET PATH:" << shell_arguments.socket_path << "\n";

	if (shell_arguments.mode == "HELP") {
		shell_help(argv);
	}
	else if (shell_arguments.mode == "STANDALONE" || shell_arguments.mode == "") {
		success = launch_as_standalone();
	}
	else if (shell_arguments.mode == "SERVER") {
		success = launch_as_server();
	}
	else if (shell_arguments.mode == "CLIENT") {
		success = launch_as_client();
	}
	
	return success;
}