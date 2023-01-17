#include "stdafx.h"
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <vector>
#include <string>
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>
#include "UVCExtensionApp.h"
#include "LibUVCExt.h"


//23E49ED0 - 1178 - 4F31 - AE52 - D2FB8A8D3B48
static const GUID xuGuid = { 0x23E49ED0, 0x1178, 0x4F31, { 0xAE, 0x52, 0xD2, 0xFB, 0x8A, 0x8D, 0x3B, 0x48 } };

//
IMFMediaSource *pVideoSource = NULL;
IMFAttributes *pVideoConfig = NULL;
IMFActivate **ppVideoDevices = NULL;
IMFSourceReader *pVideoReader = NULL;
//
UINT32 noOfVideoDevices = 0;
WCHAR *szFriendlyName = NULL;

int main()
{
	HRESULT hr = S_OK;
	CHAR enteredStr[MAX_PATH], videoDevName[20][MAX_PATH];
	UINT32 selectedVal = 0xFFFFFFFF;
	ULONG flags, readCount;
	size_t returnValue;
	BYTE an75779FwVer[8] = { 0x80, 0x02, 0x07, 0x00, 0x00 , 0x00 , 0x00 , 0x00 };

	//Get all video devices
	GetVideoDevices();

	for (UINT32 i = 0; i < noOfVideoDevices; i++)
	{
		//Get the device names
		GetVideoDeviceFriendlyNames(i);
		wcstombs_s(&returnValue, videoDevName[i], MAX_PATH, szFriendlyName, MAX_PATH);
		printf("%d: %s\n", i, videoDevName[i]);

		//Store the App note firmware (Whose name is *FX3*) device index  
		if (!(strcmp(videoDevName[i], "Pixio StreamCube")))
			selectedVal = i;
	}
	selectedVal = 1;

	//Specific to UVC extension unit in AN75779 appnote firmware
	if (selectedVal != 0xFFFFFFFF)
	{
		printf("\nFound \"FX3\" device\n");

		//Initialize the selected device
		InitVideoDevice(selectedVal);

		/*
		printf("\nSelect\n1. To Set Firmware Version \n2. To Get Firmware version\nAnd press Enter\n");
		fgets(enteredStr, MAX_PATH, stdin);
		fflush(stdin);
		selectedVal = atoi(enteredStr);
		*/
		selectedVal = 1;

		if (selectedVal == 1)
			flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
		else
			flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

		printf("\nTrying to invoke UVC extension unit...\n");

		if (!SetGetExtensionUnit(xuGuid, 2, 1, flags, (void*)an75779FwVer, 8, &readCount))
		{
			printf("Found UVC extension unit\n");

			if (flags == (KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY))
			{
				printf("\nAN75779 Firmware Version Set to %d.%d (Major.Minor), Build Date: %d/%d/%d (MM/DD/YY)\n\n", an75779FwVer[0], an75779FwVer[1],
					an75779FwVer[2], an75779FwVer[3], an75779FwVer[4]);
			}
			else
			{
				printf("\nCurrent AN75779 Firmware Version is %d.%d (Major.Minor), Build Date: %d/%d/%d (MM/DD/YY)\n\n", an75779FwVer[0], an75779FwVer[1],
					an75779FwVer[2], an75779FwVer[3], an75779FwVer[4]);
			}
		}
	}
	else
	{
		printf("\nDid not find \"FX3\" device (AN75779 firmware)\n\n");
	}

	//Release all the video device resources
	for (UINT32 i = 0; i < noOfVideoDevices; i++)
	{
		SafeRelease(&ppVideoDevices[i]);
	}
	CoTaskMemFree(ppVideoDevices);
	SafeRelease(&pVideoConfig);
	SafeRelease(&pVideoSource);
	CoTaskMemFree(szFriendlyName);

	printf("\nExiting App in 5 sec...");
	Sleep(5000);

    return 0;
}

//Function to get UVC video devices
HRESULT GetVideoDevices()
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	MFStartup(MF_VERSION);

	// Create an attribute store to specify the enumeration parameters.
	HRESULT hr = MFCreateAttributes(&pVideoConfig, 1);
	CHECK_HR_RESULT(hr, "Create attribute store");

	// Source type: video capture devices
	hr = pVideoConfig->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);
	CHECK_HR_RESULT(hr, "Video capture device SetGUID");

	// Enumerate devices.
	hr = MFEnumDeviceSources(pVideoConfig, &ppVideoDevices, &noOfVideoDevices);
	CHECK_HR_RESULT(hr, "Device enumeration");

done:
	return hr;
}

//Function to get device friendly name
HRESULT GetVideoDeviceFriendlyNames(int deviceIndex)
{
	// Get the the device friendly name.
	UINT32 cchName;

	HRESULT hr = ppVideoDevices[deviceIndex]->GetAllocatedString(
		MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
		&szFriendlyName, &cchName);
	CHECK_HR_RESULT(hr, "Get video device friendly name");

done:
	return hr;
}

//Function to initialize video device
HRESULT InitVideoDevice(int deviceIndex)
{
	HRESULT hr = ppVideoDevices[deviceIndex]->ActivateObject(IID_PPV_ARGS(&pVideoSource));
	CHECK_HR_RESULT(hr, "Activating video device");

	// Create a source reader.
	hr = MFCreateSourceReaderFromMediaSource(pVideoSource, pVideoConfig, &pVideoReader);
	CHECK_HR_RESULT(hr, "Creating video source reader");

done:
	return hr;
}

//Function to set/get parameters of UVC extension unit
HRESULT SetGetExtensionUnit(GUID xuGuid, DWORD dwExtensionNode, ULONG xuPropertyId, ULONG flags, void *data, int len, ULONG *readCount)
{
	GUID pNodeType;
	IUnknown *unKnown;
	IKsControl * ks_control = NULL;
	IKsTopologyInfo * pKsTopologyInfo = NULL;
	KSP_NODE kspNode;

	HRESULT hr = pVideoSource->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&pKsTopologyInfo);
	CHECK_HR_RESULT(hr, "IMFMediaSource::QueryInterface(IKsTopologyInfo)");

	hr = pKsTopologyInfo->get_NodeType(dwExtensionNode, &pNodeType);
	CHECK_HR_RESULT(hr, "IKsTopologyInfo->get_NodeType(...)");

	hr = pKsTopologyInfo->CreateNodeInstance(dwExtensionNode, IID_IUnknown, (LPVOID *)&unKnown);
	CHECK_HR_RESULT(hr, "ks_topology_info->CreateNodeInstance(...)");

	hr = unKnown->QueryInterface(__uuidof(IKsControl), (void **)&ks_control);
	CHECK_HR_RESULT(hr, "ks_topology_info->QueryInterface(...)");

	kspNode.Property.Set = xuGuid;              // XU GUID
	kspNode.NodeId = (ULONG)dwExtensionNode;   // XU Node ID
	kspNode.Property.Id = xuPropertyId;         // XU control ID
	kspNode.Property.Flags = flags;             // Set/Get request

	hr = ks_control->KsProperty((PKSPROPERTY)&kspNode, sizeof(kspNode), (PVOID)data, len, readCount);
	CHECK_HR_RESULT(hr, "ks_control->KsProperty(...)");

done:
	SafeRelease(&ks_control);
	return hr;
}


EXTERN_DLL_EXPORT bool LibUVCWriteControl(BYTE *controlPacket, int length, ULONG *pReadCount)
{
	HRESULT	hr = GetVideoDevices();
	//
	int idxDevice = -1;
	//
	for(int i = 0; i < noOfVideoDevices; i++)
	{
		char videoDevName[MAX_PATH];
		size_t returnValue;
		//
		GetVideoDeviceFriendlyNames(i);
		wcstombs_s(&returnValue, videoDevName, MAX_PATH, szFriendlyName, MAX_PATH);
		if (!(strcmp(videoDevName, "Pixio StreamCube")))
			idxDevice = i;
	}
	bool fSuccess = false;
	//
	if( idxDevice >= 0 )
	{
		InitVideoDevice(idxDevice);
		//
		ULONG flags  = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
		//
		if (!SetGetExtensionUnit(xuGuid, 2, 1, flags, (void*)controlPacket, length, pReadCount))
			fSuccess = true;
	}
	for (UINT32 i = 0; i < noOfVideoDevices; i++)
	{
		SafeRelease(&ppVideoDevices[i]);
	}
	CoTaskMemFree(ppVideoDevices);
	SafeRelease(&pVideoConfig);
	SafeRelease(&pVideoSource);
	CoTaskMemFree(szFriendlyName);
	return fSuccess;
}


EXTERN_DLL_EXPORT bool LibUVCReadControl(BYTE* controlPacket, int length, ULONG* pReadCount)
{
	HRESULT	hr = GetVideoDevices();
	//
	int idxDevice = -1;
	//
	for (int i = 0; i < noOfVideoDevices; i++)
	{
		char videoDevName[MAX_PATH];
		size_t returnValue;
		//
		GetVideoDeviceFriendlyNames(i);
		wcstombs_s(&returnValue, videoDevName, MAX_PATH, szFriendlyName, MAX_PATH);
		if (!(strcmp(videoDevName, "Pixio StreamCube")))
			idxDevice = i;
	}
	bool fSuccess = false;
	//
	if (idxDevice >= 0)
	{
		InitVideoDevice(idxDevice);
		//
		ULONG flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
		//
		if (!SetGetExtensionUnit(xuGuid, 2, 2, flags, (void*)controlPacket, length, pReadCount))
			fSuccess = true;
	}
	for (UINT32 i = 0; i < noOfVideoDevices; i++)
	{
		SafeRelease(&ppVideoDevices[i]);
	}
	CoTaskMemFree(ppVideoDevices);
	SafeRelease(&pVideoConfig);
	SafeRelease(&pVideoSource);
	CoTaskMemFree(szFriendlyName);
	return fSuccess;
}

