#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

struct tGames {
	char processName[50];
	char fullName[250];
	char multiplayerCommandLineArguments[100];
	char registryName[250];
} record;

void listAllRecords () {
	fstream file;
	file.open("gameslist.dat",ios::in | ios::binary);
	int counter=0;
	while (true) {
		file.read( (char*)&record,sizeof(tGames) );
		if (file.eof()==true)
			break;
		cout << "Game #" << ++counter << endl
			 << "\tProcess Name: " << record.processName << endl
			 << "\tFull Name: " << record.fullName << endl
			 << "\tCommand Line Arguments for joining server: " << record.multiplayerCommandLineArguments << endl
			 << "\tRegistry Name: " << record.registryName << endl;
	}
	file.close();
	file.clear();
}

int binarySearch (fstream &file,char* value,int first,int last) {
	if (first>last) {
		return -1;
	}
	int mid=(first+last)/2;
	file.seekg(mid*sizeof(tGames));
	tGames tmp;
	file.read( (char*)&tmp,sizeof(tGames) );
	if (strcmp(value,tmp.processName)<0)
		return binarySearch(file,value,first,mid-1);
	else if (strcmp(value,tmp.processName)>0)
		return binarySearch(file,value,mid+1,last);
	else
		return mid;
}

int binarySearchWrapper (char* processName) {
	fstream file;
	file.open("gameslist.dat",ios::in | ios::binary);
	file.seekg(0,ios::end);
	int numOfRecords=file.tellg()/sizeof(tGames);
	int result=binarySearch (file,processName,0,numOfRecords-1);
	file.close();
	return result;
}

void addNewRecord () {
	cout << "Enter process name (often same like name of executable file; include its extension): ";
	cin.ignore();
	cin.getline(record.processName,50);
	cout << "Enter full name of that game: ";
	cin.getline(record.fullName,250);
	cout << "Enter command line arguments for directly joining multiplayer of that game" << endl
		 << "set %%ip%% on position where normally is IP address of remote gameserver" << endl
		 << "set %%port%% on position where normally is port of remote gameserver" << endl
		 << ": ";
	cin.getline(record.multiplayerCommandLineArguments,100);
	cout << "Enter registry name that contains path to executable: " << endl;
	cin.getline(record.registryName,250);
	fstream file;
	file.open("gameslist.dat",ios::out | ios::in | ios::binary);
	file.seekg(0,ios::end);
	int numOfRecords=file.tellg()/sizeof(tGames);
	tGames recordIth;
	int i=numOfRecords-1;
	if (i!=-1) {
		file.seekg(i*sizeof(tGames));
		file.read( (char*)&recordIth,sizeof(tGames) );
	}
	for (;i>=0 && strcmp(recordIth.processName,record.processName)>0;i--) {
		file.seekp((i+1)*sizeof(tGames));
		file.write( (char*)&recordIth,sizeof(tGames) );
		if (i!=0) {
			file.seekg((i-1)*sizeof(tGames));
			file.read( (char*)&recordIth,sizeof(tGames) );
		}
	}
	file.seekp((i+1)*sizeof(tGames));
	file.write( (char*)&record,sizeof(tGames) );
	file.close();
}

void efficient_bubble_sort (fstream &file,int N) {	//replacing edited record will take long in worst case scenario
	tGames record1,record2;
	int i,j;
	bool stop=false;
	for (j=0;j<N-1 && stop==false;j++) {
		stop=true;
		for (i=N-2;i>=j;i--) {
			file.seekg(i*sizeof(tGames));
			file.read((char*)&record1,sizeof(tGames));
			file.read((char*)&record2,sizeof(tGames));
			if (strcmp(record1.processName,record2.processName)>0) {
				file.seekp(i*sizeof(tGames));
				file.write((char*)&record2,sizeof(tGames));
				file.write((char*)&record1,sizeof(tGames));
				stop=false;
			}
		}
	}
}

void editExistingRecord () {
	cout << "Enter process name (often same like name of executable file; include its extension): ";
	cin.ignore();
	cin.getline(record.processName,50);
	int currentPosition=binarySearchWrapper(record.processName);
	if (currentPosition==-1) {
		cout << "There is no record with this process name in file." << endl;
		return;
	}
	cout << "Enter new process name: ";
	cin.getline(record.processName,50);
	cout << "Enter new full name of that game: ";
	cin.getline(record.fullName,250);
	cout << "Enter command line arguments for directly joining multiplayer of that game" << endl
		 << "set %%ip%% on position where normally is IP address of remote gameserver" << endl
		 << "set %%port%% on position where normally is port of remote gameserver" << endl
		 << ": ";
	cin.getline(record.multiplayerCommandLineArguments,100);
	cout << "Enter new registry name that contains path to executable: " << endl;
	cin.getline(record.registryName,250);
	fstream file;
	file.open("gameslist.dat",ios::in | ios::out | ios::binary);
	file.seekg(0,ios::end);
	int numOfRecords=file.tellg()/sizeof(tGames);
	file.seekp((currentPosition)*sizeof(tGames));
	file.write( (char*)&record,sizeof(tGames) );
	efficient_bubble_sort(file,numOfRecords);
	file.close();
}

int main () {
	fstream file;
	file.open("gameslist.dat",ios::app | ios::binary);
	file.close();
	int choice;
	do {
		cout<< "Enter number that represent one of the following options:" << endl
			<< "1. List through all records" << endl
			<< "2. Add new record" << endl
			<< "3. Edit existing one" << endl
			<< "9. Exit GameLibrary Manager (this changes will be used as an update in future)" << endl
			<< "Your choice: ";
		cin >> choice;
		switch (choice) {
			case 1:
				listAllRecords();
				break;
			case 2:
				addNewRecord();
				break;
			case 3:
				editExistingRecord();
				break;
		}
		cout<<endl;
	} while (choice!=9);
	return 0;
}