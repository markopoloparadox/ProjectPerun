# ProjectPerun
Instant messaging application with game detection and activity tracking

Development of this system has been motivated by discontinuation of [Xfire](https://en.wikipedia.org/wiki/Xfire) and [Raptr](https://en.wikipedia.org/wiki/Raptr) services - common services used by many gamers all around the globe. The idea was to cover all the features that were supported by that platform.

This system has been written in following languages:
* C++ (Qt) – entire client application (solution is compilable with Qt 5.10 development framework with following compilers: GCC 5.3.1 on Linux and MSVC 2015 on Windows)
* C++ - server-side program for maintaining file of supported games (`gameslist.dat`)
* JavaScript (Node.js) – script that handles TCP messages, notifies users and stores/reads data in/from JSON files (`GameActivity.json` and `UserList.json`)

Portability of server-side script depends on Node.js runtime environment - it will most likely work fine on each platform that is supported by Node.js engine (tested on v8.9.4 on Windows 10 and on v.4.2.6 on Ubuntu 16.04 LTS).

On the other hand, client app works on distributions of Windows (Windows 7 and newer) and Linux (tested on Ubuntu 14.04 LTS and 16.04 LTS) operating system, although there are some features that are currently only available on Windows operating system. Some of them are following:
* taking in-game screenshots when pressing Alt + B hotkey
* detecting address of gameserver where user is playing

Most of solution is based on <i>publish-subscribe pattern</i> like notifying friends and other chat participants about new messages and activity change (custom status and game status). Exception are detection of active game (which is accomplished with separate thread that scans list of all active processes every 10 seconds) and detection of gameserver where the user is playing (that is also checked in previously mentioned thread by calling netsh WFP process for capturing network interfaces).

As screenshots of app's usage would provide insufficient insight, overview of most relevant app's features is shown in the following video:

[![](/images/thumbnail.png?raw=true)](https://www.youtube.com/watch?v=4zYpEBgGpc0)