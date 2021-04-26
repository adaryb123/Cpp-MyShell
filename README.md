# Cpp-MyShell

The program acts like a linux shell. It can execute basic shell commands using library functions fork() and exec(), as well as custom-made functions.


Supported special characters: "<" , ">" , "\" , ";" , "|", "#" . 


Custom made function "prompt" lets user modify the command prompt.

Run the program with argument "-h" to run in help mode. (prints the same as custom-made "help" function)

Run the program with argument "-a" or with no argument, to run in standalone mode (all in 1 window)

Run the program with argument "-s" to run in server mode, and with argument "-c" to run in client mode.
(server recieves the commands from client and sends the output back to client, via local sockets)

When running as client or server, add argument "-u" followed with path to local socket, to specify the socket used for comunnication. 
Otherwise, default socket "./sck" is used.

End the program with "quit" command.

##Hovewer

Pipes "|" only work in standalone mode, and can only be used once cause they shutdown the program after they are executed, for some reason.

"prompt" command also works only in standalone mode

