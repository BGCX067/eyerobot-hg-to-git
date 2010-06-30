// Robot_Control.cpp: implementation of the Robot_Control class.
//
//////////////////////////////////////////////////////////////////////

#include "Robot_Control.h"
#include <stdio.h>
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/* world position in mm*/
#define CENTERX 584.8
#define CENTERY 0.0
#define CENTERZ 508.0
#define CENTERROTX 0.0
#define CENTERROTY 0.0

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Robot_Control::Robot_Control()
{

}

Robot_Control::~Robot_Control()
{
	//Robbie->LockAxes(alAllAxes);
	Robbie->UnlockAxes(alAllAxes);
	Robbie->ControlRelease();
}

int Robot_Control::Initialize()
{
	counterX=0;
	counterY=0;
	counterZ=0;
	toolX=0;
	toolY=0;
	toolZ=0;
	current_xrotation = 0;
	current_yrotation = 0;

	if FAILED(CoInitialize (NULL)) 
	{
		MessageBoxA(NULL,(LPCSTR)"CoInitialize() failed.","Error", MB_OK);
		fprintf(stderr,"CoInitialize() failed.");
		exit(0);
	}

	try 
	{

		// initialize robot pointer variables
		Robbie = ICRSRobotPtr(__uuidof(CRSRobot));
		TestV3 = ICRSV3FilePtr(__uuidof(CRSV3File));

		Robbie->put_Units( utMetric );
		//Robbie->put_Units( utEnglish );
		
		TheSpeed = 80;

		//get control of robot
		Robbie->ControlGet();

		// open V3File
		TestV3->Open("reach_eye.v3", v3fReadOnly);
		//TestV3->Open("metr.v3", v3fReadOnly);
	
		//retrieve variables from V3 file
		locationtemp = TestV3->GetLocation("initial", 0, 0);
			
		// close file
		TestV3->Close();
		
		//Unlock all the axes...
		Robbie->UnlockAxes(alAllAxes);

		for (int i = 0;i<100;i++){
			for (int j=0;j<100;j++){
				for (int k=0;k<100;k++){
					grid[i][j][k] = ICRSLocationPtr(__uuidof(CRSLocation));
					grid[i][j][k] = locationtemp;
					grid[i][j][k]->x=CENTERX;
					grid[i][j][k]->y=CENTERY;
					grid[i][j][k]->z=CENTERZ;
					grid[i][j][k]->xrot=CENTERROTX;
					grid[i][j][k]->yrot=CENTERROTY;
				}}}

	}
	catch (_com_error MyError)
	{
		// display error message
		sprintf(WorkString, "The following error occurred during initialization --\n%s", (LPCTSTR) MyError.Description());
		fprintf(stderr,"%s",WorkString);
		MessageBoxA(NULL,(LPCSTR) MyError.Description(),"The following error occurred during initialization --", MB_OK);
		//		MessageBox(NULL,WorkString,"Error", MB_OK);
		//AfxMessageBox(WorkString);

		//close the program of the parent...or let them try something else...
		Robbie->UnlockAxes(alAllAxes);
		Robbie->ControlRelease();
		return 0;
	}


	return 1;
}

ICRSLocationPtr Robot_Control::GetLocation( _bstr_t V3FileName, _bstr_t Variable ){

		TestV3->Open( V3FileName, v3fReadOnly);
		//retrieve variables from V3 file
		locationtemp = TestV3->GetLocation(Variable, 0, 0);
		// close file
		TestV3->Close();
return locationtemp;
}

int Robot_Control::Compute_Grid(int points,int pointsz, float x, float y, float z, float spacing)
{
	int i, j, k; //counter variables temporary
	//int validStatus;
	//check the boundaries of the given world axis and the grid - is it in bounds?
	//instantiate a v3 object, create a file, and let's add variables to it...  
	//transform the world coordinate to the given origin relative to the robot base...
	//generate the grid...

	FILE *fp1 = fopen("zztest.txt","wt");

	/*in sensor coords: i = z, j = x, k = y*/
	/*in robot coords:  i = x, j = y, k = z*/
	for (i=0;i<points;i++){
		for (j=0;j<points;j++){
			for (k=0;k<pointsz;k++){
		
				mygrid[i][j][k].x = (float) (spacing*i+x);
				mygrid[i][j][k].y = (float) (spacing*j+y);
				mygrid[i][j][k].z = (float) (spacing*k+z);
				mygrid[i][j][k].xrot = 0.0;
				mygrid[i][j][k].yrot = 0.0;

				printf("Vals: x: %f y: %f z: %f\n", mygrid[i][j][k].x,mygrid[i][j][k].y,mygrid[i][j][k].z);
				fprintf(fp1,"%f\t%f\t%f\n", mygrid[i][j][k].x,mygrid[i][j][k].y,mygrid[i][j][k].z);
			}
		}  
	}

	fprintf(fp1,"\nCorrected coordinates\n");

	float lsens = 10.25;   //length of sensor in in. 26cm or 10.25 in

	for (int i=0;i<points;i++){
		for (int j=0;j<points;j++){
			for (int k=0;k<pointsz;k++){
				float r1 = atan2(mygrid[i][j][k].y, mygrid[i][j][k].x);
				float r2 = sqrt( pow(mygrid[i][j][k].x,2)+pow(mygrid[i][j][k].y,2)) - lsens;
				float u = r2 * cos( r1 ) ;
				float v = r2 * sin( r1 );

				mygrid[i][j][k].x = u;
				mygrid[i][j][k].y = v;
				printf("New Vals: x: %f y: %f z: %f\n", mygrid[i][j][k].x,mygrid[i][j][k].y,mygrid[i][j][k].z);
				fprintf(fp1,"%f\t%f\t%f\n", mygrid[i][j][k].x,mygrid[i][j][k].y,mygrid[i][j][k].z);

			}}}

	fclose(fp1);
	return (points*3);
}


int Robot_Control::MoveToGridPoint(int xcounter, int ycounter, int zcounter)
{

	//locationtemp->PutFlags( ACTIVEROBOTLib::locFlags(16) );

	locationtemp->x = (float)mygrid[xcounter][ycounter][zcounter].x;
	locationtemp->y = (float)mygrid[xcounter][ycounter][zcounter].y;
	locationtemp->z = (float)mygrid[xcounter][ycounter][zcounter].z;
	locationtemp->xrot = (float)mygrid[xcounter][ycounter][zcounter].xrot;
	locationtemp->yrot = (float)mygrid[xcounter][ycounter][zcounter].yrot;


	VARIANT_BOOL metr = locationtemp->IsMetric;

	locFlags flags = locFlags();
	locationtemp->get_Flags(&flags);

	try 
	{
		Robbie->PutSpeed(TheSpeed);
		Robbie->Move(locationtemp);
		//Robbie->Move(grid[xcounter][ycounter][zcounter]);
	}
	catch (_com_error MyError)
	{
		// display error message
		sprintf(WorkString, "The following error occurred in MoveToGridPoint --\n%s", (LPCTSTR) MyError.Description());
		fprintf(stderr,"%s",WorkString);
		MessageBoxA(NULL,(LPCSTR) MyError.Description(),"The following error occurred in MoveToGridPoint", MB_OK);
		//AfxMessageBox(WorkString);
		return 0;
	}
	counterX = xcounter;
	counterY = ycounter;
	counterZ = zcounter;
	return 1;
}

int Robot_Control::Get_X_Counter(void)
{
	if (counterX<0 || counterX>1000)
	{
		fprintf(stderr,"CounterX is not initialized yet");
		//AfxMessageBox("CounterX is not initialized yet", MB_OK, -1);
		//MessageBox(NULL,"CounterX is not initialized yet","Error", MB_OK);
		return -1;
	}
	else
		return (counterX);
}

int Robot_Control::Get_Y_Counter()
{
	if (counterY<0 || counterY>1000)
	{
		fprintf(stderr,"CounterY is not initialized yet");
		//AfxMessageBox("CounterY is not initialized yet", MB_OK, -1);
		//		MessageBox(NULL,"CounterY is not initialized yet","Error", MB_OK);
		return -1;
	}
	else
		return (counterY);
}

int Robot_Control::Get_Z_Counter()
{
	if (counterZ<0 || counterZ>1000)
	{
		fprintf(stderr,"CounterZ is not initialized yet");
		//AfxMessageBox("CounterZ is not initialized yet", MB_OK, -1);
		//MessageBox(NULL,"CounterZ is not initialized yet","Error", MB_OK);
		return -1;
	}
	else
		return (counterZ);
}


float Robot_Control::Get_X_Position()
{
	return (grid[counterX][counterY][counterZ]->x);
}

float Robot_Control::Get_Y_Position()
{
	return (grid[counterX][counterY][counterZ]->y);
}

float Robot_Control::Get_Z_Position()
{
	return (grid[counterX][counterY][counterZ]->z);
}

int Robot_Control::MoveTo_Actual_Point(float x, float y, float z, float rotatex, float rotatey)
{
	//locationtemp = ICRSLocationPtr(__uuidof(CRSLocation));
	locationtemp->Putx(x);
	locationtemp->Puty(y);
	locationtemp->Putz(z);
	locationtemp->Putxrot(rotatex);
	locationtemp->Putyrot(rotatey);
	float x1 = locationtemp->Getx();
	float y1 = locationtemp->Gety();
	float z1 = locationtemp->Getz();
	float xr1 = locationtemp->Getxrot();
	float xy1 = locationtemp->Getyrot();
	
	locationtemp->GetIsValid();
	/*locationtemp->x = (float)x;
	locationtemp->y = (float)y;
	locationtemp->z = (float)z;
	locationtemp->xrot =(float)rotatex;
	locationtemp->yrot =(float)rotatey;*/
	VARIANT_BOOL validloc = false;
	validloc = locationtemp->GetIsValid();
	


	if( validloc ){
		try
		{
			Robbie->PutSpeed(TheSpeed);
			//Robbie->MoveStraight(locationtemp);  //not interpolated movement?
			//Robbie->raw_Move(locationtemp);	//no error msgs
			Robbie->Move(locationtemp);
			//Robbie->Approach(locationtemp,10);
			
		}
		catch (_com_error MyError)
		{
			// display error message
			sprintf(WorkString, "The following error occurred during movement to actual point --\n%s", (LPCTSTR) MyError.Description());
			fprintf(stderr,"%s",(LPCTSTR) MyError.Description());
			//fprintf(stderr,"%s",WorkString);
			//AfxMessageBox(WorkString);
			::MessageBoxA(NULL,(LPCSTR) MyError.Description(),"Error", MB_OK);
			//close the program of the parent...or let them try something else...
			return 0;
		}
	}
	else{	//if invalid location
		fprintf(stderr,"Point %f %f %f is invalid\n",x,y,z);
	}
	return 1;
}

void Robot_Control::Ready( void ){
Robbie->Ready();
}


int Robot_Control::GPIO(int pin, int onoff)
{
	try
	{	/* OR outputs and incoming pin for on*/
		if( onoff ){
		long out = Robbie->Outputs[1];
		out = ((long) 1 << (pin - 1)) | out; //(long) pow( (float) 2, (pin-1) ) | out;
		Robbie->PutOutputs(1, out);
		}	
		/* NAND outputs and incoming pin for on*/
		else{
		long out = Robbie->Outputs[1];
		out = out & (long)((long) 0xFFFF - (long) (1 << (pin - 1)));//((long) pow( (float) 2, (pin-1) ));
		Robbie->PutOutputs(1, out);
		}

	}
	catch (_com_error MyError)
	{
		// display error message
		sprintf(WorkString, "The following error occurred during GPIO output change --\n%s", (LPCTSTR) MyError.Description());
		fprintf(stderr,"%s",WorkString);
		::MessageBoxA(NULL,(LPCSTR) MyError.Description(),"Error", MB_OK);
		//AfxMessageBox(WorkString);
		//MessageBox(NULL,WorkString,"Error", MB_OK);
		//close the program of the parent...or let them try something else...
		return 0;
	}
	return 1;
}
