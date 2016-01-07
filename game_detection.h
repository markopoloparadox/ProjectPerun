#ifndef GAME_DETECTION
#define GAME_DETECTION

#include "QXmlStreamReader"
#include "QFile"
#include "QDir"
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <memory>
#include "funkcije.h"

#if defined (_WIN32)

//#include <Wtsapi32.h>   //set of API functions related to program activity for Windows XP and Windows Vista
//#pragma comment(lib, "Wtsapi32.lib")
#include <Windows.h>

#include <Psapi.h>      //set of API functions related to program activity for Windows 7 and newer operating systems
#include <TlHelp32.h>

#endif


char* game_running_in_background(char* process_name=NULL);

char* found_gameserver_address (char* gameprocess_name);

#endif // GAME_DETECTION

