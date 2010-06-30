// Video.h: interface for the Video class.
//
//////////////////////////////////////////////////////////////////////

#if !defined (AFX_PDICAPT_H__76e5281b_d41a_47e1_9b4e_deaf150b40be__INCLUDED_)
#define AFX_PDICAPT_H__76e5281b_d41a_47e1_9b4e_deaf150b40be__INCLUDED_



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "PDI.h"

#define NUMBER_OF_STATIONS 8

#include "SensorData.h"

class CPDICapture  
{
public:
	// Construction functions

	// bufSize: buffer size : long int
	// caption rate: PI_FRATE_120
	// data format of orientation: PDI_MODATA_QTRN
	// filter is disabled
	// callback function is disabled
	CPDICapture (long bufFrameNumber);

	// rate:    caption rate
	// PI_FRATE_120 or PI_FRATE_240
	CPDICapture (long bufFrameNumber, ePiFrameRate rate);

	// dataFormat: data format of orientation
	// PDI_MODATA_QTRN:   Orientation Quaternion, 16 bytes (4 FLOATS)
	// PDI_MODATA_DIRCOS: Direction Cosine Matrix, 36 bytes (3x3 FLOATs)
	// PDI_MODATA_ORI:    az, el, ro Euler ori angles, 12 bytes ( 3 FLOATs )
	CPDICapture (long bufFrameNumber, ePiFrameRate rate, ePDIMotionData oriDataFormat);
	CPDICapture (long bufFrameNumber, ePiFrameRate rate, ePDIMotionData oriDataFormat, CPDIfilter * pfilter);

	// CALLBACK function is temprarily NOT implemented
	CPDICapture (long bufFrameNumber, ePiFrameRate rate, ePDIMotionData oriDataFormat, CPDIfilter * pfilter, int func(LPARAM) );

	~CPDICapture();

	int GetNumberOfSensors ();	// positive : number of sensors
								// -1: error
								// 0:  no sensor

	// Interface functions
	// How many frames has already been captured
	int GetFrameCount();
	// Reset the frame count, the data pointer will be set to the head of buffer
	void ClearCount();

	// Get the data of certain frame,
	// 0: Current frame
	// -1: last frame
	// -2: Last last frame, ...
	PBYTE GetData (int reverseFrameCount);

	// PDI device control functions
	// 0:  success
	// -1: fail
	int  StartCapture();
	int  StopCapture();

	// Actually retrieve data
	void Capture (DWORD wParam, PBYTE lParam);

	// Advanced device control functions, don't need to call be client in ususal cases
	int  InitializePDIDevice ();
	int  Connect ();
	int  Disconnect ();

	// Draw the capture window to show the essential information
	void OnPaint ();

	// Response to device
	void CaptureStarted();
	void CaptureStopped();
	bool IsCaptureStarted();
	bool IsConnected();
	void WaitForCaptureStop();

	// Parse the data in each frame
	CSensorData * ParseFrame (PBYTE lparam);

	// Window and rendering context handles
	HWND m_hwnd; 
	HDC  m_hdc;

private:

	// Member variables for PDI device
	CPDIdev m_pdiDev;
	CPDImdat m_pdiMDat;
	CPDIfilter m_pdiFilter;
	CPDIfilter * m_pfilter;		// Data filter, it will be null for this version, reserved for future use

	// Number of frames already received
	DWORD m_frameCount;

	// Configue data for PDI capture device
	DWORD m_bufSize;				// number of frame, actual size will be determined with number of sensor and data format
	ePiFrameRate m_rate;		// Data sample rate
	ePDIMotionData m_oriDataFormat;	// Orientation data format

	// Callback function, NULL in this version
	int (*m_pfunc) (LPARAM);


	// Data buffer for capture
	PBYTE m_pData;

	// Pointer to error description
	LPCTSTR m_pErrorString;
	// Error code
	ePiErrCode m_errorCode;

	// Handle of new thread that to create a window to handle the message from PDI device
//	HANDLE m_thread;

	// Retrive the error code and description
	void GetError();

	// Calculate the memory length of one frame data from each sensor
	// Return number of bytes
	int LengthOfFrame();

	// Actual size of each frame
	int m_frameSize;
	int m_frameSizeInFloat;

	// Number of sensors attached to the PDI device
	int m_sensorCount;
	
	// Calculate the size of data buffer in the number of bytes
	DWORD CalculateBufferSize ();

	// For debug, store current data
/*	struct CurrentData {
		int sensor;
		float x, y, z;
		float q[4];
	} * m_pcurrentData;
*/
	// Initialize variables, called by constructors
	void InitVariables(long bufFrameNumber, ePiFrameRate rate, ePDIMotionData oriDataFormat, CPDIfilter * pfilter, int func(LPARAM) );

	// Show the content of each frame or not
	bool m_bShowFrameContent;

	// Status: is capture started
	bool m_bCaptureStarted;

	HANDLE m_hStopEvent;   // stop capture
	HANDLE m_hMutex;       // protect shared objects


public:
	void DataError(void);
private:
	long m_bufFrameNumber;
public:
	void ResetFrameCount(void);
	int GetBadFrameCount(void);
private:
	int m_badFrameCount;
public:
	int GetFrameSize(void);
};


#endif // !defined(AFX_VIDEO_H__76e5281b_d41a_47e1_9b4e_deaf150b40be__INCLUDED_)
