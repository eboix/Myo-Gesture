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

using namespace std;

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>

inline bool file_exists (string name) {


    ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }
}
template <typename T>
  string NumberToString ( T Number )
  {
     ostringstream ss;
     ss << Number;
     return ss.str();
  }
// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class DataCollector : public myo::DeviceListener {

public:
    DataCollector()
    : onArm(false), isUnlocked(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
    {
        accel = vector<vector<double> >(3, vector<double>(0, 0));
        orient = vector<vector<double> >(3, vector<double>(0, 0));
        roll = 0; pitch = 0; yaw = 0;
        ax = ay = az = 0;
    }

    // onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
    void onUnpair(myo::Myo* myo, uint64_t timestamp)
    {
        // We've lost a Myo.
        // Let's clean up some leftover state.
        roll_w = 0;
        pitch_w = 0;
        yaw_w = 0;
        onArm = false;
        isUnlocked = false;
    }

    // onAccelerometerData() is called whenever the Myo device provides its current acceleration, which is represented
    // as a 3-vector.
    void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& acceleration) {
        ax = acceleration[0];
        ay = acceleration[1];
        az = acceleration[2];
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
    }

    // onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
    // arm. This lets gestures[i].substring(0,gestures[i].find_first_of('_')Myo know which arm it's on and which way it's facing.
    void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
                   myo::WarmupState warmupState)
    {
        onArm = true;
        whichArm = arm;
    }

    // onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
    // it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
    // when Myo is moved around on the arm.
    void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
    {
        onArm = false;
    }

    // onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
    void onUnlock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = true;
    }

    // onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
    void onLock(myo::Myo* myo, uint64_t timestamp)
    {
        isUnlocked = false;
    }

    // There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
    // For this example, the functions overridden above are sufficient.

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

    // Append current data to stored vectors.
    void recData() {
        accel[0].push_back(ax);
        accel[1].push_back(ay);
        accel[2].push_back(az);
        orient[0].push_back(roll);
        orient[1].push_back(pitch);
        orient[2].push_back(yaw);
      //  cout << ax << endl;
    }

    // Clear current data vectors.
    void clearData() {
        for(int i = 0; i < 3; i++) {
            accel[i].clear();
            orient[i].clear();
        }
    }

    // Appends raw recorded data to a file whose name is given by filename.
    void appendData(string filename) {
        std::ofstream outfile(filename);
        cerr << "Opened file" << endl;
        outfile << endl;
        outfile << accel[0].size() << endl;
       // cerr << accel[0].size() << endl;
        for(size_t i = 0; i < accel[0].size(); i++) {
            for(int j = 0; j < 3; j++) {
                outfile << accel[j][i] << " ";
            }
            for(int j = 0; j < 3; j++) {
                outfile << orient[j][i] << " ";
            }
            outfile << endl;
        }
        cerr << "Done writing" << endl;
        outfile.close();
        cerr << "Closing outfile" << endl;
    }

    // These values are set by onArmSync() and onArmUnsync() above.
    bool onArm;
    myo::Arm whichArm;

    // This is set by onUnlocked() and onLocked() above.
    bool isUnlocked;

    // These values are set by onOrientationData() and onPose() above.
    int roll_w, pitch_w, yaw_w;
    float roll, pitch, yaw, ax, ay, az;
    vector<vector<double> > accel;
    vector<vector<double> > orient;
    myo::Pose currentPose;
};

// Appends raw gesture data to a file whose name is given by filename.
void recordGesture(string gesturename, int t) {

        // First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
    // publishing your application. The Hub provides access to one or more Myos.
    myo::Hub hub("com.example.gesturemyo");
    myo::Myo* myo = hub.waitForMyo(10000);
    if (!myo) {
        throw std::runtime_error("Unable to find a Myo!");
    }

    DataCollector collector;
    hub.addListener(&collector);
    int fno = 0;

    while(t--) {
        // Finally we enter our main loop.
        while(file_exists("rawdata\\" + gesturename + "_" + NumberToString(fno) + ".rawmyo")) fno++;
        string filename = "rawdata\\" + gesturename + "_" + NumberToString(fno) + ".rawmyo";
        std::cout << "Press s when ready to start recording." << endl;
        while('s' != getch()) {

        }
        std::cout << "Recording gesture. Press Esc to end recording." << std::endl;
    //    int calibrated = 0;
        while(1) {
            // In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
            // In this case, we wish to update our display 20 times a second, so we run for 1000/20 milliseconds.
            hub.run(10);
       //     if(!calibrated) {
      //          collector.calibrate();
      //          calibrated = 1;
     //       }
            // After processing events, we call the print() member function we defined above to print out the values we've
            // obtained from any events that have occurred.
            if(!collector.onArm) {
                cout << "!!!!!!!!!!!!!!Please sync the Myo!!!!!!!!!!!!!!!!!!!!!" << endl;
                return;
            }
            collector.print();
            collector.recData();
            if(GetAsyncKeyState(VK_ESCAPE)) break;
        }
        std::cout << "Use recording? (y/n)" << std::endl;
        char ans;
        while((ans = getch()) != 'n' && ans != 'y');

        // If the recording was good, append it into the file. This is just the data collection stage -- processing of the files generated will take place later.
        if(ans == 'y') {
            collector.appendData(filename);
            cout << "Written as " << filename << endl;
        }
        collector.clearData();


    }
}

int main(int argc, char** argv)
{
    CreateDirectory("rawdata", NULL);

    if(argc > 1) {
        cout << "Welcome to the data collector for MyoTrainer!" << endl;
        cout << "With this module, you can add raw data gesture files to your gesture library." << endl;
        cout << "After adding these files, you should process them with processmyo.exe" << endl;
    }

    // We catch any exceptions that might occur below -- see the catch statement for more details.
    try {

    while(1) {
        std::cout << "Enter name of gesture: ";
        string gesturename; cin >> gesturename;
        if(gesturename.size() == 0) return 0;
        std::cout << "And number of times you'll perform it: ";
        int t = 0; cin >> t;
        recordGesture(gesturename, t);
    }



    // If a standard exception occurred, we print out its message and exit.
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Press enter to continue.";
        std::cin.ignore();
        return 1;
    }
}


