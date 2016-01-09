#include "funkcije.h"

int binarySearch (std::fstream &file,const char* value,int first,int last) {  //should be called only indirectly by calling binarySearchWrapper function
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

int binarySearchWrapper (std::fstream &file,const char* processName) {  //for searching records in gameslist.dat file - IT MUST BE PREVIOUSLY OPENED FOR READING! (opening a file takes a lot of time, so it is much more appropriate to manually set it's starting and ending lifecycle's point (especially when using nested loops))
    file.seekg(0,std::ios::end);
    int numOfRecords=file.tellg()/sizeof(tGames);
    int result=binarySearch (file,processName,0,numOfRecords-1);
    return result;
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
  int seconds = (int) (duration % 60);
  duration /= 60;
  int minutes = (int) (duration % 60);
  duration /= 60;
  int hours = (int) (duration / 24);
  if(hours == 0)
      return QString("%1min:%2s").arg( QString::number(minutes).rightJustified(2,' ') ).arg( QString::number(seconds).rightJustified(2,'0') );
  if (minutes == 0)
      return QString("%1s").arg( QString::number(seconds).rightJustified(2,' ') );
  return QString("%1h:%2min:%3s").arg( QString::number(hours).rightJustified(5,' ') ).arg( QString::number(minutes).rightJustified(2,'0') ).arg( QString::number(seconds).rightJustified(2,'0') );
}

char* stringToLowerCase(char* string) {
    for (int i=0 ; string[i]!='\0' ; i++) {
        if (string[i]>=65 && string[i]<=90) {
            string[i] += 32;
        }
    }
    return string;
}
