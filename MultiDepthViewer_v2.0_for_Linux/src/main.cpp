
#include <iostream>
#include <string>

#include "OpenNI.h"
#include "Viewer.h"
#include "COBDevice.h"


using namespace std;


void CheckOpenNIError(openni::Status result, string status, const char* deviceUri)
{
    if (result != openni::Status::STATUS_OK)
    {
        cerr << "Error: " << status << deviceUri << openni::OpenNI::getExtendedError() << endl << endl;
        openni::OpenNI::shutdown();
    }
}


int main(int argc, char** argv)
{
    openni::Status rc = openni::STATUS_OK;

    openni::VideoStream    depthStreams[MAX_DEVICES];
    COBDevice              cob_devices[MAX_DEVICES];
    const char*            deviceUris[MAX_DEVICES];
    char                   serialNumbers[MAX_DEVICES][12]; // Astra serial number has 12 numbers

    rc = openni::OpenNI::initialize();
    CheckOpenNIError(rc, "Initialize failed\n", "");

    cout << "MultiDepthViewer v1.2" << endl << endl;
    cout << "Esc - Quit " << endl << endl;

    // Enumerate devices
    openni::Array<openni::DeviceInfo> deviceList;
    openni::OpenNI::enumerateDevices(&deviceList);
    int devices_count = deviceList.getSize();

    cout << "Devices Count = " << devices_count << std::endl << std::endl;
    cout << "Max Devices = " << MAX_DEVICES << std::endl << std::endl;

    if (devices_count >= MAX_DEVICES)
    {
	    cout << "Support " << MAX_DEVICES << " devices only!" << endl;
	    return 0;
    }

    cout << "Press any key between 1 and " << devices_count << " to switch the laser" << endl;
    cout << endl;

    // Open all depth streams
    for (int i = 0; i < devices_count; ++i)
    {
		// Open device by Uri
		openni::Device* device = new openni::Device;
		deviceUris[i] = deviceList[i].getUri();
			
		rc = device->open(deviceUris[i]);
		CheckOpenNIError(rc, "Couldn't open device : ", deviceUris[i]);

		rc = depthStreams[i].create(*device, openni::SENSOR_DEPTH);
		CheckOpenNIError(rc, "Couldn't create stream on device : ", deviceUris[i]);


	  cob_devices[i].InitDevice();
	  cob_devices[i].OpenDevice(deviceUris[i]);

		// Read serial number
		int data_size = sizeof(serialNumbers[i]);
		device->getProperty((int)ONI_DEVICE_PROPERTY_SERIAL_NUMBER, (void *)serialNumbers[i], &data_size);

		rc = depthStreams[i].start();
		CheckOpenNIError(rc, "Couldn't create stream on device : ", serialNumbers[i]);

		if (!depthStreams[i].isValid())
		{
			printf("SimpleViewer: No valid streams. Exiting\n");
			openni::OpenNI::shutdown();
			return 6;
		}

		cout << "Device " << (i + 1) << endl << "Uri: " << deviceUris[i] << endl << "SN: " << serialNumbers[i] << endl;
		cout << endl;
    }

    cout << endl;

    SampleViewer sampleViewer("MultiDepthViewer", depthStreams, devices_count, cob_devices);

    rc = sampleViewer.init(argc, argv, deviceUris, serialNumbers);
    CheckOpenNIError(rc, "SimpleViewer: Initialization failed", "");
  

    sampleViewer.run();
}
