#ifndef FUNKCIJE
#define FUNKCIJE

#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <QString>
#include <QStringList>

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

int binarySearch (std::fstream &file,const char* value,int first,int last, short recordSize);

int binarySearchWrapper (std::fstream &file,const char* processName);

QString seconds_to_HMS (double durationDouble);

QString divide_on_multiple_lines(QString longText, int charsPerLine);

char* stringToLowerCase(char* string);

#endif // FUNKCIJE

