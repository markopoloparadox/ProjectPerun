#include "launch_game.h"

void start_program (char* prog_name,char* ip=NULL,char* port=NULL) {    //start game which server flagged as supported (in gameslist.dat file) and which path is defined (in gamepath.dat file)
    std::stringstream command;

    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    tGames gameRecord;

    file.seekg( binarySearchWrapper(file,prog_name)*sizeof(tGames) , std::ios::beg );
    file.read( (char*)&gameRecord,sizeof(tGames) );
    file.close();

    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    tPath pathRecord;
    bool pathNotDefined=true;
    while (true) {
        file.read( (char*)&pathRecord,sizeof(tPath) );
        if (file.eof()==true) {
            pathNotDefined=false;
            break;
        }
        if (strcmp(pathRecord.processName,prog_name)==0) {
            break;
        }
    }

    file.close();
    if (pathNotDefined==false) {
        QMessageBox msgBox;
        msgBox.setText("Define game's path in settings to join game over your friend");
        msgBox.exec();
        file.clear();
        return;
    }
#if defined (_WIN32)
    command << "cd \"" << pathRecord.path << "\" && " << "start " << prog_name;
#endif
#if defined (__linux__)
    std::string path = pathRecord.path;
    path.replace(path.find("/home"),5,"~");             //replaces "/home" part of directory path with "~" because that's the only way how can be programs run when using absolute path
    command << path.c_str() << "/" << prog_name;
#endif

    if (ip!=NULL) {
        std::string launchArguments = gameRecord.multiplayerCommandLineArguments;
        launchArguments.replace(command.str().find("%%ip%%"),6,ip);  //6 represents length of IP adress's placeholder
        launchArguments.replace(command.str().find("%%port%%"),8,port);   //8 represents length of port's placeholder
        command << " " << launchArguments.c_str();
    }
    command << " " << pathRecord.customExecutableParameters;
    system(command.str().c_str());  //executing
}

