/*
 GRT MIT License
 Copyright (c) <2012> <Nicholas Gillian, Media Lab, MIT>

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

 /* ^
    |
    |
    I'm using part of someone's machine-learning library.

*/
#include "GRT.h"
#include <windows.h>
#include <map>
#define WINDOWS
#define NUMPARAM 6
using namespace GRT;

/* Returns a list of files in a directory (except the ones that begin with a dot) */
/* Taken from http://stackoverflow.com/questions/306533/how-do-i-get-a-list-of-files-in-a-directory-in-c. */
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

vector<vector<vector<double > > > data;

// MAYBE I SHOULD PREPROCESS YAW,PITCH, and ROLL --> time derivatives of the Euler angles.

int main() {
    vector<string> gestures(0,"");
    GetFilesInDirectory(gestures, "rawdata");
    CreateDirectory("gesturekernels", NULL);
    sort(gestures.begin(), gestures.end());
    data = vector<vector<vector<double > > >(gestures.size(), vector<vector<double > >(0,vector<double>(0,0)));
    for(size_t i = 0; i < gestures.size(); i++) {
        ifstream fin(gestures[i]);
        int n; fin >> n;
       // cerr << gestures[i] << endl;
       // cerr << n << endl;
        data[i] = vector<vector<double> >(n, vector<double>(NUMPARAM, 0));
        for(int j = 0; j < n; j++) {
            for(int k = 0; k < NUMPARAM; k++) {
                fin >> data[i][j][k];
            }
        }
        fin.close();
    }


    //Create a new instance of the TimeSeriesClassificationDataStream
    TimeSeriesClassificationData trainingData;

    // ax, ay, az
    trainingData.setNumDimensions(3);
    trainingData.setDatasetName("GestureTrainingData.txt");
    ofstream labelfile("GestureTrainingDataLabels.txt");
    UINT currLabel = 1;
    Random random;
    map<string, int> gesturenames;
    for(size_t overall = 0; overall < gestures.size(); overall++) {

        string nam = gestures[overall].substr(8,gestures[overall].find_first_of('_')-8);
        if(gesturenames.count(nam)) currLabel = gesturenames[nam];
        else {
            currLabel = gesturenames.size()+1;
            gesturenames[nam] = currLabel;
            labelfile << currLabel << " " << nam << endl;
        }
        MatrixDouble trainingSample;
        VectorDouble currVec( trainingData.getNumDimensions() );
        for(size_t k = 0; k < data[overall].size(); k++) {
            for(UINT j=0; j<currVec.size(); j++){
                currVec[j] = data[overall][k][j];
            }
            trainingSample.push_back(currVec);
        }
        trainingData.addSample(currLabel, trainingSample);

    }

    //After recording your training data you can then save it to a file
    if( !trainingData.save( "TrainingData.grt" ) ){
        cout << "ERROR: Failed to save dataset to file!\n";
        return EXIT_FAILURE;
    }

    //This can then be loaded later
    if( !trainingData.load( "TrainingData.grt" ) ){
        cout << "ERROR: Failed to load dataset from file!\n";
        return EXIT_FAILURE;
    }

    //This is how you can get some stats from the training data
    string datasetName = trainingData.getDatasetName();
    string infoText = trainingData.getInfoText();
    UINT numSamples = trainingData.getNumSamples();
    UINT numDimensions = trainingData.getNumDimensions();
    UINT numClasses = trainingData.getNumClasses();

    cout << "Dataset Name: " << datasetName << endl;
    cout << "InfoText: " << infoText << endl;
    cout << "NumberOfSamples: " << numSamples << endl;
    cout << "NumberOfDimensions: " << numDimensions << endl;
    cout << "NumberOfClasses: " << numClasses << endl;

    //You can also get the minimum and maximum ranges of the data
    vector< MinMax > ranges = trainingData.getRanges();

    cout << "The ranges of the dataset are: \n";
    for(UINT j=0; j<ranges.size(); j++){
        cout << "Dimension: " << j << " Min: " << ranges[j].minValue << " Max: " << ranges[j].maxValue << endl;
    }

    //If need you can clear any training data that you have recorded
    trainingData.clear();

    return EXIT_SUCCESS;
}

