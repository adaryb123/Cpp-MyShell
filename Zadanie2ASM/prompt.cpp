#include <time.h>
#include <stdio.h>
#include <string.h>
#pragma warning(disable:4996)
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
using namespace std;

#include <pwd.h>

int main(void)
{
    time_t mytime = time(NULL);
    char* time_str_c = ctime(&mytime);
    time_str_c[strlen(time_str_c) - 1] = '\0';
    string time_string{ time_str_c };
    time_string = time_string.substr(10, 10);

    struct passwd* p = getpwuid(getuid());  // Check for NULL!
    string username{ p->pw_name };

    char hostname[15];
    gethostname(hostname, sizeof(hostname));  // Check the return value!
    string computer_name{ hostname };
    string output = time_string + " " + username + "@" + hostname + "#";
    cout << output << "\n";

    return 0;
}