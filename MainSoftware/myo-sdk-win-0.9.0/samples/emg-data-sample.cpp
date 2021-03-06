// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.

// This sample illustrates how to use EMG data. EMG streaming is only supported for one Myo at a time.

#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <list>
#include <cmath>

#include <myo/myo.hpp>

const int avgMarkCount = 5;
const int PI = 3.14159265;
const int updateTime = 50;

class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: emgSamples()
	{
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void print(int &lineCounter, std::ofstream &myfile)
	{
		lineCounter++;
		// Clear the current line
		std::cout << '\n';
		myfile << lineCounter << ' ';

		// Print out the EMG data.
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			std::cout << emgString << std::string(4 - emgString.size(), ' ');
			myfile << emgString << std::string(4 - emgString.size(), ' ');
		}

		std::cout << " " << finger[0] << " " << finger[1] << " " << finger[2] << " " << finger[3] << " " << finger[4];
		myfile << " " << finger[0] << " " << finger[1] << " " << finger[2] << " " << finger[3] << " " << finger[4];
		myfile << '\n';

		std::cout << std::flush;
	}

	int averageHandler() {
		int podSum = 0;

		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			if (podHistory[i].size() == 20) {
				podHistory[i].pop_back();
				podHistory[i].push_front(abs(emgSamples[i]));
			}
			else {
				podHistory[i].push_front(abs(emgSamples[i]));
			}
			sum[i] += abs(emgSamples[i]);
			counter[i]++;
		}
		std::cout << '\n';

		//average from most recent 8
		for (int pod = 0; pod < 8; pod++) {
			podSum = 0;
			std::list<int>::iterator it = podHistory[pod].begin();
			for (int z = 0; z < 8; z++) {
				if (it == podHistory[pod].end()) {
					break;
				}
				else {
					podSum += *it;
					it++;
				}
			}

			float avgAll = (float)sum[pod] / (float)counter[pod];
			float podAvg = (float)podSum / 8.0;
			avgAllArr[pod] = avgAll;

			// //printed for checking info
			// std::cout << "podAvg for " << pod << ": " << podAvg << std::endl;

			if (podAvg < 5.0 && avgAll > 5.0) {
				sum[pod] = 0;
				counter[pod] = 0;
				std::cout << "pod " << pod << " RESET\n";
			}
		}
		for (int pod = 0; pod < 8; pod++) {
			if (((float)sum[pod] / (float)counter[pod]) > 5.0) { //avgAll > 5.0 then its leaving the neighborhood
				return 1; //return 1 to set off fourier info
			}
		}

		// //printed for checking info
		// for(int pod = 0; pod < 8; pod++){
		// float avgAll = (float)sum[pod] / (float)counter[pod];
		// std::cout << "avgAll for " << pod << ": " << avgAll << std::endl;
		// }
		return 0;
	}

	int stateHandler(int &state, int &flag) {
		switch (state) {
		case 0: //rest 
			for (int pod = 0; pod < 8; pod++) {
				if (((float)sum[pod] / (float)counter[pod]) < 5.0) { //avgAll
					state = 0;
				}
				else {
					flag = 10;
					state = 1;
				}
			}
			break;

		case 1:   //transition                 
			if (flag > 0) {
				flag--;
			}
			else {
				//average from most recent 10
				for (int pod = 0; pod < 8; pod++) {
					int podSum = 0;
					std::list<int>::iterator it = podHistory[pod].begin();
					std::cout << "POD " << pod << ": ";
					for (int z = 0; z < 10; z++) {
						podSum += *it;
						it++;
					}
					float podAvg = (float)podSum / 10.0;
					std::cout << "podAvg for " << pod << ": " << podAvg << std::endl;
					recentTen[pod] = podAvg;
					if (recentTen[0] > 10.0 && recentTen[3] > 10.0 && recentTen[5] > 10.0 && recentTen[6] > 10.0 && recentTen[7] > 10.0) {
						state = 2;
						std::cout << "state: index state" << std::endl; //del after test
						flag = 10;
					}
				}
			}
			break;

		case 2:   //index
			finger[1] = 180;
			float arr[8];
			if (flag > 0) {
				flag--;
			}
			else {
				//average from most recent 10
				for (int pod = 0; pod < 8; pod++) {
					int podSum = 0;
					std::list<int>::iterator it = podHistory[pod].begin();
					std::cout << "POD " << pod << ": ";
					for (int z = 0; z < 10; z++) {
						podSum += *it;
						it++;
					}
					float podAvg = (float)podSum / 10.0;
					std::cout << "podAvg for " << pod << ": " << podAvg << std::endl;
					arr[pod] = podAvg;
				}
			}

			// NOTE TO SELF: SAVE THE PAST 10 READS. USE BELOW IF-ELSE TO SEE IF OK. IF OK, YOU OK?

			if (avgAllArr[0] >= 5.0 && avgAllArr[1] < 5.0 && avgAllArr[2] < 5.0 && avgAllArr[3] >= 5.0 && avgAllArr[4] < 5.0 && avgAllArr[5] >= 5.0 && avgAllArr[6] >= 5.0 && avgAllArr[7] >= 5.0) { //avgAll
				finger[1] = 180;
				state = 2;
				std::cout << "finger[1]: " << finger[1] << std::endl; //del after test
				std::cout << "state: index state" << std::endl; //del after test
				break;
			}
			else {
				finger[1] = 0;
				state = 0;
				std::cout << "finger[1]: " << finger[1] << std::endl; //del after test
				std::cout << "state: rest state" << std::endl; //del after test
			}
			std::string dog;
			std::cin >> dog;
			break;
		}

		return state;
	}

	void fourierDataFill(int q) {
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			std::cout << emgString << std::string(4 - emgString.size(), ' ');
			std::cout << "i am putting " << emgString << "in [" << i << "][" << q << "]" << std::endl;
			fourierData[i][q] = std::stoi(emgString);
		}
	}

	void fourier() {
		// std::ofstream myfile("sample.txt");
		// std::ofstream myfile("sample-transformed.txt");
		float f = 0;
		float time = 0;
		int sampleSize = 100;

		for (int a = 0; a < 8)
			for (int z = 0; z < 100; z++) {
				int An = fourierData[0][];
				result = cos((-2 * PI * time * f) / sampleSize);
				std::cout << An << "cos(" << "-2pi * " << f << " * " << time << "over N )" << endl;
				transformation = An * result;
			}

		time += 0.5;
		f += 0.1;
	}

	void spitter() {
		std::cout << "\n\n\ndata begin: \n\n\n";
		for (int row = 0; row < 8; row++) {
			for (int i = 0; i < 100; i++) {
				std::cout << "[" << row << "][" << i << "]: " << fourierData[row][i] << " ";
			}
			std::cout << std::endl << std::endl;
		}
	}

	float fourierArray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	float avgAllArr[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

	// The values of this array is set by onEmgData() above.
	std::array<int8_t, 8> emgSamples;

	//sum of entire history
	int sum[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	//counter for each reading for each pod; used for average
	int counter[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	int recentTen[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	std::list<int> podHistory[8];

	int finger[5] = { 0, 0, 0, 0, 0 };

	int fourierData[8][100];
};




int main(int argc, char** argv)
{
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {
		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.example.emg-data-sample");

		std::cout << "Attempting to find a Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		myo::Myo* myo = hub.waitForMyo(10000);

		// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}

		// We've found a Myo.
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

		// Next we enable EMG streaming on the found Myo.
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);

		//file stream to write into file
		std::ofstream myfile("a.txt");

		//number the lines to plot it
		int lineCounter = 0;
		int state = 0;
		int flag = 10;
		int foo = 0;

		// Finally we enter our main loop.
		for (int o = 1; o < 100; o++) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
			hub.run(updateTime);
			// After processing events, we call the print() member function we defined above to print out the values we've
			// obtained from any events that have occurred.

			collector.print(lineCounter, myfile);
			foo = collector.averageHandler();
			if (foo == 1) {
				int q = 0;
				for (int x = 0; x < 100; x++) {
					hub.run(updateTime);
					collector.fourierDataFill(q);
					q++;
				}
				break;
			}
			// collector.stateHandler(state, flag);          	
		}

		collector.spitter();

		std::cin.ignore();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}