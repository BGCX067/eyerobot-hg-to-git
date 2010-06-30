#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "stdafx.h"
#include "pdifunc.h"

PDIFunc::PDIFunc(void)
{

}

PDIFunc::~PDIFunc(void)
{

}

///////////////device operation////////////////////
void PDIFunc::Connect(void)
{
	m_pdifp = fopen( "configlog.txt", "wt");
	fprintf(m_pdifp,"Initializing sensor...\n");
	if (!(m_pdiDev.CnxReady()))
	{
		
		m_pdiDev.SetSerialIF( &m_pdiSer );
		ePiCommType eType = m_pdiDev.DiscoverCnx();
		//eType = m_pdiDev.DiscoverCnx();
		switch (eType)
		{
		case PI_CNX_USB:
			//fprintf(m_pdifp, "USB Connection: %s\n", m_pdiDev.GetLastResultStr() );
			break;
		case PI_CNX_SERIAL:
			//fprintf(m_pdifp, "Serial Connection: %s\n", m_pdiDev.GetLastResultStr() );
			break;
		default:
			//fprintf(m_pdifp, "DiscoverCnx result: \n  %s\n", m_pdiDev.GetLastResultStr() );
			break;
		}
		if (m_pdiDev.CnxReady())
			SetupDevice();
		else
		{
			fprintf(m_pdifp, "Reconnecting...\n");
			Connect();
		}
		
		/*BOOL good = m_pdiDev.ConnectUSB();
		printf("bool= %d \n", good);*/
	}
	else
		fprintf(m_pdifp, "Already connected.\n");
}

void PDIFunc::Disconnect(void)
{
	if (!(m_pdiDev.CnxReady()))
		//fprintf(m_pdifp, "Already disconnected\n");
		printf("Already disconnected\n");
	else
	{
		m_pdiDev.Disconnect();
		m_pdiDev.CnxReady();
		//fprintf(m_pdifp, "Disconnect result: %s\n", m_pdiDev.GetLastResultStr() );
	}
	fclose(m_pdifp);
}

void PDIFunc::SetupDevice(void)
{
	m_pdiDev.SetBinary(1);
	m_pdiDev.SetMetric(1);
	m_pdiMDat.Empty();
	m_pdiMDat.Append( PDI_MODATA_FRAMECOUNT );
	m_pdiMDat.Append( PDI_MODATA_TIMESTAMP);
	m_pdiMDat.Append( PDI_MODATA_POS );
	m_pdiMDat.Append(PDI_MODATA_DIRCOS);//3*3 Direction Cosine Matrix
	m_pdiDev.SetSDataList( -1, m_pdiMDat );
	//fprintf(m_pdifp, "The data recorded will include: FrameCount, TimeStamp, Position and Rotation Matrix\n");

	CPDIbiterr cBE;
	m_pdiDev.GetBITErrs( cBE );

	CHAR sz[100];
	cBE.Parse( sz, 100 );
	//fprintf(m_pdifp, "BIT Errors: %s\n", sz );

	if (!(cBE.IsClear()))
	{
		m_pdiDev.ClearBITErrs();
		//fprintf(m_pdifp, "BITErrs Cleared.\n");
	}
}

void PDIFunc::StartCapture( PBYTE pbuffer, long  size)
{
	m_pdiDev.SetPnoBuffer( pbuffer, size);

	if (m_pdiDev.StartContPno(GetForegroundWindow()))
	{	
		PBYTE pbuf = NULL;
		while (pbuf == NULL)
		{
			Sleep(1);
			m_pdiDev.LastPnoPtr( pbuf, dwSize); 
		}
		//fprintf(m_pdifp, "Start recording data...\n" );
	}

	else
	{
		ePiErrCode err = m_pdiDev.GetLastResult();
		//fprintf(m_pdifp, "ERROR in StartContPno: %d\n",err);
	}
}

void PDIFunc::StopCapture( void )
{
	if (m_pdiDev.StopContPno())
		//fprintf(m_pdifp, "Stop recording data...\n" );
		printf("Stop recording data\n");
	else
	{
		ePiErrCode err = m_pdiDev.GetLastResult();
		//fprintf(m_pdifp, "ERROR in StopContPno: %d\n",err);
	}
}

void PDIFunc::ClearBuffer(void)
{
	PBYTE pbuf = NULL;
	m_pdiDev.LastPnoPtr( pbuf, dwSize); 
	int	FraCount1 = *(int *)(pbuf + HEADNUM*sizeof (float));
	if (FraCount1 > 10)
	{
		ePiErrCode err1, err2, err3;
		m_pdiDev.ResetTimeStamp();
		Sleep(5);
		err1 = m_pdiDev.GetLastResult();
		m_pdiDev.ResetFrameCount();
		Sleep(5);
		err2 = m_pdiDev.GetLastResult();
		m_pdiDev.ResetPnoPtr();
		Sleep(5);
		err3 = m_pdiDev.GetLastResult();
		if ((!err1) & (!err2) & (!err3))
			//fprintf (m_pdifp,"Buffer rounded back...\n");
			printf("buffer rounded back...\n");
		else
			//fprintf(m_pdifp,"ERROR in Reset: %d %d %d\n",err1, err2, err3); 
			m_pdiDev.LastPnoPtr( pbuf, dwSize); 
		int	FraCount2 = *(int *)(pbuf + HEADNUM*sizeof (float));
		//fprintf (m_pdifp,"Frame account went from %d to %d\n", FraCount1, FraCount2);
	}
}


/////////////////////////////acceess current data//////////////////////////////
//////GetData: sen1: x, y, z, m11, m12,m14,m21,m22,m23,m31,m32,m33
//////         sen2: x, y, z, m11, m12,m14,m21,m22,m23,m31,m32,m33
//////GetFrmTim: sen1_frame, sen2_frame  for del=0
//////           sen1_time, sen2_time  for del=1

PBYTE PDIFunc::GetCurBuf(void) //get current buffer pointer
{
	PBYTE pbuf;
	m_pdiDev.LastPnoPtr(pbuf, dwSize ); 
	return pbuf;
}

void PDIFunc::GetCurData( PBYTE pbuf, float **data) 
{
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	int sennum = dwSize / LenPerSen;
	for (int i = 0; i < sennum; i++) 
		memcpy (data[i],pbuf + LenPerSen * i + sizeof (float)*(HEADNUM+FLGNUM), sizeof (float)* 12);
}

void PDIFunc::GetCurData( PBYTE pbuf, int sen, float *data )
{
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	memcpy (data,pbuf + LenPerSen * sen + sizeof (float)*(HEADNUM+FLGNUM), sizeof (float)* 12);
}

void PDIFunc::GetCurData( PBYTE pbuf, int sen, int mem, float &data )
{
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	data = * (float*) (pbuf + LenPerSen * sen + sizeof (float)*(HEADNUM+FLGNUM+mem));
}

void PDIFunc::GetCurFrmTim( PBYTE pbuf, int del, int *data) //del=0 for frame, del = 1 for time
{
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	int sennum = dwSize / LenPerSen;
	int *m = new int [sennum];
	for (int i = 0; i < sennum; i++) 
		data[i] = *(int*)(pbuf + LenPerSen * i + sizeof (float)*(HEADNUM + del));
}

void PDIFunc::GetCurFrmTim( PBYTE pbuf, int del, int sen, int &data ) //del=0 for frame, del = 1 for time
{
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	data = *(int*)(pbuf + LenPerSen * sen + sizeof (float)*(HEADNUM + del));
}


/////////////////////////////copy data from buffer//////////////////////////////
//////CopyData: data[time][sen][data]
//////          sen1: x, y, z, m11, m12,m14,m21,m22,m23,m31,m32,m33
//////          sen2: x, y, z, m11, m12,m14,m21,m22,m23,m31,m32,m33
//////CopyFrmTim:data[time][sen]
//////           sen1_frame, sen2_frame  for del=0
//////           sen1_time, sen2_time  for del=1

void PDIFunc::CopyData(PBYTE pbeg, PBYTE pfin, float***data)
{
	int j = 0;
	for (PBYTE i = pbeg; i<=pfin; i=i+ dwSize )
	{
		GetCurData(i, data[j]) ;
		j++;
	}
}

void PDIFunc::CopyData(PBYTE pbeg, PBYTE pfin, int sen, float**data )
{
	int j = 0;
	for (PBYTE i = pbeg; i<=pfin; i=i+ dwSize )
	{
		GetCurData(i, sen, data[j]) ;
		j++;
	}
}

void PDIFunc::CopyData(PBYTE pbeg, PBYTE pfin, int sen, int mem, float *data )
{
	int j = 0;
	for (PBYTE i = pbeg; i<=pfin; i=i+ dwSize )
	{
		GetCurData(i, sen, mem, data[j]) ;
		j++;
	}
}

void PDIFunc::CopyFrmTim( PBYTE pbeg, PBYTE pfin, int del, int **data) //del=0 for frame, del = 1 for time
{
	int j = 0;
	for (PBYTE i = pbeg; i<=pfin; i=i+ dwSize )
	{
		GetCurFrmTim(i,del, data[j]) ;
		j++;
	}
}

void PDIFunc::CopyFrmTim( PBYTE pbeg, PBYTE pfin, int del, int mem, int *data) //del=0 for frame, del = 1 for time
{
	int j = 0;
	for (PBYTE i = pbeg; i<=pfin; i=i+ dwSize )
	{
		GetCurFrmTim(i, del, mem, data[j]) ;
		j++;
	}
}

///////////////print to some file////////////////////////////////
///////////sens1: frame, time, x, y, z,  m11, m12,m14,m21,m22,m23,m31,m32,m33
///////////sens2: frame, time, x, y, z,  m11, m12,m14,m21,m22,m23,m31,m32,m33

void PDIFunc::PrintToFile (PBYTE pbeg, PBYTE pfin, FILE* fp)
{
	float* pdata;
	int* ptime;
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	int sennum = dwSize / LenPerSen;

	for( PBYTE pbuf = pbeg; pbuf<=pfin; pbuf+=dwSize )
	{
		for (int j = 0; j<sennum; j++) //for each sensor
		{
			ptime = (int*) (pbuf + LenPerSen * j + sizeof (float)*(HEADNUM));//position and rotation
			fprintf( fp, "%d %d ", ptime[0], ptime[1]);
			pdata = (float*) (pbuf + LenPerSen * j + sizeof (float)*(HEADNUM+FLGNUM));//position and rotation
			for (int i = 0; i<12; i++)
				fprintf( fp, "%f ", pdata[i]);
			fprintf (fp, "\n");
		}
	}
}
/*** added by Slavik ***/
/*returns number of connected sensors*/
int PDIFunc :: GetSennum(void){
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	int sennum = dwSize / LenPerSen;
	return sennum;
}


/*assumes tracker is @240HZ*/
void PDIFunc::PrintToFilePerSec (PBYTE pbeg, PBYTE pfin, FILE* fp, int fps)
{
	float* pdata;
	int* ptime;
	int member = HEADNUM + FLGNUM + DATNUM;
	int LenPerSen = member*sizeof (float);
	int sennum = dwSize / LenPerSen;

	for( PBYTE pbuf = pbeg; pbuf<=pfin; pbuf+= dwSize )
	{
		for (int j = 0; j<sennum; j++) //for each sensor
		{
			ptime = (int*) (pbuf + LenPerSen * j + sizeof (float)*(HEADNUM));//position and rotation

			if((fps != 0) && (ptime[0] % (240/fps) == 0)){
				fprintf( fp, "%d %d ", ptime[0], ptime[1]);
				pdata = (float*) (pbuf + LenPerSen * j + sizeof (float)*(HEADNUM+FLGNUM));//position and rotation
				for (int i = 0; i<12; i++)
					fprintf( fp, "%f ", pdata[i]);
				fprintf (fp, "\n");
			}
		}
	}
}

/***end of added by Slavik ***/

///////////////allocate space////////////////////////////////

float ***PDIFunc::AllocMatrix_Float(int higs, int rows, int cols)
{
	float ***m;

	m = new float **[higs];
	for (int j = 0; j<higs; j++){
		m[j] = new float *[rows];
		for (int i = 0; i < rows; i++) {
			m[j][i] = new float [cols];
		}
	}
	return (m);
}

float **PDIFunc::AllocMatrix_Float(int rows, int cols)
{
	int i;
	float **m;

	m = new float *[rows];
	for (i = 0; i < rows; i++) {
		m[i] = new float [cols];
	}
	return (m);
}

int **PDIFunc::AllocMatrix_Int(int rows, int cols)
{
	int i;
	int **m;

	m = new int *[rows];
	for (i = 0; i < rows; i++) {
		m[i] = new int [cols];
	}
	return (m);
}

void PDIFunc::DeleMatrix(float **m, int rows)
{
	for (int i = 0; i < rows; i++) {
		delete[] m[i];
	}
	delete m;
}

void PDIFunc::DeleMatrix(float ***m, int rows, int cols)
{
	for (int i = 0; i < rows; i++) {
		for (int j=0; j<cols; j++)
			delete[] m[i][j];
		delete[] m[i];
	}
	delete m;
}

void PDIFunc::DeleMatrix(int **m, int rows)
{
	for (int i = 0; i < rows; i++) {
		delete[] m[i];
	}
	delete m;
}

