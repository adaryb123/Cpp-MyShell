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
	return lhs;
}

void createCommandObjects(string input_line) {
	vector<char> delimiters = { ' ','\t','\r','\n','\a' };
	vector<char> special_characters = { '#',';','<','>','|' };
	char escape_character = '\\';

	string first_word = "";
	vector<string> other_words;
	bool first_done = false;

	string word = "";
	for (size_t i = 0; i < input_line.size(); i++) {
		bool is_delimiter = false;
		for (size_t j = 0; j < delimiters.size(); j++) 
			if (input_line[i] == delimiters[j])
				is_delimiter = true;

		if (is_delimiter) {
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
		else
			word += input_line[i];
	}
	if (first_done)
		other_words.push_back(word);
	else 
		first_word = word;
		CommandObject commandObject{ first_word,other_words,"","" };
		cout << commandObject;
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
	getline(std::cin, input_line);
	createCommandObjects(input_line);
	return 0;
}