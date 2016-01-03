#include "game_detection.h"
#include <QMessageBox>   //temporary!!!!!!!!!!!!!!!!!!!!!!!!!
#include <QDebug>

char* game_running_in_background() {  //background...does not necessarily mean that wanted process is minimized/inactive
    char* process_name = new char [50];    //assume that nothing is found
    bool found = false;

    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);

    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if(hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32 = {0};
        pe32.dwSize = sizeof(pe32);

        if(::Process32First(hSnapshot, &pe32)) {
            while (true) {
                std::wcstombs(process_name,pe32.szExeFile,50);    //converts process name which is in UTF-16 Windows format into 8-bit ASCII and store it into process_name
                //Check if it's our process
                for (int i=0 ; process_name[i]!='\0' ; i++) {   //convert process name into lowercase (because MS DOS derived operating system does not make changes between upper and lower letters in file names)
                    if (process_name[i]>=65 && process_name[i]<=90) {
                        process_name[i]+=32;
                    }
                }
                if( binarySearchWrapper(file,process_name)!=-1 ) {
                    //Looks like some game is running!
                    found = true;
                    //We're done...
                    break;
                }

                if(!::Process32Next(hSnapshot, &pe32)) {   //iterating through list of processes
                    break;
                }
            }
        }

        ::CloseHandle(hSnapshot);
    }
    file.close();

    if (found) {
        return process_name;
    }
    else {
        delete [] process_name;
        return NULL;
    }
}

bool is_still_active (const char* process_name) {
    std::stringstream command, output;
#if defined (__linux__)
    command << "pstree | grep -q \"" << process_name << "\"";
    return system(command.str().c_str())==0;    //system() will return 0 if process is still running, -1 (what is also 'true') if isn't
#endif
#if defined (_WIN32)
    command << "start /MIN /B tasklist /FI \"ImageName eq " << process_name << "\" /NH";
    std::shared_ptr<FILE> pipe(popen(command.str().c_str(), "r"), pclose);
    if (!pipe) {
        output << "ERROR";
    }
    char buffer[128];
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            output << buffer;
    }
    if (output.str().find(process_name,0)==0) {   //process name should be at the beginning of grabbed string if process is running (so its starting position should be 0)
        return true;
    }
    else {
        return false;
    }
#endif
}

void start_packet_tracing () {
    //path = QDir::currentPath().toStdString();
    //start with packet tracing
//    start_tracing << "netsh trace start scenario=NetConnection capture=yes report=no persistent=yes maxSize=1 overwrite=yes traceFile=" << path << "\\file.etl";   //this command takes too long to generate required .XML file (about 30 secounds)
//    start_tracing << "netsh wfp capture start cab=off traceonly=off keywords=none file=" << path "\\network_traffic";
    system("start /MIN /B netsh wfp capture start cab=off traceonly=off keywords=none file=network_traffic");
    //stop shortly after
    sleep(1);
    system("start /MIN /B netsh wfp capture stop");
    sleep(1);
/*  Is no longer required because netsh wfp program with forwarded arguments is not archiving required files into .cab file
    //create new directory (if it does not currently exist) where archived (.cab) file will be extracted
    make_directory << "mkdir " << path << "\\files";
    system(make_directory.str().c_str());
    //extract archive into folder
    extract_file << "expand " << path << "\\file.cab -F:* " << path << "\\files";
    system(extract_file.str().c_str());
    */
}

/*char* found_gameserver_address (char* gameprocess_name) {  //search for gameserver's public IP address of game currently playing; return true if it exists (IP address and Port of that gameserver are later accessible via global objects destIP and destPort)
    start_packet_tracing();
    std::string destIP, destPort, asStringVal, appLayerAppName;
    QFile *xmlFile = new QFile("network_traffic.xml");
        if (!xmlFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
//                QMessageBox::critical("Load XML File Problem",
//                "Couldn't open wfpstate.xml to analyse packet traffic for identifying IP address of gameserver you are playing on",
//                QMessageBox::Ok);
                return NULL;
        }
    QXmlStreamReader *xmlReader = new QXmlStreamReader(xmlFile);
    bool validDestAddr=false;   //assume that current IP address is not valid, i.e. public (of course it isn't because we still haven't started analysing XML file)
    //Parse the XML until we reach end of it
//    QDomDocument document;   //QDomDocument, QDomElement and other similar classes related to DOM are usually used when code is wanted to make easier to understand (but then the efficiency is slightly worse)
//    if (!document.setContent(xmlFile)) {  //required libraries to include: "QtXml/QDomDocument", "QtXml/QDomElement"
//        return false;
//    }
//    QDomElement root = document.firstChildElement();
    while(!xmlReader->atEnd() && !xmlReader->hasError()) {
            // Read next element
            QXmlStreamReader::TokenType token = xmlReader->readNext();
            //If token is just StartDocument - go to next
            if(token == QXmlStreamReader::StartDocument) {
                    continue;
            }
            //If token is StartElement - read it
            if(token == QXmlStreamReader::StartElement) {
                    //asString field contains process name (with its path) that received/sent packet
                    if(xmlReader->name() == "remoteV4Address") {
                        destIP = xmlReader->readElementText().toUtf8().toStdString();  //method readElementText() is returning QString object as a value which is UTF-16 encoding format, so we convert it into UTF-8 QtString and then convert it into std::String object
                        if ( !(destIP.find("192.168.")==0 || destIP.find("127.0.0.1")==0 || destIP.find("10.",0)==0 ) || (destIP.find("172.",0)==0 && stoi(destIP.substr(4,3))>15 && stoi(destIP.substr(4,3))<32) ) { //if destination address is not Loopback or Private address, then save it
                            validDestAddr=true;
                        }
                        continue;  //remoteV4Address field cannot contain any other field (or Tag, how it is called in HTML)
                    }
                    if(xmlReader->name() == "remotePort") {
                        destPort = xmlReader->readElementText().toUtf8().toStdString();
                        continue;
                    }
                    if(xmlReader->name() == "asString") {
                        asStringVal = xmlReader->readElementText().toUtf8().toStdString();
                        unsigned short i=asStringVal.length()-1;
                        //starting from the end of the string and positioning to the beginning of process name (without its path)
                        while (asStringVal.c_str()[i]!='\\') {
                            i--;
                        }
                        i+=2;
                        //positioning to the end of the total string and saving its required part to separate object
                        appLayerAppName.clear();
                        while (i<asStringVal.length()) {
                            appLayerAppName+=asStringVal.c_str()[i];
                            i+=2;  //asString value is in following type: p.a.t.h./.p.r.o.c.e.s.s...e.x.e... (each symbol on even place has to be ignored)
                        }
                        asStringVal.resize(asStringVal.length()-1);
                        if (strcmp(asStringVal.c_str(),gameprocess_name)==0) {
                            //if current analysed packet is used by currently played game
                            if (validDestAddr==true) {  //preview IP address of game server only if its IP address is public
                                break;  //go out of this while structure to remaining code in this function (clearing XMLreader and closing XMLfile)
                            }
                        }
                        else {
                            validDestAddr=false;
                        }
                    }
            }
    }

//    if(xmlReader->hasError()) {
//            QMessageBox::critical(this,
//            "xmlFile.xml Parse Error",xmlReader->errorString(),
//            QMessageBox::Ok);
//            return;
//    }

    //close reader and flush file
    xmlReader->clear();
    xmlFile->close();
    if (validDestAddr==true) {  //if validDestAddr contains state true, then public address of gameserver was succesfully found
        destIP += ":" + destPort;
        char* retval = new char [destIP.length()];
        strcpy(retval,destIP.c_str());
        return retval;
    }
    else {
        return NULL;
    }
}*/

char* found_gameserver_address(char* gameprocess_name) {  //XML file with network traffic is read from END to beginning
    start_packet_tracing();     //first we need to track network packets and generate required XML file
    int numOfTabs = 0;  //number of tab spaces which are set next to each other
    std::fstream file;
    file.open("network_traffic.xml",std::ios::in);
    file.seekg(0,std::ios::end);      //we will start from the end of file because answer is there (and if it isn't, then it also isn't in whole file (because most of file contains unnecessary data)
    int i = file.tellg()-1;    //position from which is file pointer reading (it's necessary to set it one byte before EOF (end of file), so it won't read trash character which is behind range of file
    char searchedOne[200];       //that will be text we will look for (it will point to packet related to game we are playing (if we are playing any))
    short j;    //this will be used as index of character in string we are iterating through
    short searchedOneStrLen = strlen(gameprocess_name);     //string length of name of game process
    for (j=0 ; j<searchedOneStrLen ; j++ ) {        //iterate through each character of name of game process
        searchedOne[j*2] = gameprocess_name[j];     //save on each even position j-th character
        searchedOne[j*2+1] = '.';                   //save on each odd position dot ('.')   - because in text file in which we will search, after every character comes dot (probably because process name is usually in UTF16 (each character takes 2 bytes), so here is each character represented with dot next to itself)
    }
    searchedOne[j*2] = searchedOne[j*2+1] = '.';    //because after each process name there are also 2 another dots (probably because there was NullString which was encoded to dot)
    searchedOne[j*2+2] = '\0';
    strcat(searchedOne,"</asString>\0");    //and then text that represent XML element in which is contained
    char gameProcessasString[200];
    strcpy(gameProcessasString,searchedOne);   //gameProcessasString is required in case that address found for played game is not valid, so we need to search again for next appearence of that (game) process
    searchedOneStrLen = strlen(searchedOne)-1;        //length of text which represents process name in encoded format (decreased by one, so we don't to decrease it later every time in loop (because last character of string is on (N-1)th position))
    short endingBlockIdentifierNumber = searchedOneStrLen-11;   // 11 is string length of "</asString>" which is ending identifier of element asString
    short type = 0;	//0...processName , 1...remotePort , 2...remoteAddress
    char port[6];
    char ip_addr[16];
    bool endSoon = false;   //firstly we will assume that end is not yet near
    while (true) {      //"infinite loop", but it will be cancelled with "break" command (in every condition!) - eventually it could do wrong if network_traffic.xml file isn't properly created)
        file.seekg(i);      //we put file pointer for reading on i-th position
        char c;
        file.get(c);     //we read content from that position
        if (endSoon==true) {    //if we are in block of elements which represents end of our searching quest
            if (c=='\t') {      //and if character (which has been currently read) is tab space
                numOfTabs++;    //then increment counter which is counting tab spaces which are next to each other
            }
            else {      //if we have read some character which is not tab space
                if (numOfTabs==2) {     //then check if there were exactly 2 tab spaces before (because all other XML element inside of ending block have 3 or more tab spaces at the beginning of line)
                    file.close();   //we won't need this file anymore
                    return NULL;    //and exit function - no appropriate address was found (game wasn't played via Internet)
                }
                else {  //if there were more than 2 tab spaces in that line
                    numOfTabs = 0;  //set tab counter again on 0 (because we are not going to analyze next line (i.e. previous because we are reading file in reverse order)
                }
            }
        }
        for (j = searchedOneStrLen ; j>=0 && searchedOne[j]==c ; j--) {     //iterating through each character of text sample we are currently searching for - repeating as long as text sample is equivalent to current piece of text in file
            file.seekg(--i);    //we decrement index of byte which we need next time to read
            file.get(c);    //and now we read it
        }
        if (endingBlockIdentifierNumber > j) {  //text sample at the beginning contains "g.a.m.e._.n.a.m.e...e.x.e...</asString>", so if text "</asString> was read, that means that we entered ending block (part of document where packet data is listed)
            endSoon = true;     //and we are setting that end is near
        }
        if (j==-1) {    //if we found the string we are seaching for (we were comparing each character in file and each was the same (as one in string variable with text we are looking for)
            if (type==0) {      //check if we are currently looking for process (game) name
                strcpy(searchedOne,"</remotePort>\0");      //set new goal - to find port of game server on which we are playing
                searchedOneStrLen = strlen(searchedOne)-1;  //save also string length of "</remotePort>"
                type = 1;   //set that we are now looking for port number (not process name anymore)
            }
            else if (type==1) {     //check if we are currently looking for port number
                short k;
                char tmp[5];        //create variable where we will save each digit of port number (but in reversed order!)
                for (k = 0 ; c!='>' ; k++) {    //find out number of digits which port number contains (since it can be from 0 to 65535)
                    tmp[k] = c;         //save each digit in 'tmp' array
                    file.seekg(--i);    //set file pointer for reading on one place before (so we can read previous digit (or ending character '>'))
                    file.get(c);    //save previous character in variable 'c'
                }
                for (short l = 0 ; l<k ; l++) { //iterate through array which contains digits of port number but in reversed order
                    port[l] = tmp[k-1-l];   //put last character on first place (because 'tmp' array was in reversed order
                }
                port[k] = '\0';     //set NullString at the end what represents end of string
                strcpy(searchedOne,"</remoteV4Address>\0");     //next we need to find IP address of game server we are playing on
                searchedOneStrLen = strlen(searchedOne)-1;      //and save also length of "</remoteV4Address>"
                type = 2;   //set that we are now looking for IPv4 address (not port number anymore)
            }
            else {      //if we are looking for IPv4 address of game server on which we are playing on
                short k;
                char tmp[16];   //create variable where we will save each character of IPv4 address (but in reversed order!)
                for (k = 0 ; c!='>' ; k++) {    //find out string length of IPv4 address (since it can be from 0.0.0.0 (7 characters) to 255.255.255.255 (15 characters)
                    tmp[k] = c;     //save each character in 'tmp' array
                    file.seekg(--i);    //set file pointer for reading on one place before (so we can read previos character of IP address (or ending character '>'))
                    file.get(c);    //save previous chracter in variable 'c'
                }
                for (short l = 0 ; l<k ; l++) { //iterate through array which contains characters of IPv4 address but in reversed order
                    ip_addr[l] = tmp[k-1-l];    //put last character on first place (beause 'tmp' array was in reversed order
                }
                ip_addr[k] = '\0';      //set NullString at the end what represents end of string

                bool localAddr = false;     //assume that found address is not local
                char private_addresses[3][10] = { "192.168.\0" , "127.\0" , "10.\0" };      //list of local addresses (those can't exist on Internet)
                for (short l = 0 ; l<3 ; l++) {     //iterate through all private addresses
                    short m;
                    for (m = 0 ; m<strlen(private_addresses[l]) && ip_addr[m]==private_addresses[l][m] ; m++ ) {    //iterate through each character of private address and compare it with found address
                        ;       //do nothing specific
                    }
                    if (m==strlen(private_addresses[l])) {    //if found address is private
                        localAddr = true;       //then set that it is private
                        break;      //we don't need to compare it with other private addresses (because it can't any other)
                    }
                }
                if (localAddr==false) {     //if we still treat found IP address as public, check if it is maybe in range 172.16.0.0 - 172.31.255.255
                    char complex_private_address[6] = "172.\0";     //that's the beginning of that type of private address
                    short l;
                    for (l = 0 ; l<3 && complex_private_address[l]==ip_addr[l] ; l++) {     //check how many characters does found address and this private address have in common
                        ;
                    }
                    if (l==3) {     //if they have same first part of address (then this address might be private)
                        char secondPart[4];     //store second part of address in this array
                        secondPart[0] = ip_addr[4];
                        secondPart[1] = ip_addr[5];
                        secondPart[2] = ip_addr[6];
                        secondPart[3] = '\0';
                        if ( atoi(secondPart)>15 && atoi(secondPart)<32 ) {     //check if that second part of address is between 15 and 32 (excluding them)
                            localAddr = true;       //then set that found address is private
                        }
                    }
                }
                if (localAddr==true) {      //if found address is private
                    type = 0;       //then we need to find another (if there exists any), so we are again for packets of game we are playing
                    i--;    //we decrement position where file pointer will read next character
                    strcpy(searchedOne,gameProcessasString);
                    searchedOneStrLen = strlen(searchedOne)-1;
                    continue;          //we go back to the main loop where we are iterating through each character of file (on the place where we left)
                }

                char* ip_and_port = new char [22];  //create container where IP address and port will be stored (and returned from this function)
                file.close();   //we won't need this file anymore
                strcpy(ip_and_port,ip_addr);    //copy IP address in it
                strcat(ip_and_port,":");    //IP address and port number are usually divided with semi-colon sign (":")
                strcat(ip_and_port,port);   //attach on that string port number, too
                return ip_and_port;     //return that IP address with port number
            }
        }
        i--;    //each time in loop we decrement position of character (because in each loop we read at least 1 character
    }
}

void game_detection () {
    char *running_game, *gameserver_info;
    while (true) {
        gameserver_info = new char [22];
        running_game = game_running_in_background();
        if (running_game != NULL) {
            gameserver_info = found_gameserver_address(running_game);
        }
        //send_notification_message(running_game,gameserver_info);
        sleep(10);
        while (running_game!=NULL && is_still_active(running_game)==true) {
            char* tmp = found_gameserver_address(running_game);
            if (tmp == NULL && gameserver_info != NULL) {   //user was till now on some game server and now he is not
                delete [] gameserver_info;
                gameserver_info = NULL;
            }
            else if (tmp != NULL && gameserver_info == NULL) {  //user was till now in game, but wasn't on any game server
                gameserver_info = tmp;
            }
            else if (tmp == NULL && gameserver_info == NULL) {  //user still hasn't joined any game server
                sleep(10);
                continue;
            }
            else if (strcmp(tmp,gameserver_info)!=0) {  //user has stayed playing same game, but he switched to another game server
                delete [] gameserver_info;
                gameserver_info = tmp;
            }
            else if (strcmp(tmp,gameserver_info)==0) {  //if user plays on same server on which he played before 10 seconds
                delete [] tmp;
                sleep(10);
                continue;   //there is no need to send notification message if state hasn't changed
            }
            //send_notification_message(running_game,gameserver_info);
            delete [] tmp;
            sleep(10);
        }
        if (running_game!=NULL) {
            delete [] running_game;
        }
    }
}
