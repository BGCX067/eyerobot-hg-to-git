// Robot_Control.h: interface for the Robot_Control class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROBOT_CONTROL_H__BE26C000_B72E_11DA_93B5_0003FF721802__INCLUDED_)
#define AFX_ROBOT_CONTROL_H__BE26C000_B72E_11DA_93B5_0003FF721802__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX 100
//////////////////////////////////////////////////
//
/*****		IMPORT ACTIVEROBOT LIBRARY		*****/

#import "C:\Program Files\CRS Robotics\ActiveRobot\ActiveRobot.dll"
using namespace ACTIVEROBOTLib; 

//////////////////////////////////////////////////

class Robot_Control  
{
public:
	Robot_Control();
	virtual ~Robot_Control();
public:
	int ReleaseControl(void);
	int GPIO(int pin, int onoff);
	int MoveTo_Actual_Point(float x, float y, float z, float rotatex, float rotatey);
	float Get_Z_Position(void);
	float Get_Y_Position(void);
	float Get_X_Position(void);
	int Get_Z_Counter(void);
	int Get_Y_Counter(void);
	int Get_X_Counter(void);
	int MoveToGridPoint( int xcounter, int ycounter, int zcounter);
	int Compute_Grid(int points, float x, float y, float z, float spacing);
	int Initialize( void );

private:
	// pick and place variables
	ICRSLocationPtr grid[MAX][MAX][MAX];	// pick and place locations
	//float AppDepDist, GForce;				// approach/depart distance and grip force
	long TheSpeed;							// speed and grip force
	ICRSRobotPtr Robbie;
	ICRSLocationPtr locA, locB, locSafe, pos1, pos2, pos3, pos4, pos5, pos6, fetalpos;	// pick and place locations
	float AppDepDist, GForce;	// approach/depart distance and grip force
//	long TheSpeed;				// speed and grip force
	long TheSpeed2;				// speed and grip force
	long TheSpeed3;				// speed and grip force
	ICRSV3FilePtr TestV3;		 // to retrieve locations
	ICRSV3FilePtr fetal;
	char WorkString[255];
protected:
    
	int counterX;
	int counterY;
	int counterZ;
	float current_xrotation;
	float current_yrotation;
	float toolX;
	float toolY;
	float toolZ;
};

#endif // !defined(AFX_ROBOT_CONTROL_H__BE26C000_B72E_11DA_93B5_0003FF721802__INCLUDED_)
