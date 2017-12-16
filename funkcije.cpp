#include "funkcije.h"

int binarySearch (std::fstream &file,const char* value,int first,int last, short recordSize) {  //should be called only indirectly by calling binarySearchWrapper function
    if (first>last) {
        return -1;
    }
    int mid=(first+last)/2;
    file.seekg(mid*recordSize);
    char tmp[50];   //in tmp will be stored name of process of each game stored in gameslist.dat file
    file.read( tmp,50 );
    if (strcmp(value,tmp)<0) {
        return binarySearch(file,value,first,mid-1,recordSize);
    }
    else if (strcmp(value,tmp)>0) {
        return binarySearch(file,value,mid+1,last,recordSize);
    }
    else {
        return mid;
    }
}

int binarySearchWrapper (std::fstream &file,const char* processName) {  //for searching records in gameslist.dat file (by default) or in gamepath.dat file (if value of last parameter is true) - IT MUST BE PREVIOUSLY OPENED FOR READING! (opening a file takes a lot of time, so it is much more appropriate to manually set it's starting and ending lifecycle's point (especially when using nested loops))
    file.seekg(0,std::ios::end);
    short recordSize = sizeof(tGames);
    return binarySearch ( file , processName , 0 , file.tellg()/recordSize -1 , recordSize );
}

QString divide_on_multiple_lines(QString longText, int charsPerLine) {  //text is not wrapping on word (but on whitespaces)
    QStringList words = longText.split(' ');
    longText.clear();
    short num_of_chars_in_current_line = 0;
    short num_of_words = words.length();
    for (int i=0 ; i<num_of_words ; i++,num_of_chars_in_current_line++) {
        if (num_of_chars_in_current_line + words[i].length() <= charsPerLine) {
            longText.append(words[i]);
            num_of_chars_in_current_line += words[i].length();
        }
        else {
            longText.append("\r\n");
            longText.append(words[i]);
            num_of_chars_in_current_line = words[i].length();
        }
        longText.append(' ');
    }
    longText.resize(longText.length()-1);   //removing space after last word in longText
    num_of_chars_in_current_line--;

    for (int i=0 ; i<charsPerLine-num_of_chars_in_current_line ; i++) {
        longText.append('.');
    }
    return longText;
}

QString seconds_to_HMS(double durationDouble)
{
  int duration = (int) durationDouble;
  QString res;
  int seconds = duration % 60;
  duration /= 60;
  int minutes = duration % 60;
  duration /= 60;
  int hours = duration;
  if(hours == 0)
      if (minutes == 0)
          return QString("%1s").arg( QString::number(seconds).rightJustified(2,'.') );
      else
          return QString("%1min:%2s").arg( QString::number(minutes).rightJustified(2,'.') ).arg( QString::number(seconds).rightJustified(2,'0') );
  return QString("%1h:%2min:%3s").arg( QString::number(hours).rightJustified(5,'.') ).arg( QString::number(minutes).rightJustified(2,'0') ).arg( QString::number(seconds).rightJustified(2,'0') );
}

char* stringToLowerCase(char* string) {
    char* tmp = new char [50];
    int i;
    for (i=0 ; string[i]!='\0' ; i++) {
        if (string[i]>='A' && string[i]<='Z') {
            tmp[i] = string[i] + 32;
        }
        else {
            tmp[i] = string[i];
        }
    }
    tmp[i] = '\0';
    return tmp;
}
