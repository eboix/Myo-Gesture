# This is a quick hack
You do not want to use this repository. You will not get good results.

# Myo-Gesture
HackPrinceton2015F -- Extends the Myo API to allow for custom gesture detection.

## What it does
Myo Gesture extends the Myo API to provide support for user-defined custom gestures. A developer enters training data for a certain gesture by running the `CollectRaw` executable. He/she then runs the `ProcessRaw` executable to train the model for the gestures inputted.

The `CollectRaw` executable will create `.rawmyo` files in the `.\rawdata` folder, and the `ProcessRaw` executable will train a classifier on those `.rawmyo` files to create a `GestureTrainingDataLabels.txt` file and a `DTWModel.txt` file in the `.\processed` directory. These two filenames are passed to a `GestureDeviceListener` to initialize the correct training model.

Once this simple procedure has been followed to define custom gestures, it is easy to use the API. Simply include `gesturedevicelistener.h`, and let your device listener class extend `GestureDeviceListener`, instead of extending `myo::DeviceListener`. Your device listener class should work just as if it were extending `GestureDeviceListener` except that (1), whenever you call `hub.run(int)` for a Myo Hub `hub`, you should also call `collector.recData()` immediately afterward (where `collector` is your instance of your device listener class), and (2) when you override `GestureDeviceListener`'s methods, override the methods with a 2 at the end of the name (e.g., `void onAccelerometerData2(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& acceleration) { }` instead of `void onAccelerometerData(myo::Myo* myo, uint64_t timestamp, const myo::Vector3<float>& acceleration) { }`.

## Additional Notes
This repo contains the source for Nick Gillian's wonderful GRT library, which should be built with the `collectraw.cpp` and `gesturemyo.h` files. Alternatively, the library may be found on Nick's webpage, at http://www.nickgillian.com/software/grt.

Also, in order to run the files in the Myo-Gesture repository, you need to have Myo Connect installed and the Myo device synced.

