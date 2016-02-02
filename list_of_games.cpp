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
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    if (!file) {
    	std::cout<<"There is no gameslist.dat file in directory where this program is. You still haven't downloaded successfuly file that contains list of supported games or it was deleted manually."<<std::endl;
    	file.close();
    	file.clear();
	}
	else {
		tGames gameRecord;
	    int counter=0;
	    while (true) {
	        file.read( (char*)&gameRecord,sizeof(tGames) );
	        if (file.eof()==true) {
	            break;
	        }
	        counter++;
	        std::cout << counter << '\t' << "Process name: " << gameRecord.processName << std::endl;
	        std::cout << '\t' << "Full game name: " << gameRecord.fullName << std::endl;
	    }
	    file.close();
	    file.clear();
	}
	std::cout << std::endl;
	file.open("gamepath.dat",std::ios::in | std::ios::binary);
	if (!file) {
		std::cout<<"There is no gamepath.dat file in directory where this program is. You still haven't specified games from list of supported games that you have on your computer or it was deleted manually."<<std::endl;
		file.close();
		file.clear();
	}
	else {
		tPath gamePath;
		int counter=0;
		while (true) {
			file.read( (char*)&gamePath,sizeof(tPath) );
			if (file.eof()==true) {
				break;
			}
			counter++;
			std::cout << counter << '\t' << "Process name: " << gamePath.processName << std::endl;
			std::cout << '\t' << "Game directory path: " << gamePath.path << std::endl;
		}
	}
	std::cout << "\nPress any key to continue..." << std::endl;
	char sth_to_receive[2];
	std::cin.getline(sth_to_receive,2,'\n');
	return 0;
}

