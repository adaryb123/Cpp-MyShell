#pragma once

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
#include <map>

using namespace std;

string make_prompt(string mode) {
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

	string output = "";
	for (size_t i = 0; i < mode.size(); i++) {
		if (mode[i] == 'T' || mode[i] == 't')
			output += time_string;
		else if (mode[i] == 'C' || mode[i] == 'c')
			output += ">" + computer_name;
		else if (mode[i] == 'U' || mode[i] == 'u')
			output += "@" + username;
	}
	output += "#";

	return output;
}