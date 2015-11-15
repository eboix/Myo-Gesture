
/*
 GRT DTW Example
 This examples demonstrates how to initialize, train, and use the DTW algorithm for classification.

 The Dynamic Time Warping (DTW) algorithm is a powerful classifier that works very well for recognizing temporal gestures.

 In this example we create an instance of an DTW algorithm and then train the algorithm using some pre-recorded training data.
 The trained DTW algorithm is then used to predict the class label of some test data.

 This example shows you how to:
 - Create an initialize the DTW algorithm
 - Load some LabelledTimeSeriesClassificationData from a file and partition the training data into a training dataset and a test dataset
 - Trim any periods of non-movement from the start and end of each timeseries recording
 - Train the DTW algorithm using the training dataset
 - Test the DTW algorithm using the test dataset
 - Manually compute the accuracy of the classifier
*/

//You might need to set the specific path of the GRT header relative to your project
#include "GRT\\GRT.h"
using namespace GRT;

int main (int argc, const char * argv[])
{
    //Create a new DTW instance, using the default parameters
    DTW dtw;

    //Load some training data to train the classifier - the DTW uses LabelledTimeSeriesClassificationData
    LabelledTimeSeriesClassificationData trainingData;

    if( !trainingData.loadDatasetFromFile("TrainingData.grt") ){
        cout << "Failed to load training data!\n";
        return EXIT_FAILURE;
    }

    //Use 20% of the training dataset to create a test dataset
    LabelledTimeSeriesClassificationData testData = trainingData.partition( 80 );

    //Trim the training data for any sections of non-movement at the start or end of the recordings
    dtw.enableTrimTrainingData(true,0.1,90);
    dtw.enableNullRejection(true);
    //Train the classifier
    if( !dtw.train( trainingData ) ){
        cout << "Failed to train classifier!\n";
        return EXIT_FAILURE;
    }

    //Save the DTW model to a file
    if( !dtw.saveModelToFile("DTWModel.txt") ){
        cout << "Failed to save the classifier model!\n";
        return EXIT_FAILURE;
    }

    //Load the DTW model from a file
    if( !dtw.loadModelFromFile("DTWModel.txt") ){
        cout << "Failed to load the classifier model!\n";
        return EXIT_FAILURE;
    }

    //Use the test dataset to test the DTW model
    double accuracy = 0;
    for(UINT i=0; i<testData.getNumSamples(); i++){
        //Get the i'th test sample - this is a timeseries
        UINT classLabel = testData[i].getClassLabel();
        MatrixDouble timeseries = testData[i].getData();

        //Perform a prediction using the classifier
        if( !dtw.predict( timeseries ) ){
            cout << "Failed to perform prediction for test sample: " << i <<"\n";
            return EXIT_FAILURE;
        }

        //Get the predicted class label
        UINT predictedClassLabel = dtw.getPredictedClassLabel();
        double maximumLikelihood = dtw.getMaximumLikelihood();
        VectorDouble classLikelihoods = dtw.getClassLikelihoods();
        VectorDouble classDistances = dtw.getClassDistances();

        //Update the accuracy
        if( classLabel == predictedClassLabel ) accuracy++;

        cout << "TestSample: " << i <<  "\tClassLabel: " << classLabel << "\tPredictedClassLabel: " << predictedClassLabel << "\tMaximumLikelihood: " << maximumLikelihood << endl;
    }

    cout << "Test Accuracy: " << accuracy/double(testData.getNumSamples())*100.0 << "%" << endl;

    return EXIT_SUCCESS;
}

