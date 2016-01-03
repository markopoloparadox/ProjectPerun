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

//#include <Wtsapi32.h>   //set of API functions related to program actifivity for Windows XP and Windows Vista
//#pragma comment(lib, "Wtsapi32.lib")
#include <Windows.h>

//Use K32EnumProcesses for better performance
//but that will cut off XP and Vista support!
//Here I'll stick with older Psapi.dll
#include <Psapi.h>      //set of API functions related to program activity for Windows 7 and newer operating systems
#pragma comment(lib, "Psapi.lib")
#include <TlHelp32.h>
//Now we took care of all possibilities!

#endif

#if defined (__linux__)

    /* Linux. --------------------------------------------------- */
    // ps -au    //prikazuje sve trenutno aktivne procese    // ps -ef	//prikazuje SVE aktivne procese
#endif


char* game_running_in_background();

bool is_still_active (const char* process_name);

void start_packet_tracing ();

const char* found_gameserver_address (const char* gameprocess_name);

void send_notification_message (char* played_game_name,char* gameserver_info);

void game_detection ();

#endif // GAME_DETECTION

