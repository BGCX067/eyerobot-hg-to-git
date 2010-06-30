/////////////////////////////////////////////////////////////////////
// Polhemus Inc.,  www.polhemus.com
// © 2003 Alken, Inc. dba Polhemus, All Rights Reserved
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
//  Filename:           $Workfile: PDItracker.h $
//
//  Project Name:       Polhemus Developer Interface  
//
//  Description:        Tracker-Specific definitions
//
//  VSS $Header: /PIDevTools/Inc/PDItracker.h 4     6/20/03 9:30p Sgagnon $  
//
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
#ifndef _PDITRACKER_H_
#define _PDITRACKER_H_

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Liberty Binary Mode Header Format
/////////////////////////////////////////////////////////////////////

typedef struct _BIN_HDR_STRUCT 
{
	short preamble;              
	BYTE station;
	BYTE cmd;
	BYTE err;
	BYTE reserved;
	short length;

}*LPBINHDR,BINHDR;

#define LIBERTY_PREAMBLE 0x594C

/////////////////////////////////////////////////////////////////////
// Liberty Error Codes
/////////////////////////////////////////////////////////////////////

typedef enum
{
	PI_DEVERR_NO_ERROR = 0				//	0

	// Command errors
	, PI_DEVERR_INVALID_COMMAND = 1		//	1
	, PI_DEVERR_STATION_OUT_OF_RANGE	//	2
	, PI_DEVERR_INVALID_PARAMETER		//	3
	, PI_DEVERR_TOO_FEW_PARAMETERS		//	4
	, PI_DEVERR_TOO_MANY_PARAMETERS		//	5
	, PI_DEVERR_PARAMETER_BELOW_LIMIT	//	6
	, PI_DEVERR_PARAMETER_ABOVE_LIMIT	//	7
	, PI_DEVERR_SP_COM_FAILURE			//	8
	, PI_DEVERR_SP1_INIT_FAILURE		//	9
	, PI_DEVERR_SP2_INIT_FAILURE		//	10
	, PI_DEVERR_SP3_INIT_FAILURE		//	11
	, PI_DEVERR_SP4_INIT_FAILURE		//	12
	, PI_DEVERR_SP_NONE_DETECTED		//	13
	, PI_DEVERR_SRC_INIT_FAILURE		//	14
	, PI_DEVERR_MEM_ALLOC_ERROR			//	15
	, PI_DEVERR_EXCESS_CMD_CHARS		//	16
	, PI_DEVERR_EXIT_UTH				//	17
	, PI_DEVERR_SOURCE_READ_ERROR		//	18
	, PI_DEVERR_READONLY_ERROR			//  19
	, PI_DEVERR_TEXT_MESSAGE			//  20

	// Realtime errors 
	, PI_DEVERR_RT_SRC_FAIL_a_X			//  21 'a'
	, PI_DEVERR_RT_SRC_FAIL_b_Y			//  22 'b'
	, PI_DEVERR_RT_SRC_FAIL_c_XY		//  23 'c'
	, PI_DEVERR_RT_SRC_FAIL_d_Z			//  24 'd'
	, PI_DEVERR_RT_SRC_FAIL_e_XZ		//  25 'e'
	, PI_DEVERR_RT_SRC_FAIL_f_YZ		//  26 'f'
	, PI_DEVERR_RT_SRC_FAIL_g_XYZ		//  27 'g'

	, PI_DEVERR_RT_SRC_FAIL_A_X			//  28 'A'
	, PI_DEVERR_RT_SRC_FAIL_B_Y			//  29 'B'
	, PI_DEVERR_RT_SRC_FAIL_C_XY		//  30 'C'
	, PI_DEVERR_RT_SRC_FAIL_D_Z			//  31 'D'
	, PI_DEVERR_RT_SRC_FAIL_E_XZ		//  32 'E'
	, PI_DEVERR_RT_SRC_FAIL_F_YZ		//  33 'F'
	, PI_DEVERR_RT_SRC_FAIL_G_XYZ		//  34 'G'

	, PI_DEVERR_RT_BITERR				//  35 'I'

	, PI_DEVERR_MAX
}ePiDevError;

/////////////////////////////////////////////////////////////////////
// END $Workfile: PDItracker.h $
/////////////////////////////////////////////////////////////////////
#endif // _PDITRACKER_H_
