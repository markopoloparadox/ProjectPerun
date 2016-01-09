#include <iostream>
#include <cstdlib>
int main () {
	std::cout<<"If you see blank window and cursor is blinking, then that means that Server is running."<<std::endl;
	std::cout<<"Hint: for adding support for new games, launch 'gameLibraryManager.exe' and manipulate freely with data"<<std::endl;
	system("node Server.js");
	return 0;
}
