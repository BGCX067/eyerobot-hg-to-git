// Robot_Control.h: interface for the Robot_Control class library.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROBOT_CONTROL_H__BE26C000_B72E_11DA_93B5_0003FF721802__INCLUDED_)
#define AFX_ROBOT_CONTROL_H__BE26C000_B72E_11DA_93B5_0003FF721802__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable: 4996)
#endif

//currently the max number of gridpoints is 100x100x100.  
//You can change this but keep it reasonable.
#define MAX 100
//////////////////////////////////////////////////
//
/*****		IMPORT ACTIVEROBOT LIBRARY		*****/

//you must have this library to use the robot...
#import "C:\Program Files\CRS Robotics\ActiveRobot\ActiveRobot.dll"
using namespace ACTIVEROBOTLib; 

//////////////////////////////////////////////////

class Robot_Control  
{
public:
	Robot_Control();
	virtual ~Robot_Control();
public:

	//turn on/off a GPIO pin.  Be very careful when wiring an external component to the GPIO connector
	int GPIO(int pin, int onoff);
	
	//tell the robot to move to some point in space relative to the center of the 
	//robot base.  The rotations are the rotations of the tool at the end of the robot arm.
	int MoveTo_Actual_Point(float x, float y, float z, float rotatex, float rotatey);
	
	//gives you the current x,y,z position of the robot in a space oriented relative 
	//to the center of the base of the robot...
	float Get_Z_Position(void);
	float Get_Y_Position(void);
	float Get_X_Position(void);

	//tells you the current grid counter location.  Useful for moving to the next.
	//you have to handle avoiding trying to move to a point past the grid size.
	//it will move to its origin if you call beyond what is initialized.
	int Get_Z_Counter(void);
	int Get_Y_Counter(void);
	int Get_X_Counter(void);
	

	//move to a grid point marked by the x,y,z counter of the grid array, 0,0,0 being the origin
	int MoveToGridPoint( int xcounter, int ycounter, int zcounter);
	
	//call this with the number of points, origin of your grid and spacing between
	//grid points...
	int Compute_Grid(int points,int pointsz, float x, float y, float z, float spacing);
	
	//you must call this to initialize a robot object and get control of the robot
	int Initialize( void );
	ICRSLocationPtr GetLocation( _bstr_t V3FileName, _bstr_t Variable );
	void Ready( void );

private:
	// pick and place variables
	typedef struct{
		float x;
		float y;
		float z;
		float xrot;
		float yrot;
	}fgrid;
	ICRSLocationPtr grid[MAX][MAX][MAX];	// grid locations
	fgrid mygrid[MAX][MAX][MAX];	// grid locations
	//float AppDepDist;				// approach/depart distance and grip force
	long TheSpeed;							// speed and grip force
	ICRSV3FilePtr TestV3;		// to retrieve locations   
	ICRSRobotPtr Robbie;	
	ICRSLocationPtr locationtemp;
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
