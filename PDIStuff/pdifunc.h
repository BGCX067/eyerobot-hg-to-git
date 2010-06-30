#pragma once

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "pdi.h"

#define HEADNUM 2 //2 flag
#define FLGNUM 2 //frame and time
#define DATNUM 12 // 3(pos) + 9(rot matrix) 

class PDIFunc
{
public:
	 PDIFunc(void);
	~PDIFunc(void);

	//device operation
	void Connect(void); //connect to the device
	void Disconnect(void); //disconnet the device
	void SetupDevice(void); //setup devide
	void StartCapture( PBYTE pbuffer, long  BUFFER_SIZE); //initialize data buffer, ready to record data
	void StopCapture( void );//stop to capture data
	void ClearBuffer(void); //buffer goes back to the beginning

	// Access buffer data by pointer
	PBYTE GetCurBuf(void);
	void GetCurData( PBYTE pbuf, float **data);
	void GetCurData( PBYTE pbuf, int sen, float *data );
	void GetCurData( PBYTE pbuf, int sen, int mem, float &data );
	void GetCurFrmTim( PBYTE pbuf, int del, int *data);
	void GetCurFrmTim( PBYTE pbuf, int del, int sen, int &data);

	// Access latest data by pointer
	void CopyData(PBYTE pbeg, PBYTE pfin, float ***data );
	void CopyData(PBYTE pbeg, PBYTE pfin, int sen, float **data );
	void CopyData(PBYTE pbeg, PBYTE pfin, int sen, int mem, float *data );
	void CopyFrmTim( PBYTE pbeg, PBYTE pfin, int del, int **data); //del=0 for frame, del = 1 for time
	void CopyFrmTim( PBYTE pbeg, PBYTE pfin, int del, int mem, int *data); //del=0 for frame, del = 1 for time

	//print to some file
	void PrintToFile (PBYTE pbeg, PBYTE pfin, FILE* fp);

	/*** added by Slavik ***/
	void PrintToFilePerSec (PBYTE pbeg, PBYTE pfin, FILE* fp,int fps);//print to file only one frame out of 240/120
	int  GetSennum(void);
	/***end of added by Slavik ***/

	//allocate space
	float ***AllocMatrix_Float(int higs, int rows, int cols);
	float **AllocMatrix_Float(int rows, int cols);
	int **AllocMatrix_Int(int rows, int cols);
	void DeleMatrix(float ***m, int rows, int cols);
	void DeleMatrix(float **m, int rows);
	void DeleMatrix(int **m, int rows);

	DWORD dwSize;
	CPDIser	m_pdiSer;
	CPDIdev m_pdiDev;
	CPDImdat m_pdiMDat;
	FILE* m_pdifp;
};
