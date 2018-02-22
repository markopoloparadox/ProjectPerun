#ifndef FUNKCIJE
#define FUNKCIJE

#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <QString>
#include <QRegularExpression>

struct tGames {
    char processName[50];
    char fullName[250];
    char multiplayerCommandLineArguments[100];
    char registryKeyFullname[250];
};

struct tPath {
    char processName[50];
    char path[250];
    char customExecutableParameters[100];
};

int binarySearch (std::fstream &file,const char* value,int first,int last, short recordSize);

int binarySearchWrapper (std::fstream &file,const char* processName);

QString convertSecondsToHmsFormat (double durationDouble);

char* stringToLowerCase(char* string);

QString extractGameNameOnly (QString gameStatus);

#endif // FUNKCIJE
