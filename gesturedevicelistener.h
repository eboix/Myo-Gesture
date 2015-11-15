// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.
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

#define NUMPARAM 6

#include <GRT.h>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>
using namespace GRT;

// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class GestureDeviceListener : public myo::DeviceListener {

public:

    /* If you override this class, make sure to override these methods, with the extra "2" after the method name */
    void onAccelerometerData2(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& acceleration) { }
    void onOrientationData2(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat) {}
    void onPose2(myo::Myo* myo, uint64_t timestamp, myo::Pose pose) { }
    void onUnpair2(myo::Myo* myo, uint64_t timestamp) { }
    void onArmSync2(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
                   myo::WarmupState warmupState) { }
    void onArmUnsync2(myo::Myo* myo, uint64_t timestamp) { }
    void onUnlock2(myo::Myo* myo, uint64_t timestamp) { }
    void onLock2(myo::Myo* myo, uint64_t timestamp) { }
    void onConnect2(myo::Myo *myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion) { }
    void onDisconnect2(myo::Myo *myo, uint64_t timestamp) { }
    void onGyroscopeData2(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &gyro) { }
    void onRssi2(myo::Myo *myo, uint64_t timestamp, int8_t rssi) { }
    void onBatteryLevelReceived2(myo::Myo *myo, uint64_t timestamp, uint8_t level) { }
    void onEmgData2(myo::Myo *myo, uint64_t timestamp, const int8_t *emg) { }
    void onWarmupCompleted2(myo::Myo *myo, uint64_t timestamp, myo::WarmupResult warmupResult) { }

    void onConnect(myo::Myo *myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion) {
        onConnect2(myo, timestamp, firmwareVersion);
    }
    void onDisonnect(myo::Myo *myo, uint64_t timestamp) {
        onDisconnect2(myo, timestamp);
    }
    void onGyroscopeData(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &gyro) {
        onGyroscopeData2(myo,timestamp, gyro);
    }
    void onRssi(myo::Myo *myo, uint64_t timestamp, int8_t rssi) {
        onRssi2(myo, timestamp, rssi);
    }
    void onBatteryLevelReceived(myo::Myo *myo, uint64_t timestamp, uint8_t level) {
        onBatteryLevelReceived2(myo, timestamp, level);
    }
    void onEmgData(myo::Myo *myo, uint64_t timestamp, const int8_t *emg) {
        onEmgData2(myo, timestamp, emg);
    }
    void onWarmupCompleted(myo::Myo *myo, uint64_t timestamp, myo::WarmupResult warmupResult) {
        onWarmupCompleted2(myo, timestamp, warmupResult);
    }

    /* Called by recData when a gesture is detected. The lower the value of "confidence", the more likely the gesture was activated. */
    void onGesture(double confidence, string gesturename) {
        cout << confidence << " " << gesturename << endl;
    }

    // These values are set by onArmSync() and onArmUnsync().
    bool onArm;
    myo::Arm whichArm;

    // This is set by onUnlocked() and onLocked().
    bool isUnlocked;

    // These values are set by onOrientationData().
    int roll_w, pitch_w, yaw_w;
    float roll, pitch, yaw;

    // These values are set by onAccelerometerData().
    float ax, ay, az;

    // These are the buffers storing the past maxbufsize acceleration and orientation values.
    int maxbufsize;
    vector<vector<double> > data;

    // This pose data is set by onOrientation(). It can only take on the values allowed by the standard Myo connect API.
    myo::Pose currentPose;

    double gx, gy, gz; int totsync = 100;
    int sync;

    //Create a new DTW instance, using the default parameters.
    DTW dtw;

    vector<string> gesturenames;

    // Our constructor.
    GestureDeviceListener(string datalabelsfile, string datamodelfile) : onArm(false), isUnlocked(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
    {
        data = vector<vector<double> >(0, vector<double>(0, 0));
        roll = 0; pitch = 0; yaw = 0;
        ax = ay = az = 0;
        maxbufsize = 100;
        gx = gy = gz = 0;
        sync = totsync;

        gesturenames.push_back("null");

        ifstream classNames(datalabelsfile);
        int i; string s;
        while(classNames >> i) {
            classNames >> s;
            gesturenames.push_back(s);
        }
       // cout << gesturenames.size() << endl;


     /*   //Load some training data to train the classifier - the DTW uses LabelledTimeSeriesClassificationData
        LabelledTimeSeriesClassificationData trainingData;

        if( !trainingData.loadDatasetFromFile("processed\\TrainingData.grt") ){
            cerr << "Failed to load training data!\n";
            exit(EXIT_FAILURE);
        } */

        //Trim the training data for any sections of non-movement at the start or end of the recordings
        dtw.enableTrimTrainingData(true,0.1,90);
//        dtw.enableNullRejection(true);
        //Train the classifier

        //Load the DTW model from a file
        if( !dtw.loadModelFromFile(datamodelfile) ){
            cerr << "Failed to load the classifier model!\n";
            exit(EXIT_FAILURE);
        }
        cerr << "Device listener constructed!" << endl;

    }



    // Append current data to stored vectors. Should be called each time that hub.run is called.
    void recData() {
  //      if(sync) {
           // sync--;
  //          gx += ax;
   //         gy += ay;
    //        gz += az;
     //       if(!sync) {
      //          gx /= totsync;
       //         gy /= totsync;
        //        gz /= totsync;
         //   }
          //  return;
       // }

        vector<double> tempaddition;
        tempaddition.push_back(ax);
        tempaddition.push_back(ay);
        tempaddition.push_back(az);
        tempaddition.push_back(roll);
        tempaddition.push_back(pitch);
        tempaddition.push_back(yaw);
        data.push_back(tempaddition);
    //    cerr << ax << " " << ay << " " << az << endl;

        int buffersize = 100;
        if(data.size() > buffersize+1) {
            MatrixDouble window;
            for(int i = 0; i < buffersize; i++) {
             //   cerr << "ABOUT TO MAKE THE PREDICTION! " << i << endl;
                VectorDouble currv;
                for(int j = 0; j < 3; j++) {
                    currv.push_back(data[data.size()-buffersize+i][j]);
                }
                window.push_back(currv);
            }
		// Perform a prediction using the classifier
            if( !dtw.predict(window) ){
                cerr << "Failed to perform prediction!" << endl;
                exit(EXIT_FAILURE);
            }

            //Get the predicted class label
            UINT predictedClassLabel = dtw.getPredictedClassLabel();
            double maximumLikelihood = dtw.getMaximumLikelihood();
           // cerr << predictedClassLabel << endl;
          //  VectorDouble classLikelihoods = dtw.getClassLikelihoods();
          //  VectorDouble classDistances = dtw.getClassDistances();
            if(predictedClassLabel)
                onGesture(maximumLikelihood, gesturenames[predictedClassLabel]);// "\tMaximumLikelihood: " << maximumLikelihood << endl;
        }

    }

    // Clear current data vectors.
    void clearData() {
        for(int i = 0; i < NUMPARAM; i++) {
            data[i].clear();
        }
    }
  /*  void clearGestureKernels() {
        gesturekernels.clear();
        gesturenames.clear();
    } */

    // onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
    void onUnpair(myo::Myo* myo, uint64_t timestamp)
    {
        // We've lost a Myo.
        // Let's clean up some leftover state.
        roll_w = 0;
        pitch_w = 0;
        yaw_w = 0;
        ax = ay = az = 0;
        onArm = false;
        isUnlocked = false;
    }

    // onAccelerometerData() is called whenever the Myo device provides its current acceleration, which is represented
    // as a 3-vector.
    void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& acceleration) {
        ax = acceleration[0];
        ay = acceleration[1];
        az = acceleration[2];
        onAccelerometerData2(myo, timestamp, acceleration);
    }

    // onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
    // as a unit quaternion.
    void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
    {
        // Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
        roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
                           1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
        pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
        yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
                        1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));
       // cerr << "Got orientation" << endl;
     //  cout << roll << " " << pitch << " " << yaw << endl;
        // Convert the floating point angles in radians to a scale from 0 to 18.
        roll_w = static_cast<int>((roll + (float)M_PI)/(M_PI * 2.0f) * 18);
        pitch_w = static_cast<int>((pitch + (float)M_PI/2.0f)/M_PI * 18);
        yaw_w = static_cast<int>((yaw + (float)M_PI)/(M_PI * 2.0f) * 18);
        onOrientationData2(myo, timestamp, quat);
    }

    // onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
    // making a fist, or not making a fist anymore.
    void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
    {
        currentPose = pose;

        if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
            // Tell the Myo to stay unlocked until told otherwise. We do that here so you can hold the poses without the
            // Myo becoming locked.
            myo->unlock(myo::Myo::unlockHold);

            // Notify the Myo that the pose has resulted in an action, in this case changing
            // the text on the screen. The Myo will vibrate.
            myo->notifyUserAction();
        } else {
            // Tell the Myo to stay unlocked only for a short period. This allows the Myo to stay unlocked while poses
            // are being performed, but lock after inactivity.
            myo->unlock(myo::Myo::unlockTimed);
        }
        onPose2(myo, timestamp, pose);
    }

    // onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
    // arm. This lets gestures[i].substring(0,gestures[i].find_first_of('_')Myo know which arm it's on and which way it's facing.
    void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
                   myo::WarmupState warmupState)
    {
        onArm = true;
        whichArm = arm;
        onArmSync2(myo, timestamp, arm, xDirection, rotation, warmupState);
    }

    // onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
    // it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
    // when Myo is moved around on the arm.
    void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
    {
        onArm = false;
        onArmUnsync2(myo, timestamp);
    }

    // onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
    void onUnlock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = true;
        onUnlock2(myo, timestamp);
    }

    // onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
    void onLock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = false;
        onLock2(myo, timestamp);
    }

private:
    // We define this function to print the current values that were updated by the on...() functions above.
    void print()
    {
        // Clear the current line
        std::cout << '\r';

        // Print out the orientation. Orientation data is always available, even if no arm is currently recognized.
        std::cout << '[' << std::string(roll_w, '*') << std::string(18 - roll_w, ' ') << ']'
                  << '[' << std::string(pitch_w, '*') << std::string(18 - pitch_w, ' ') << ']'
                  << '[' << std::string(yaw_w, '*') << std::string(18 - yaw_w, ' ') << ']';

        if (onArm) {
            // Print out the lock state, the currently recognized pose, and which arm Myo is being worn on.

            // Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
            // output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
            // that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
            std::string poseString = currentPose.toString();

            std::cout << '[' << (isUnlocked ? "unlocked" : "locked  ") << ']'
                      << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
                      << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
        } else {
            // Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
            std::cout << '[' << std::string(8, ' ') << ']' << "[?]" << '[' << std::string(14, ' ') << ']';
        }

        std::cout << std::flush;
    }

    void check_gesture() {

    }

};

// A past attempt at a solution:
/*
    void addGestureKernel(string filename) {
        ifstream fin(filename);
        int n; fin >> n;
        gesturenames.push_back(filename.substr(15,filename.length()-8));
        gesturekernels.push_back(vector<vector<double > >());
        for(int i = 0; i < n; i++) {
            vector<double> a;
            for(int j = 0; j < NUMPARAM; j++) {
                double t; fin >> t;
                a.push_back(t);
            }
            gesturekernels[gesturekernels.size()-1].push_back(a);
        }
        fin.close();
    }

    double weight[NUMPARAM] = {1.0, 1.0, 1.0, 0.0, 0.0, 0.0};

    double convolve(vector<vector<double > >& vec1, vector<vector<double > >& vec2) {
        double score = 0;
        if(vec1.size() < vec2.size()) {
            vector<vector<double > > temp = vec1;
            vec1 = vec2;
            vec2 = temp;
        }
      //  cout << vec1.size() << " " << vec2.size() << endl;
        // Acceleration vectors:
        double n1 = 0.01, n2 = 0.01;
        for(int k = 0; k < NUMPARAM; k++) {

            for(size_t j = 0; j < vec2.size(); j++) {

                double v1 = vec1[vec1.size()-vec2.size()+j][k];
                double v2 = vec2[j][k];

                score += weight[k] * v1*v2;

                n1 += weight[k] * v1*v1;
                n2 += weight[k] * v2*v2;
            }

        }
        score /= n1; score /= n2; // score *= 100;
        return score;
    } */
