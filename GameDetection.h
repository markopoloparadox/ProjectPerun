#ifndef GAME_DETECTION
#define GAME_DETECTION

#include "QXmlStreamReader"
#include "QFile"
#include "QDir"
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#if defined(__linux__)
#include <unistd.h>
#endif
#include <memory>
#include "UsefulFunctions.h"

#if defined (_WIN32)

//#include <Wtsapi32.h>   //set of API functions related to program activity for Windows XP and Windows Vista
//#pragma comment(lib, "Wtsapi32.lib")
#include <Windows.h>

#include <Psapi.h>      //set of API functions related to program activity for Windows 7 and newer operating systems
#include <TlHelp32.h>

void sleep(int seconds);
#endif


char* getNameOfGameRunningInBackground(char* processName=NULL);

char* foundGameserverAddress (char* gameprocessName);

#endif // GAME_DETECTION

