#pragma warning(disable:4996)
#include <string>
#include <iostream>

using namespace std;

class ShellArguments final {
public:
	string mode = "";
	string port = "";
	string socket_path = "";
};

ShellArguments shell_arguments;

bool init_arguments_object(int length, char* arguments[]) {
	for (int i = 1; i < length; i++) {
		cout << arguments[i] << "\n";
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

int main(int argc, char* argv[])
{
	bool no_error = init_arguments_object(argc, argv);
	if (!no_error)
		cout << "ERROR\n";
	else
		cout << shell_arguments.mode << "\n" << shell_arguments.port << "\n" << shell_arguments.socket_path << "\n";
	return 0;
}