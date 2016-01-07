#include <iostream>
#include <fstream>
#include <cstring>

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

int main () {
    std::fstream file;
    tGames gameRecord;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    int counter=0;
        //qDebug() << "hej";
    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()==true) {
            break;
        }
        counter++;
        std::cout << gameRecord.fullName << std::endl;
    }
    file.close();
    file.clear();
}

