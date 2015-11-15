/*
   A simple example that shows how to use the Myo Gesture API.
   Only works on Windows at this stage.
*/
#define _USE_MATH_DEFINES
#define M_PI 3.1415926535897832
#include <cmath>
#include <conio.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <fstream>
#include <windows.h>
#include <sstream>
#define WINDOWS

using namespace std;

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>
#include "gesturedevicelistener.h"

void GetFilesInDirectory(std::vector<string> &out, const string &directory)
{
#ifdef WINDOWS
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return; /* No files found */

    do {
        const string file_name = file_data.cFileName;
        const string full_file_name = directory + "/" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (file_name[0] == '.')
            continue;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
#else
    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(directory);
    while ((ent = readdir(dir)) != NULL) {
        const string file_name = ent->d_name;
        const string full_file_name = directory + "/" + file_name;

        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    }
    closedir(dir);
#endif
} // GetFilesInDirectory

int main(int argc, char** argv)
{
	// Connect to a Myo.
    myo::Hub hub("com.example.gesturemyo");
    myo::Myo* myo = hub.waitForMyo(10000);
    if (!myo) {
        throw std::runtime_error("Unable to find a Myo!");
    }

	// Add a GestureDeviceListener to the hub. See instructions in Readme if you want to extend GestureDeviceListener.
	// The files passed to the GestureDeviceListener are generated by the ProcessRaw and CollectRaw executables in the bin folder.
	// You can also build ProcessRaw and CollectRaw by building processraw.cpp and collectraw.cpp.
    GestureDeviceListener collector("processed\\GestureTrainingDataLabels.txt", "processed\\DTWModel.txt");
    hub.addListener(&collector);

    while(1) {
		// Query every 10 milliseconds.
        hub.run(10);
        if(!collector.onArm) {
            cout << "!!!!!!!!!!!!!!Please sync the Myo!!!!!!!!!!!!!!!!!!!!!" << endl;
            return 0;
        }
		// Remember to call collector.recData() each time you call hub.run()!
        collector.recData();
		
		// Quit with the Esc key.
        if(GetAsyncKeyState(VK_ESCAPE)) break;
    }
}


