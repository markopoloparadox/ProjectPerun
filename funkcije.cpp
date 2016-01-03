#include "funkcije.h"

int binarySearch (std::fstream &file,char* value,int first,int last) {  //should be called only indirectly by calling binarySearchWrapper function
    if (first>last) {
        return -1;
    }
    int mid=(first+last)/2;
    file.seekg(mid*sizeof(tGames));
    tGames tmp;
    file.read( (char*)&tmp,sizeof(tGames) );
    if (strcmp(value,tmp.processName)<0) {
        return binarySearch(file,value,first,mid-1);
    }
    else if (strcmp(value,tmp.processName)>0) {
        return binarySearch(file,value,mid+1,last);
    }
    else {
        return mid;
    }
}

int binarySearchWrapper (std::fstream &file,char* processName) {  //for searching records in gameslist.dat file - IT MUST BE PREVIOUSLY OPENED FOR READING! (opening a file takes a lot of time, so it is much more appropriate to manually set it's starting and ending lifecycle's point (especially when using nested loops))
    file.seekg(0,std::ios::end);
    int numOfRecords=file.tellg()/sizeof(tGames);
    int result=binarySearch (file,processName,0,numOfRecords-1);
    return result;
}
