#include "launch_game.h"
#include <QDebug>

void start_program (const char* prog_name,const char* ip=NULL,const char* port=NULL) {    //start game which server flagged as supported (in gameslist.dat file) and which path is defined (in gamepath.dat file)
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

    if (ip!=NULL && strcmp(gameRecord.multiplayerCommandLineArguments,"\0")!=0) {   //if in this function is forwarded ip address and port of some remote server and if there exists a way to join specific gameserver in game directly via command line
        std::string launchArguments = gameRecord.multiplayerCommandLineArguments;
        if (launchArguments.length()==0) {

        }
        launchArguments.replace(launchArguments.find("%%ip%%"),6,ip);  //6 represents length of IP adress's placeholder
        launchArguments.replace(launchArguments.find("%%port%%"),8,port);   //8 represents length of port's placeholder
        command << " " << launchArguments.c_str();
    }
    command << " " << pathRecord.customExecutableParameters;

    system(command.str().c_str());  //executing
}

