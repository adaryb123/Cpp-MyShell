#include <iostream>
#include <string>
#include <vector>
using namespace std;


class CommandObject final {
public:
	CommandObject(string command, vector<string> arguments,
		string output_file_name, string input_file_name) {
		m_command = command;
		for (size_t i = 0; i < arguments.size(); i++)
			m_arguments.push_back(arguments[i]);
		m_input_file_name = input_file_name;
		m_output_file_name = output_file_name;
	}

private:
	string m_command;
	vector<string> m_arguments;
	string m_input_file_name;
	string m_output_file_name;

	friend std::ostream& operator<<(std::ostream& lhs, const CommandObject& rhs);
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

	for (size_t i = 0; i < input_line.size(); i++) {
		bool is_delimiter = false;
		for (size_t j = 0; j < delimiters.size(); j++) 
			if (input_line[i] == delimiters[j])
				is_delimiter = true;

		if (!is_delimiter && input_line[i] != ';' && filename_ended) break;

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


	CommandObject commandObject{ first_word,other_words,output_file,input_file};
	result.push_back(commandObject);
	for (size_t i = 0; i < other_objects.size(); i++)
		result.push_back(other_objects[i]);

	return result;
	}

	/*
	// Returns first token 
	char* token = strtok(str, "-");
	string command;
	vector<string> tokens;
	while (token != NULL)
	{
		printf("%s\n", token);
		token = strtok(NULL, " \t\r\n\a");
	}*/

int main(int argc, char* argv[]) {
	string input_line;
	vector<CommandObject> commands;
	getline(std::cin, input_line);
	commands = createCommandObjects(input_line);
	for (size_t i = 0; i < commands.size(); i++)
		cout << i << ":\n" << commands[i] << "\n";
	return 0;
}