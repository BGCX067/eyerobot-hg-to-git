// Robot_ReachingDlg.cpp : implementation file
//
/************************************************
	Robot Reaching: initializes robot arm and moves it
	to preset locations
	CAUTION: double check locations and origin in robot
	firmware - it will crash into smth if locations are
	incorrect
	Outputs data in SAM ascii format
**************************************************/


#include "stdafx.h"
#include "Robot_Reaching.h"
#include "Robot_ReachingDlg.h"


#include "luautils.h"


//polhemus stuff...
//#include "PDI.h"
//#include "pdifunc.h"
#include <time.h>
#include <mmsystem.h>
#include <algorithm>
#include <vector>
#include <sstream>

using namespace std;



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define BUFFER_SIZE 1000000000
BYTE BUFFER[BUFFER_SIZE];

//in mm
#define CENTERX 380.0
#define CENTERY 0.0
#define CENTERZ 300.0
#define YOFFSET 100.0	// y offset of the hand from the base
#define ZOFFSET 100.0	// z offset of the hand from the base



#define DATA_LENGTH 12
#define WAIT 2500   //how long the robot stays at target
#define MOVWAIT 1000   //wait for the robot to finish moving
#define LEDONTIME 2000 //led on at fixation point
#define ROBOT2TARGET 300 //max time(guesstimate) taken to target location
#define	TONE2END 3000 //from start tone to end of trial(robot back to initial)
#define LED2TONE 1000 
#define TONE2LED 200 //time that LED goes off after the tone
#define SPEED 90 //ROBOT speed
#define FPS 240

using namespace std;
//PDIFunc pdi;//tracker
//PBYTE pbeg, pcur, pfin;//tracker
CStdioFile fp;

std:: vector < double > data[DATA_LENGTH]; //data from tracker
float curData[DATA_LENGTH]; //data from 1 sensor
int newTime;
int oldTime = -1;
int sensors = 0;	    //number of sensors used
int counter = 1;		//trial counter;
int framec = 0;			//frame counter
int trialNum[4] = {90, 90, 90, 90}; //fabian note; total no of trials, can change to alter the number of trials
int withinTrialTargetNum; //fabian note; total no of trials, can change to alter the number of trials
int targetSeq[MAXEVENTS][MAXEVENTS];
int condNum[4] = {1, 1, 2, 3};//the LED sequence. DO NOT CHANGE

//bool flag = false;		//flag for recording state (i.e. recording if !flag)
bool initialized = false;	//flag for usage of tracker, false if tracker not used
CString str, name("");

//UINT __cdecl threadFoo( LPVOID pParam );


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CRobot_ReachingDlg dialog




CRobot_ReachingDlg::CRobot_ReachingDlg(CWnd* pParent /*=NULL*/)
: CDialog(CRobot_ReachingDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRobot_ReachingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO1, radbut1);
	DDX_Control(pDX, IDC_RADIO2, radbut2);
	DDX_Control(pDX, IDC_RADIO3, radbut3);
	DDX_Control(pDX, IDC_RADIO4, radbut4);
	DDX_Control(pDX, IDC_COMBO1, m_combo1);
	DDX_Control(pDX, IDC_COMBO2, m_combo2);
	DDX_Control(pDX, IDC_COMBO3, m_combo3);
	DDX_Control(pDX, IDC_COMBO4, m_combo4);
	DDX_Control(pDX, IDC_COMBO5, m_combo5);
	DDX_Control(pDX, IDC_COMBO6, m_combo6);
	DDX_Control(pDX, IDC_COMBO7, m_combo7);
	DDX_Control(pDX, IDC_COMBO8, m_combo8);
}

BEGIN_MESSAGE_MAP(CRobot_ReachingDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CRobot_ReachingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CRobot_ReachingDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CRobot_ReachingDlg::OnBnClickedButton2)
	//ON_BN_CLICKED(IDC_BUTTON3, &CRobot_ReachingDlg::OnBnClickedButton3)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON4, &CRobot_ReachingDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CRobot_ReachingDlg message handlers

BOOL CRobot_ReachingDlg::OnInitDialog()
{
	Robot = NULL;	

	current_trial_counter = 0;	//reset trial counter
	currentTarget = 0;
	
	/*init repetition tracker*/
	for ( int i=0;i<LOCATIONNUM-1;i++){
		repetitions[i] = 0;
	}


	for ( int i=0;i< trialNum[condition_type - 1] / 2;i++){
		for ( int j=0;j<2;j++){
			current_trial[2*i+j] = j;
	}}

	CDialog::OnInitDialog();

	/*turn off 'Next Trial', 'redo trial' and 'Stop Recording'*/
	CWnd * pbutton = GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(false);
	pbutton = GetDlgItem( IDC_BUTTON3);
	pbutton->EnableWindow(false);	
	pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(false);	

	radbut1.SetCheck(BST_CHECKED);    //Set one of the radiobuttons to be selected

	m_combo1.AddString(_T("Control"));
	m_combo1.AddString(_T("Parkinsons DBS"));
	m_combo1.SetCurSel(0);

	m_combo2.AddString(_T("Right"));
	m_combo2.AddString(_T("Left"));
	m_combo2.SetCurSel(1);

	/*Robot*/
	m_combo3.AddString(_T("Robot"));
	m_combo3.AddString(_T("Hand"));
	m_combo3.AddString(_T("Thumb"));
	m_combo3.AddString(_T("Index"));
	m_combo3.AddString(_T("Middle"));
	m_combo3.AddString(_T("Wrist"));
	m_combo3.AddString(_T("Elbow"));
	m_combo3.AddString(_T("Shoulder"));
	m_combo3.AddString(_T("Forearm"));
	m_combo3.AddString(_T("Upperarm"));
	m_combo3.AddString(_T("none"));
	m_combo3.SetCurSel(0);
	
	/*Hand*/
	m_combo4.AddString(_T("Robot"));
	m_combo4.AddString(_T("Hand"));
	m_combo4.AddString(_T("Thumb"));
	m_combo4.AddString(_T("Index"));
	m_combo4.AddString(_T("Middle"));
	m_combo4.AddString(_T("wrist"));
	m_combo4.AddString(_T("elbow"));
	m_combo4.AddString(_T("shoulder"));
	m_combo4.AddString(_T("Forearm"));
	m_combo4.AddString(_T("Upperarm"));
	m_combo4.AddString(_T("none"));
	m_combo4.SetCurSel(1);

	/*Wrist*/
	m_combo5.AddString(_T("Robot"));
	m_combo5.AddString(_T("Hand"));
	m_combo5.AddString(_T("Thumb"));
	m_combo5.AddString(_T("Index"));
	m_combo5.AddString(_T("Middle"));
	m_combo5.AddString(_T("wrist"));
	m_combo5.AddString(_T("elbow"));
	m_combo5.AddString(_T("shoulder"));
	m_combo5.AddString(_T("Forearm"));
	m_combo5.AddString(_T("Upperarm"));
	m_combo5.AddString(_T("none"));
	m_combo5.SetCurSel(5);

	/*Elbow*/
	m_combo6.AddString(_T("Robot"));
	m_combo6.AddString(_T("Hand"));
	m_combo6.AddString(_T("Thumb"));
	m_combo6.AddString(_T("Index"));
	m_combo6.AddString(_T("Middle"));
	m_combo6.AddString(_T("wrist"));
	m_combo6.AddString(_T("elbow"));
	m_combo6.AddString(_T("shoulder"));
	m_combo6.AddString(_T("Forearm"));
	m_combo6.AddString(_T("Upperarm"));
	m_combo6.AddString(_T("none"));
	m_combo6.SetCurSel(6);

	/*shoulder*/
	m_combo7.AddString(_T("Robot"));
	m_combo7.AddString(_T("Hand"));
	m_combo7.AddString(_T("Thumb"));
	m_combo7.AddString(_T("Index"));
	m_combo7.AddString(_T("Middle"));
	m_combo7.AddString(_T("wrist"));
	m_combo7.AddString(_T("elbow"));
	m_combo7.AddString(_T("shoulder"));
	m_combo7.AddString(_T("Forearm"));
	m_combo7.AddString(_T("Upperarm"));
	m_combo7.AddString(_T("none"));
	m_combo7.SetCurSel(7);

	m_combo8.AddString(_T("Robot"));
	m_combo8.AddString(_T("Hand"));
	m_combo8.AddString(_T("Thumb"));
	m_combo8.AddString(_T("Index"));
	m_combo8.AddString(_T("Middle"));
	m_combo8.AddString(_T("wrist"));
	m_combo8.AddString(_T("elbow"));
	m_combo8.AddString(_T("shoulder"));
	m_combo8.AddString(_T("Forearm"));
	m_combo8.AddString(_T("Upperarm"));
	m_combo8.AddString(_T("none"));
	m_combo8.SetCurSel(10);


	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRobot_ReachingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRobot_ReachingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRobot_ReachingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



int CRobot_ReachingDlg::Initialize_locations()
{

	temploc = Robot->GetLocation("reach_eye.v3", "n_1");
	locations[0].x = temploc->Getx();
	locations[0].y = temploc->Gety();
	locations[0].z = temploc->Getz();
	locations[0].rotatex = temploc->Getxrot();
	locations[0].rotatey = temploc->Getyrot();
	
	temploc = Robot->GetLocation("reach_eye.v3", "n_2");
	locations[1].x = temploc->Getx();
	locations[1].y = temploc->Gety();
	locations[1].z = temploc->Getz();
	locations[1].rotatex = temploc->Getxrot();
	locations[1].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("reach_eye.v3", "n_3");
	locations[2].x = temploc->Getx();
	locations[2].y = temploc->Gety();
	locations[2].z = temploc->Getz();
	locations[2].rotatex = temploc->Getxrot();
	locations[2].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("reach_eye.v3", "n_4");
	locations[3].x = temploc->Getx();
	locations[3].y = temploc->Gety();
	locations[3].z = temploc->Getz();
	locations[3].rotatex = temploc->Getxrot();
	locations[3].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("reach_eye.v3", "n_5");
	locations[4].x = temploc->Getx();
	locations[4].y = temploc->Gety();
	locations[4].z = temploc->Getz();
	locations[4].rotatex = temploc->Getxrot();
	locations[4].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_6");
	locations[5].x = temploc->Getx();
	locations[5].y = temploc->Gety();
	locations[5].z = temploc->Getz();
	locations[5].rotatex = temploc->Getxrot();
	locations[5].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("reach_eye.v3", "n_7");
	locations[6].x = temploc->Getx();
	locations[6].y = temploc->Gety();
	locations[6].z = temploc->Getz();
	locations[6].rotatex = temploc->Getxrot();
	locations[6].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_8");
	locations[7].x = temploc->Getx();
	locations[7].y = temploc->Gety();
	locations[7].z = temploc->Getz();
	locations[7].rotatex = temploc->Getxrot();
	locations[7].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_9");
	locations[8].x = temploc->Getx();
	locations[8].y = temploc->Gety();
	locations[8].z = temploc->Getz();
	locations[8].rotatex = temploc->Getxrot();
	locations[8].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_10");
	locations[9].x = temploc->Getx();
	locations[9].y = temploc->Gety();
	locations[9].z = temploc->Getz();
	locations[9].rotatex = temploc->Getxrot();
	locations[9].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_11");
	locations[10].x = temploc->Getx();
	locations[10].y = temploc->Gety();
	locations[10].z = temploc->Getz();
	locations[10].rotatex = temploc->Getxrot();
	locations[10].rotatey = temploc->Getyrot();


	temploc = Robot->GetLocation("reach_eye.v3", "n_12");
	locations[11].x = temploc->Getx();
	locations[11].y = temploc->Gety();
	locations[11].z = temploc->Getz();
	locations[11].rotatex = temploc->Getxrot();
	locations[11].rotatey = temploc->Getyrot();



	/* initial position */
	temploc = Robot->GetLocation("reach_eye.v3", "initial3");
	locations[12].x= temploc->Getx(); 
	locations[12].y= temploc->Gety(); 
	locations[12].z= temploc->Getz(); 
	locations[12].rotatex= temploc->Getxrot(); 
	locations[12].rotatey= temploc->Getyrot(); 

	/* safety position */
	temploc = Robot->GetLocation("reach_eye.v3", "n_safe_l");
	locations[13].x= temploc->Getx(); 
	locations[13].y= temploc->Gety(); 
	locations[13].z= temploc->Getz(); 
	locations[13].rotatex= temploc->Getxrot(); 
	locations[13].rotatey= temploc->Getyrot(); 

	/* another safety position */
	temploc = Robot->GetLocation("reach_eye.v3", "n_safe_r");
	locations[14].x= temploc->Getx(); 
	locations[14].y= temploc->Gety(); 
	locations[14].z= temploc->Getz(); 
	locations[14].rotatex= temploc->Getxrot(); 
	locations[14].rotatey= temploc->Getyrot(); 

	return 1;
}



void CRobot_ReachingDlg::Randomize_trials()
{
	vector<int> Numbers;
	// Initialize vector Numbers
	for (int i=0; i < 2; i++)
		for (int j = 0; j < (trialNum[condition_type - 1] / 2); j++)
			Numbers.push_back(i);

	random_shuffle(Numbers.begin(),Numbers.end());
	for ( int i=0;i<trialNum[condition_type - 1];i++)
		current_trial[i]= (int) Numbers[i] ;

	CStdioFile fp1;
	CFileException fileException;
	CString tempstr_folder("");
	tempstr_folder.Format(_T("%s\\LogRobot.txt"),name);

	if ( !fp1.Open( tempstr_folder, CFile::modeCreate |   
		CFile::modeReadWrite, &fileException ) )
	{
		::AfxMessageBox(fileException.m_cause, 0, 0 );
	}

	/*Write coordinates and trial sequence in the file*/
	CString tempstr1("");
	for( int i =0; i<3; i++){
		tempstr1.Format(_T("Location %d: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f\n"), i+1,
		locations[i].x, locations[i].y, locations[i].z, locations[i].rotatex, locations[i].rotatey);
	SetDlgItemText(IDC_STATIC2, tempstr1 );
	fp1.WriteString( tempstr1 );
	}
	
	for ( int i=0;i<trialNum[condition_type - 1];i++){
		tempstr1.Format(_T("Trial %d: location %d\n"), i+1,	current_trial[i]+1);
		fp1.WriteString( tempstr1 );
	}
	fp1.Flush();
	fp1.Close();
}



void CRobot_ReachingDlg::RandomizeLeds()
{
	vector<int> Num;
	// Initialize vector Numbers
	for (int i=0; i < condNum[condition_type - 1]; i++)
	{
		for (int j = 0; j < (trialNum[condition_type - 1] / condNum[condition_type - 1]); j++)
		{
			Num.push_back(i);
		}
	}
	
	random_shuffle(Num.begin(),Num.end());
	for ( int i=0;i<trialNum[condition_type - 1];i++)
		current_leds[i]= (int) Num[i];

	CStdioFile fp1;
	CFileException fileException;
	CString tempstr_folder("");
	tempstr_folder.Format(_T("%s\\LedSequence.txt"),name);

	if ( !fp1.Open( tempstr_folder, CFile::modeCreate |   
		CFile::modeReadWrite, &fileException ) )
	{
		::AfxMessageBox(fileException.m_cause, 0, 0 );
	}

	/*Write coordinates and trial sequence in the file*/
	CString tempstr1("");
	//for( int i = 0; i < 3; i++){
	//	tempstr1.Format(_T("LED Sequence %d: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f\n"), i+1,
	//	locations[i].x, locations[i].y, locations[i].z, locations[i].rotatex, locations[i].rotatey);
	//SetDlgItemText(IDC_STATIC2, tempstr1 );
	//fp1.WriteString( tempstr1 );
	//}
	
	for ( int i=0;i<trialNum[condition_type - 1];i++){
		tempstr1.Format(_T("Trial %d: LED sequence %d\n"), i+1, current_leds[i]+1);
		SetDlgItemText(IDC_STATIC2, tempstr1 );
		fp1.WriteString( tempstr1 );
	}
	fp1.Flush();
	fp1.Close();
}





void CRobot_ReachingDlg::OnBnClickedOk()
{
	OnOK();
}

void CRobot_ReachingDlg::DoEvent(int type, int para)
{
	/*** Robot operations ***/
	//move to next location, play sound, record data until next trial is pressed again
	float x,y,z,xro,yro;

	int t;
	switch (type)
	{
	case 0:
		t = targetSeq[current_trial_counter][currentTarget] - 1;
		x = (float) locations[t].x;
		y = (float) locations[t].y;
		z = (float) locations[t].z;
		xro = (float) locations[t].rotatex;
		yro = (float) locations[t].rotatey;
		Robot->MoveTo_Actual_Point(x, y, z, xro, yro);
		currentTarget++;
		break;
	case 1:
		Robot->GPIO(15, para);
		break;
	case 2:
		Robot->GPIO(14, para);
		break;
}
}

/*************** Start Trial ****************/
void CRobot_ReachingDlg::OnBnClickedButton1()
{
	/*turn off 'Next Trial' and 'redo trial'*/
	CWnd * pbutton = GetDlgItem( IDC_BUTTON1);
	LARGE_INTEGER sample1, sample2, frequency;
	LONGLONG elapsedTime;
	pbutton->EnableWindow(false);
	pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(false);	


	if( radbut1.GetCheck() == BST_CHECKED)		//condition
		condition_type = 1;
	else if( radbut2.GetCheck() == BST_CHECKED)
		condition_type = 2;
	else if (radbut3.GetCheck() == BST_CHECKED)
		condition_type = 3;
	else 
	{
		condition_type = 4;
	}

	radbut1.EnableWindow( false );
	radbut2.EnableWindow( false );
	radbut3.EnableWindow( false );
	radbut4.EnableWindow( false );
	
	
	/*** Tracker operations: 
	 "name\name_conditiontype_currtrial_target#_rep#.dat ***/
	str.Format( _T("%s\\%s_cond_%d_%d_%d_%d.dat"), name, name, condition_type, current_trial_counter+1,
		current_trial[current_trial_counter]+1, repetitions[current_trial[current_trial_counter]] );
	SetDlgItemText(IDC_EDIT1, str );

	//flag = false;  //reset the falg for new recording

	CFileException fileException;
	if ( !fp.Open( str, CFile::modeCreate |   
		CFile::modeReadWrite, &fileException ) )
	{
		::AfxMessageBox(fileException.m_cause, 0, 0 );
	}

	//pdi.ClearBuffer(); //buffer goes back to the beginning
	//Sleep(100);		   //give some time to clear buffer
	
	/*** Header for the file ***/
	CString fstr ("info: 8\n");				//#of subseq. lines
	fp.WriteString( fstr );

	fstr.Format(_T("subject_name %s\n"),name);	//subj. name
	fp.WriteString( fstr );

	if( radbut1.GetCheck() == BST_CHECKED)		//condition
		fstr.Format(_T("condition 1\n"));
	else if( radbut2.GetCheck() == BST_CHECKED)
		fstr.Format(_T("condition 2\n"));
	else
		fstr.Format(_T("condition 3\n"));
	fp.WriteString( fstr );

	int nIndex = m_combo1.GetCurSel();				//classification
	CString tstr;
	m_combo1.GetLBText(nIndex, tstr);
	fstr.Format( _T("classification %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo2.GetCurSel();					//hand
	m_combo2.GetLBText(nIndex, tstr);
	fstr.Format( _T("hand %s\n"),tstr);   
	fp.WriteString(fstr);

	fstr.Format( _T("sample_rate %d\n"), FPS);   //frames per second
	fp.WriteString(fstr);

	CTime t = CTime::GetCurrentTime();			//date and time
	fstr = t.Format( "Date %m/%d/%y\nTime %H.%M.%S\n\n" );
	fp.WriteString(fstr);

	fstr.Format( _T("channels: %d\n"),sensors );
	fp.WriteString(fstr);

	nIndex = m_combo3.GetCurSel();					//channel 0
	m_combo3.GetLBText(nIndex, tstr);
	fstr.Format( _T("1 vector %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo4.GetCurSel();					//channel 1
	m_combo4.GetLBText(nIndex, tstr);
	fstr.Format( _T("2 vector %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo5.GetCurSel();					//channel 2
	m_combo5.GetLBText(nIndex, tstr);
	fstr.Format( _T("3 vector %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo6.GetCurSel();					//channel 3
	m_combo6.GetLBText(nIndex, tstr);
	fstr.Format( _T("4 vector %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo7.GetCurSel();					//channel 4
	m_combo7.GetLBText(nIndex, tstr);
	fstr.Format( _T("5 vector %s\n"),tstr);   
	fp.WriteString(fstr);

	nIndex = m_combo8.GetCurSel();					//channel 5
	m_combo8.GetLBText(nIndex, tstr);
	fstr.Format( _T("6 vector %s\n\n"),tstr);   
	fp.WriteString(fstr);

	fstr.Format( _T("connections: 5\n0 3\n1 3\n2 3\n3 4\n4 5\n\nsamples:\n"));   
	fp.WriteString(fstr);

	CString str(""); 
	/*call thread function*/
	//AfxBeginThread( threadFoo, 0, 0, 0, 0, NULL); 
	str += "Recording data...\n";
	SetDlgItemText(IDC_STATIC2, str );



	/*inc repetition for current location*/	
	repetitions[current_trial[current_trial_counter]]++;

	CString tempstr(""); 
//	tempstr.Format(_T("Trial #%d / %d \nMoving to: location #%d:\n x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
//		current_trial_counter+1,trialNum[condition_type - 1], current_trial[current_trial_counter]+1, x, y, z, xro, yro);
	SetDlgItemText(IDC_STATIC2, tempstr );
	

	QueryPerformanceCounter (&sample1);
	QueryPerformanceFrequency (&frequency);

	//::AfxMessageBox(tempstr, 0, 0 );

	if(condition_type==1)// remembered target, 100 ms target, no tone
	{
		for (int i=0; i<eventCount; ++i ) {
			if (eventTime[i] < 0.001)
			{
				DoEvent(eventType[i], eventPara[i]);
				continue;
			}
			do
			{
				QueryPerformanceCounter(&sample2);
				elapsedTime = (sample2.QuadPart - sample1.QuadPart) / frequency.QuadPart;
			}
			while (elapsedTime <= eventTime[i]);//Sleep(1000);
			DoEvent(eventType[i], eventPara[i]);
		}
		//hand LED on for 1 sec


		//if(current_trial[current_trial_counter]==2)// || current_trial[current_trial_counter]==3)
		//{
		//	//robot moves out of the way
		//	Robot->MoveTo_Actual_Point(locations[4].x, locations[4].y, locations[4].z, locations[4].rotatex, locations[4].rotatey);
		//}
		//else
		//{
		//	//robot moves out of the way
		//	Robot->MoveTo_Actual_Point(locations[3].x, locations[3].y, locations[3].z, locations[3].rotatex, locations[3].rotatey);
		//}

		//back to initial position
		//Sleep(TONE2END); 
		Robot->MoveTo_Actual_Point(locations[12].x, locations[12].y, locations[12].z, locations[12].rotatex, locations[12].rotatey);

		CString str("");
		str.Format(_T("\nBack to initial position:\nMoving to: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
			locations[12].x, locations[12].y, locations[12].z, locations[12].rotatex, locations[12].rotatey);
		tempstr += str;
		SetDlgItemText(IDC_STATIC2, tempstr );
		current_trial_counter++;
		currentTarget = 0;
	}
	else if (condition_type==2) 
	{
		switch (current_trial[current_trial_counter] + 1)
		{
		case 1:
			Robot->MoveTo_Actual_Point(locations[0].x, locations[0].y, locations[0].z, locations[0].rotatex, locations[0].rotatey);
			break;
		case 2:
			Robot->MoveTo_Actual_Point(locations[1].x, locations[1].y, locations[1].z, locations[1].rotatex, locations[1].rotatey);
			break;
		}
		switch (current_leds[current_trial_counter] + 1)
		{
		case 1: // if the current LED sequence == Condition 2.2
			Sleep(1000);
			Robot->GPIO(14, 1);
			Sleep(4900);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Sleep(5000);
			Robot->GPIO(14, 0);
			break;
		}
		Robot->MoveTo_Actual_Point(locations[2].x, locations[2].y, locations[2].z, locations[2].rotatex, locations[2].rotatey);

		CString str("");
		str.Format(_T("\n\nLED condition 2.2"));
		tempstr += str;
		SetDlgItemText(IDC_STATIC2, tempstr );
		current_trial_counter++;

	}
	else if (condition_type==3) // actual target
	{

		switch (current_trial[current_trial_counter] + 1)
		{
		case 1:
			Robot->MoveTo_Actual_Point(locations[0].x, locations[0].y, locations[0].z, locations[0].rotatex, locations[0].rotatey);
			break;
		case 2:
			Robot->MoveTo_Actual_Point(locations[1].x, locations[1].y, locations[1].z, locations[1].rotatex, locations[1].rotatey);
			break;
		}
		switch (current_leds[current_trial_counter] + 1)
		{
		case 1: // if the current LED sequence = condition 2.1
			str.Format(_T("\n\nLED condition 2.1"));
			tempstr += str;
			SetDlgItemText(IDC_STATIC2, tempstr );
			Sleep(1000);
			Robot->GPIO(14, 1);
			Sleep(9900);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Robot->GPIO(14, 0);
			break;
		case 2: // if the current LED sequence == condition 2.3
			str.Format(_T("\n\nLED condition 2.3"));
			tempstr += str;
			SetDlgItemText(IDC_STATIC2, tempstr );
			Sleep(1000);
			Robot->GPIO(14, 1);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Sleep(9900);
			Robot->GPIO(14, 0);
			break;
		}
		Robot->MoveTo_Actual_Point(locations[2].x, locations[2].y, locations[2].z, locations[2].rotatex, locations[2].rotatey);

		current_trial_counter++;
	}
	else if (condition_type == 4)
	{
		switch (current_trial[current_trial_counter] + 1)
		{
		case 1:
			Robot->MoveTo_Actual_Point(locations[0].x, locations[0].y, locations[0].z, locations[0].rotatex, locations[0].rotatey);
			break;
		case 2:
			Robot->MoveTo_Actual_Point(locations[1].x, locations[1].y, locations[1].z, locations[1].rotatex, locations[1].rotatey);
			break;
		}
		switch (current_leds[current_trial_counter] + 1)
		{
		case 1: // if the current LED sequence == #1
			str.Format(_T("\n\nLED condition 2.1"));
			tempstr += str;
			SetDlgItemText(IDC_STATIC2, tempstr );
			Sleep(1000);
			Robot->GPIO(14, 1);
			Sleep(9900);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Robot->GPIO(14, 0);
			break;
		case 2: // if the current LED sequence == #2
			str.Format(_T("\n\nLED condition 2.2"));
			tempstr += str;
			SetDlgItemText(IDC_STATIC2, tempstr );
			Sleep(1000);
			Robot->GPIO(14, 1);
			Sleep(4900);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Sleep(5000);
			Robot->GPIO(14, 0);
			break;
		case 3: // if the current LED sequence == #3
			str.Format(_T("\n\nLED condition 2.3"));
			tempstr += str;
			SetDlgItemText(IDC_STATIC2, tempstr );
			Sleep(1000);
			Robot->GPIO(14, 1);
			Robot->GPIO(15, 1);
			Sleep(100);
			Robot->GPIO(15, 0);
			Sleep(9900);
			Robot->GPIO(14, 0);
			break;
		}
		Robot->MoveTo_Actual_Point(locations[2].x, locations[2].y, locations[2].z, locations[2].rotatex, locations[2].rotatey);

		current_trial_counter++;

	}

	if(current_trial_counter>=MAXTRIALS){
		AfxMessageBox(_T("Done with this trial condition"), MB_OK, -1);
		radbut1.EnableWindow( true );
		radbut2.EnableWindow( true );
		radbut3.EnableWindow( true );
		current_trial_counter = 1;
	}

	/*turn on 'Stop Recording'*/
	//pbutton = GetDlgItem( IDC_BUTTON3);
	//pbutton->EnableWindow(true);
//}
/******** Stop Trial  ************/
//void CRobot_ReachingDlg::OnBnClickedButton3()
//{
	//pdi.ClearBuffer();	//clear buffer to start from new time
	//flag = true;  //set flag for no recording
	//Sleep(500);   //sleep for 100 ms so that all IO is done before closing the file
	fp.Flush();	  //flush just in case:-)
	fp.Close();   //close old file
	Sleep(500);   //sleep for 100 ms so that all IO is done before closing the file
	//pdi.ClearBuffer();	//clear buffer to start from new time

	CString endstr("");
	endstr.Format(_T("Stopped recording trial #%d / %d"),current_trial_counter, trialNum[condition_type - 1] );
	SetDlgItemText(IDC_STATIC2, endstr);

	/*turn on "Next Trial" and "redo trial", turn off "Stop Recording" button*/
	pbutton= GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(true);	
	pbutton->SetFocus();
	pbutton = GetDlgItem( IDC_BUTTON3);
	pbutton->EnableWindow(false);
	pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(true);	
}

/*********** Initialize ****************/
void CRobot_ReachingDlg::OnBnClickedButton2()
{

	/*check for condition*/
	if( radbut1.GetCheck() == BST_CHECKED)
		condition_type = 1;
	else if( radbut2.GetCheck() == BST_CHECKED)
		condition_type = 2;
	else if (radbut3.GetCheck() == BST_CHECKED)
		condition_type = 3;
	else if (radbut4.GetCheck() == BST_CHECKED)
		condition_type = 4;

	radbut1.EnableWindow( false );
	radbut2.EnableWindow( false );
	radbut3.EnableWindow( false );
	radbut4.EnableWindow( false );
	/*copy name of subject*/
	CWnd * pitem = GetDlgItem( IDC_EDIT1);
	str ="";
	GetDlgItemTextW(IDC_EDIT1, name);
	if( name.IsEmpty() ){
		AfxMessageBox(_T("Enter the subject name!"), MB_OK, -1);
		return;
	}
	/*CString ctype("");
	CString folderName("");
	GetDlgItemTextW(IDC_COMBO1, ctype);
	CreateDirectory( ctype, NULL ); 
	folderName.Format( _T("%s\\%s"), ctype, name);
	CreateDirectory( folderName, NULL ); 

	
	folderName.Format( _T("%s\\%s\\condition%d"),
		ctype, name,condition_type, name, condition_type, current_trial_counter);
		*/
	HANDLE hDir = NULL;
	//hDir = CreateFile ( folderName, GENERIC_READ, 
	hDir = CreateFile ( name, GENERIC_READ, 
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	/*if folder exists*/
	if( hDir != INVALID_HANDLE_VALUE ){
		if( IDNO == AfxMessageBox(_T
			("Folder exists!Do you want to overwrite files?"), MB_YESNO, -1)){
				return;}
	}

	
	/* initialize Lua */
	L = lua_open();

	/* load Lua base libraries */
	luaL_openlibs(L);

	/* run the script */
	luaL_dofile(L, ".//config.lua");



	/* Read the number of images */  
	int luaInd = 888;
	luaInd = lua_intexpr( L, "#config.eventType", &eventCount ) ; 

	for (int i=0; i<eventCount; ++i ) {
		char expr[64] = "" ;
		sprintf( expr, "config.eventTime[%d]", i+1 );
		lua_numberexpr( L, expr, &eventTime[i] );
		sprintf( expr, "config.eventType[%d]", i+1 );
		lua_intexpr( L, expr, &eventType[i] );
		sprintf( expr, "config.eventPara[%d]", i+1 );
		lua_intexpr( L, expr, &eventPara[i] );
	}

	lua_intexpr( L, "config.trialNum", &trialNum[condition_type - 1] );
	lua_intexpr( L, "config.withinTrialTargetNum", &withinTrialTargetNum );

	for (int i = 0; i < trialNum[condition_type - 1]; i++)
	{
		for (int j = 0; j < withinTrialTargetNum; j++)
		{
			char expr[64] = "" ;
			sprintf( expr, "config.targetSeq[%d]", i * withinTrialTargetNum + j + 1);
			lua_intexpr( L, expr, &targetSeq[i][j] );
		}
	}

	/* cleanup Lua */
	lua_close(L);

	
	pitem->EnableWindow(false);
	//CreateDirectory( folderName, NULL ); 
	CreateDirectory( name, NULL ); 
	//name = folderName;

	str.Format( _T("%s\\%s_cond_%d_%d_%d_%d.dat"), name, name, condition_type, current_trial_counter,
		current_trial[current_trial_counter], repetitions[current_trial[current_trial_counter]]);
	SetDlgItemText(IDC_EDIT1, str );

	/*turn on "Next Trial", turn off "Init Robot" button*/
	CWnd * pbutton = GetDlgItem( IDC_BUTTON2);
	pbutton->EnableWindow(false);	

	//initialized = true;		//set flag if connected to tracker

	CString tempstr,str("");
	tempstr.Format(_T("eventCount = %d\n"), eventCount);
//	tempstr.Format(_T("config.targetSeq[%d][%d] = %d\n"), 0, 1, targetSeq[0][1]);
	tempstr = _T("=>Time of events = ") + tempstr + 
		_T("\n\n=>Connecting to the Robot...\n");
	SetDlgItemText(IDC_STATIC2, tempstr );

	/*Connect to robot*/
	Robot = new Robot_Control();
	int x = Robot->Initialize();
	Initialize_locations();
	//Randomize_trials();
	RandomizeLeds();
	//Robot->Ready();		//move robot into Ready position
	
	/*move to initial position*/
	Robot->MoveTo_Actual_Point(locations[12].x, locations[12].y, locations[12].z, locations[12].rotatex, locations[12].rotatey);


	tempstr += "Connected to the Robot...\n";
	SetDlgItemText(IDC_STATIC2, tempstr );


	tempstr += "=>Connecting to the tracker...\n";
	SetDlgItemText(IDC_STATIC2, tempstr );

	//pdi.Connect(); //connect to the device
	//str.Format( _T("Connected to the tracker...\nTrial #%d\n"),current_trial_counter);
	//tempstr += str;
	//SetDlgItemText(IDC_STATIC2, tempstr );

	CString tempstr1(""), tstr("");
	tempstr1.Format(_T("Trial #%d / %d \nMoving to: location #%d:\n x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
		current_trial_counter+1, trialNum[condition_type - 1], current_trial[current_trial_counter]+1, locations[current_trial[current_trial_counter]].x, 
		locations[current_trial[current_trial_counter]].y, locations[current_trial[current_trial_counter]].z,
		locations[current_trial[current_trial_counter]].rotatex, locations[current_trial[current_trial_counter]].rotatey);
	tstr += tempstr1;
	tempstr1.Format(_T("\nSafe: \nMoving to: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
		 locations[12].x, locations[12].y, locations[12].z, locations[12].rotatex, locations[12].rotatey);
	tstr += tempstr1;
	SetDlgItemText(IDC_STATIC2, tstr );

	//pdi.StartCapture( BUFFER, BUFFER_SIZE); //initialize data buffer, ready to record data
	//pdi.ClearBuffer(); //buffer goes back to the beginning
	//sensors = pdi.GetSennum();   //get number of connected sensors

	/*
	pfin = pdi.GetCurBuf();
	pdi.GetCurFrmTim( pfin, 1, &newTime );
	for(int i=0;i<sensors;i++){
		pdi.GetCurData( pfin, i, curData );
		for(int j=0;j<DATA_LENGTH;j++){
			data[i].push_back(curData[j]); 
		}
	}
	*/

	/*turn on "Next Trial", turn off "Init Robot" button*/
	pbutton = GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(true);	

}


void CRobot_ReachingDlg::OnDestroy()
{
	/*release control and lock axes*/
	if( Robot ){
		delete Robot;
	}
	/*
	if ( initialized ){   //if user stoped recording => she was recording:-)
		pdi.ClearBuffer();	//clear buffer to start from new time
		pdi.StopCapture();
		pdi.Disconnect();
	}
	*/
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
}

/******* Redo Trial     ****************/
void CRobot_ReachingDlg::OnBnClickedButton4()
{
	CString tstr("");
	
	current_trial_counter--;   //dec counter
	repetitions[current_trial[current_trial_counter]]--;

	if (current_trial_counter < 0){
	current_trial_counter = 0;
	}
	tstr.Format( _T("%s\\%s_cond_%d_%d_%d_%d_bad.dat"), name, name, condition_type, current_trial_counter+1,
		current_trial[current_trial_counter]+1, repetitions[current_trial[current_trial_counter]]);
	str.Format( _T("%s\\%s_cond_%d_%d_%d_%d.dat"), name, name, condition_type, current_trial_counter+1,
		current_trial[current_trial_counter]+1, repetitions[current_trial[current_trial_counter]]);
		
	CWnd* pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(false);

	try
	{
		CFile::Rename( str, tstr );
		::AfxMessageBox( _T("Push \"Start recording\" to re-record trial"), 0, 0 );
		//CWnd* pbutton = GetDlgItem( IDC_BUTTON4);
		//pbutton->EnableWindow(false);

	}
	catch(CFileException* pEx ){
		::AfxMessageBox( _T("Unable to redo the last trial. "), 0, 0 );
		current_trial_counter++;   //inc counter
		repetitions[current_trial[current_trial_counter]]++;
		return;
	}

	SetDlgItemText(IDC_EDIT1, str );

	Sleep(100);		//give some time to copy file
}