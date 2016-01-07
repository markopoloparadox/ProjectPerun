#ifndef FUNKCIJE
#define FUNKCIJE

#include <sstream>
#include <string>
#include <cstring>
#include <fstream>

struct tGames {
    char processName[50];
    char fullName[250];
    char multiplayerCommandLineArguments[100];
};

struct tPath {
    char processName[50];
    char path[250];
    char customExecutableParameters[100];
};

int binarySearch (std::fstream &file,const char* value,int first,int last);

int binarySearchWrapper (std::fstream &file,const char* processName);

#endif // FUNKCIJE

