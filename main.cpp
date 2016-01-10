//this application is using 2 threads: main thread (which is doing almost all job) and additional thread (which is processing data and looking for relevant information) - stable communication between them is ensured using Dekker's algorithm
//this application package is using 2 processes: main process (which contains 2 threads that are previously mentioned) and additional process (which is Visual Basic Service running batch file in background that is generating relevant data) - stable communication between them is ensured using file properties (its existence and lock funcionality)
//if user runs application with administrative privileges, then (s)he can use all features; otherwise: all except information about gameserver on which is user playing (it cannot be collected with standard user privileges)

#include "loginwindow.h"
#include <QApplication>
#include <fstream>
#define LOCKFILE "lock.dat"

bool running_as_administrator () {      //if user can successfully run following command, he is administrator (so (s)he will probably be able to run them again in future)
#if defined (__linux__)
    return !getuid();   //returns 0 if user is root (superuser)
#endif
#if defined (_WIN32)
    std::stringstream output, input, recognition_pattern;
    input << "netsh wfp capture start cab=off traceonly=off keywords=none file=network_traffic && netsh wfp capture stop";
    recognition_pattern << "Run as administrator";
    std::shared_ptr<FILE> pipe(popen(input.str().c_str(), "r"), pclose);
    if (!pipe) {    //if message could not be returned, then we will assume that application is not run as administrator (even if it is, we won't use Shell commands because Operating System obviously doesn't respond)
        return false;
    }
    char buffer[256];
    while (!feof(pipe.get())) {
        if (fgets(buffer, 256, pipe.get()) != NULL)
            output << buffer;       //transfering output data from Shell to output stringstream object
    }
    if (output.str().find(recognition_pattern.str().c_str(),0)==-1) {   //if Shell doesn't return error message which contains text about lack of privileges
        return true;
    }
    else {
        return false;
    }
#endif
}

int main(int argc, char *argv[])
{
    bool adminMode = running_as_administrator();

    if (adminMode==true) {
        std::fstream file;
        file.open(LOCKFILE,std::ios::out);  //creating lock file - it will be used to determine following states:
                                                //if file exists - commands for network tracing will be run (during the run-time of application)
                                                //if file does not exists - Shell is being terminated (when user closes application, because there is no need to track traffic anymore)
                                                //if locked (opened for modifying by another process) -
                                                //XML file with traffic data is available to read
        file.close();
#if defined (__linux__)
        char* repeat = "while true ; sniffit & ; sleep 10 ; done";
#endif
#if defined (_WIN32)
        std::stringstream command;
        file.open("repeat.bat",std::ios::out);          //this batch script will be executed on many distribution of Windows Operating System
        file << "@ECHO OFF" << std::endl                //do not report any messages to visible output stream
             << ":loop" << std::endl                    //label which defines starting positon of loop
             //<< "%*" << std::endl                     //this line we could use if we want to forward additional parameters when calling this batch script
             << "2>nul (" << std::endl                  //prevents error messages
             << ">>lock.dat echo off" << std::endl      //try to open file in append mode
             << ") && (goto sleep)" << std::endl        //if it succeed (if no game is played), go on line where is label 'sleep'; otherwise go to next line
             << "netsh wfp capture start cab=off traceonly=off keywords=none file=network_traffic" << std::endl     //start capturing network traffic
             << "timeout /t 1 /nobreak" << std::endl                                    //wait for 1 second till some network traffic is generated
             << "netsh wfp capture stop" << std::endl                                   //stop tracing and generate required .XML file
             << "if not exist \"" << LOCKFILE << "\" goto exit" << std::endl            //if LOCKFILE ("lock.dat") does not exist, that means that this application has deleted it and it is no longer required to run this batch script
             << ":sleep" << std::endl                                                   //label which represents position where one of previous commands is jumping directly
             << "timeout /t 9 /nobreak" << std::endl                                    //sleeping for 9 seconds (in this time is content of .XML file read by this application)
             << "goto loop" << std::endl                                                //repeat all previous steps
             << ":exit";
        file.close();

        file.open("background_service.vbs",std::ios::out);    //this script (i.e. Service) will allow that process runs in background (user has no idea that it is running)
        file << "Dim WinScriptHost" << std::endl
             << "Set WinScriptHost = CreateObject(\"WScript.Shell\")" << std::endl
             << "WinScriptHost.Run Chr(34) & \"repeat.bat\" & Chr(34), 0" << std::endl
             << "Set WinScriptHost = Nothing";
        file.close();
        command << "start background_service.vbs";
        system( command.str().c_str() );

#endif
    }

    short exitStatus;
    do {
        QApplication a(argc, argv);
        LoginWindow w(adminMode);   //we are sending information about having administrative privileges to constructor of first form which will be opened
        //w.setAttribute(Qt::WA_DeleteOnClose);      //this will call destructors of all classes which were closed manually or which were closed by parent class (child class needs to have pointer to parent class forwarded in its constructor in that case)
        w.show();
        exitStatus = a.exec();
    } while (exitStatus == 1);      //we are opening again if exit status is 1 (if user decided to restart application (or login as another user))

    remove(LOCKFILE);       //file with name LOCKFILE (currently "lock.dat") is deleted, so Shell (which is executing netsh command) will be terminated after few seconds

#if defined (_WIN32)
    remove("repeat.bat");
    remove("background_service.vbs");
#endif

    return exitStatus;
}
