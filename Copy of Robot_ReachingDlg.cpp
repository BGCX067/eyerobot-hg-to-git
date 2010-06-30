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
//polhemus stuff...
#include "PDI.h"
#include "pdifunc.h"
#include <time.h>
#include <mmsystem.h>
#include <algorithm>
#include <vector>

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
#define WAIT 5000   //how long the robot stays at target
#define MOVWAIT 2000   //wait for the robot to finish moving
#define FPS 240

using namespace std;
PDIFunc pdi;//tracker
PBYTE pbeg, pcur, pfin;//tracker
CStdioFile fp;

std:: vector < double > data[DATA_LENGTH]; //data from tracker
float curData[DATA_LENGTH]; //data from 1 sensor
int newTime;
int oldTime = -1;
int sensors = 0;	    //number of sensors used
int counter = 1;		//trial counter;
int framec = 0;			//frame counter
bool flag = false;		//flag for recording state (i.e. recording if !flag)
bool initialized = false;	//flag for usage of tracker, false if tracker not used
CString str, name("");

UINT __cdecl threadFoo( LPVOID pParam );


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
	ON_BN_CLICKED(IDC_BUTTON3, &CRobot_ReachingDlg::OnBnClickedButton3)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON4, &CRobot_ReachingDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CRobot_ReachingDlg message handlers

BOOL CRobot_ReachingDlg::OnInitDialog()
{
	Robot = NULL;	

	current_trial_counter = 0;	//reset trial counter
	
	/*init repetition tracker*/
	for ( int i=0;i<LOCATIONNUM-1;i++){
		repetitions[i] = 0;
	}


	for ( int i=0;i<10;i++){
		for ( int j=0;j<10;j++){
			current_trial[10*i+j] = j;
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
	 //x:483.21, y:-123.98, z:206.79, xrot:-107.27, yrot:89.72
	//440.62, y:280.49, z:183.21, xrot:-91.47, yrot:-33.67
	temploc = Robot->GetLocation("eyetrack.v3", "t1");
	/*locations[0].x = (float)440.62;// temploc->Getx();
	locations[0].y = (float)280.49;//temploc->Gety();
	locations[0].z = (float)183.21;//temploc->Getz();
	locations[0].rotatex = (float)-90.47;//temploc->Getxrot();
	locations[0].rotatey = (float)-33.67;//temploc->Getyrot();
	*/
	locations[0].x = temploc->Getx();
	locations[0].y = temploc->Gety();
	locations[0].z = temploc->Getz();
	locations[0].rotatex = temploc->Getxrot();
	locations[0].rotatey = temploc->Getyrot();
	
	/*cant move to that loc, so add 2*/
	temploc = Robot->GetLocation("eyetrack.v3", "t2");
	locations[1].x = temploc->Getx();//+2;
	locations[1].y = temploc->Gety();//+2;
	locations[1].z = temploc->Getz();//+2;
	locations[1].rotatex = temploc->Getxrot();//+2;
	locations[1].rotatey = temploc->Getyrot();//+2;

	temploc = Robot->GetLocation("eyetrack.v3", "t3");
	locations[2].x = temploc->Getx();
	locations[2].y = temploc->Gety();
	locations[2].z = temploc->Getz();
	locations[2].rotatex = temploc->Getxrot();
	locations[2].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t4");
	locations[3].x = temploc->Getx();
	locations[3].y = temploc->Gety();
	locations[3].z = temploc->Getz();
	locations[3].rotatex = temploc->Getxrot();
	locations[3].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t5");
	locations[4].x = temploc->Getx();
	locations[4].y = temploc->Gety();
	locations[4].z = temploc->Getz();
	locations[4].rotatex = temploc->Getxrot();
	locations[4].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t6");
	locations[5].x = temploc->Getx();
	locations[5].y = temploc->Gety();
	locations[5].z = temploc->Getz();
	locations[5].rotatex = temploc->Getxrot();
	locations[5].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t7");
	locations[6].x = temploc->Getx();
	locations[6].y = temploc->Gety();
	locations[6].z = temploc->Getz();
	locations[6].rotatex = temploc->Getxrot();
	locations[6].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t8");
	locations[7].x = temploc->Getx();
	locations[7].y = temploc->Gety();
	locations[7].z = temploc->Getz();
	locations[7].rotatex = temploc->Getxrot();
	locations[7].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t9");
	locations[8].x = temploc->Getx();
	locations[8].y = temploc->Gety();
	locations[8].z = temploc->Getz();
	locations[8].rotatex = temploc->Getxrot();
	locations[8].rotatey = temploc->Getyrot();

	temploc = Robot->GetLocation("eyetrack.v3", "t10");
	locations[9].x = temploc->Getx();
	locations[9].y = temploc->Gety();
	locations[9].z = temploc->Getz();
	locations[9].rotatex = temploc->Getxrot();
	locations[9].rotatey = temploc->Getyrot();

	/* initial position */
	temploc = Robot->GetLocation("eyetrack.v3", "initial");
	locations[10].x= temploc->Getx(); //(float) 285;//180;
	locations[10].y= temploc->Gety(); //(float) 0.0;//400;
	locations[10].z= temploc->Getz(); //(float) 308;//CENTERZ + 70;
	locations[10].rotatex= temploc->Getxrot(); //(float) 0.0;
	locations[10].rotatey= temploc->Getyrot(); //(float) 0.0;//45.0;

	/* safety position */
	locations[11].x=(float) 180;
	locations[11].y=(float) 400;
	locations[11].z=(float) CENTERZ + 70;
	locations[11].rotatex=(float) 0.0;
	locations[11].rotatey=(float) 45.0;

	/*for( int i=1; i<7; i++){
	locations[i].x = locations[0].x;
	locations[i].y = locations[0].y;
	locations[i].z = locations[0].z;
	locations[i].rotatex = 0;
	locations[i].rotatey = 0;
	}*/

	
	return 1;
}



void CRobot_ReachingDlg::Randomize_trials()
{
	vector<int> Numbers(MAXTRIALS);

	// Initialize vector Numbers
	for ( int i=0; i<MAXTRIALS; i++)
		Numbers[i] = current_trial[i];
	
	random_shuffle(Numbers.begin(),Numbers.end());
	for ( int i=0;i<MAXTRIALS;i++)
		current_trial[i]= (int) Numbers[i] ;

	CStdioFile fp1;
	CFileException fileException;
	if ( !fp1.Open( _T("LogRobot.txt"), CFile::modeCreate |   
		CFile::modeReadWrite, &fileException ) )
	{
		::AfxMessageBox(fileException.m_cause, 0, 0 );
	}

	/*Write coordinates and trial sequence in the file*/
	CString tempstr1("");
	for( int i =0; i<11; i++){
		tempstr1.Format(_T("Location %d: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f\n"), i+1,
		locations[i].x, locations[i].y, locations[i].z, locations[i].rotatex, locations[i].rotatey);
	SetDlgItemText(IDC_STATIC2, tempstr1 );
	fp1.WriteString( tempstr1 );
	}
	
	for ( int i=0;i<MAXTRIALS;i++){
		tempstr1.Format(_T("Trial %d: location %d\n"), i+1,	current_trial[i]+1);
		fp1.WriteString( tempstr1 );
	}
	fp1.Flush();
	fp1.Close();
}


void CRobot_ReachingDlg::OnBnClickedOk()
{
	OnOK();
}

/*************** Start Trial ****************/
void CRobot_ReachingDlg::OnBnClickedButton1()
{
	/*turn off 'Next Trial' and 'redo trial'*/
	CWnd * pbutton = GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(false);
	pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(false);	


	if( radbut1.GetCheck() == BST_CHECKED)		//condition
		condition_type = 1;
	else if( radbut2.GetCheck() == BST_CHECKED)
		condition_type = 2;
	else condition_type = 3;

	radbut1.EnableWindow( false );
	radbut2.EnableWindow( false );
	radbut3.EnableWindow( false );
	
	
	/*** Tracker operations: 
	 "name\name_conditiontype_currtrial_target#_rep#.dat ***/
	str.Format( _T("%s\\%s_cond_%d_%d_%d_%d.dat"), name, name, condition_type, current_trial_counter,
		current_trial[current_trial_counter], repetitions[current_trial[current_trial_counter]] );
	SetDlgItemText(IDC_EDIT1, str );

	flag = false;  //reset the falg for new recording

	CFileException fileException;
	if ( !fp.Open( str, CFile::modeCreate |   
		CFile::modeReadWrite, &fileException ) )
	{
		::AfxMessageBox(fileException.m_cause, 0, 0 );
	}

	pdi.ClearBuffer(); //buffer goes back to the beginning
	Sleep(100);		   //give some time to clear buffer
	
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
	AfxBeginThread( threadFoo, 0, 0, 0, 0, NULL); 
	str += "Recording data...\n";
	SetDlgItemText(IDC_STATIC2, str );

	/*** Robot operations ***/
	//move to next location, play sound, record data until next trial is pressed again
	float x = (float) locations[current_trial[current_trial_counter]].x;
	float y = (float) locations[current_trial[current_trial_counter]].y;
	float z = (float) locations[current_trial[current_trial_counter]].z;
	float xro = (float) locations[current_trial[current_trial_counter]].rotatex;
	float yro = (float) locations[current_trial[current_trial_counter]].rotatey;

	/*inc repetition for current location*/	
	repetitions[current_trial[current_trial_counter]]++;

	CString tempstr(""); 
	tempstr.Format(_T("Trial #%d \nMoving to: location #%d:\n x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
		current_trial_counter,current_trial[current_trial_counter], x, y, z, xro, yro);
	SetDlgItemText(IDC_STATIC2, tempstr );
	
	//::AfxMessageBox(tempstr, 0, 0 );

	if(condition_type==1)
	{
		//turn on all led's  move then wait 1.5s, tone collect data
		Robot->GPIO(13, 1);
		//Robot->GPIO(14, 1);

		//Robot->MoveTo_Actual_Point(400, y, 300, xro, yro);
		Robot->MoveTo_Actual_Point(x, y, z, xro, yro);
		current_trial_counter++;

		//wait 
		Sleep(WAIT);

		//tone
		PlaySound( _T("slow.wav"), NULL, SND_ASYNC | SND_FILENAME );


		CString str("");
		str.Format(_T("\nBack to initial position:\nMoving to: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
			locations[10].x, locations[10].y, locations[10].z, locations[10].rotatex, locations[10].rotatey);
		tempstr += str;
		SetDlgItemText(IDC_STATIC2, tempstr );

		int response = AfxMessageBox(_T("Hit button to move back to safe position"), MB_OK, -1);

		if ( response == IDOK ){
			//lights out
			Robot->GPIO(13, 0);
			//Robot->GPIO(14, 0);
			//move out of way
			Robot->MoveTo_Actual_Point(locations[10].x, locations[10].y, locations[10].z, locations[10].rotatex, locations[10].rotatey);

			//Wait till robot finishes moving
			Sleep( MOVWAIT );

		}

	}
	else // if (condition_type==2 or 3)
	{
		//turn on robot led's off subject, then off after 1.5s
		//robot and move out of way, collect data and tone
		Robot->GPIO(13, 1);
		//Robot->GPIO(14, 1);

		Robot->MoveTo_Actual_Point(x, y, z, xro, yro);
		current_trial_counter++;

		Sleep(WAIT);	//wait 1.5s

		//lights out
		Robot->GPIO(13, 0);
		//Robot->GPIO(14, 0);

		CString str("");
		str.Format(_T("\nBack to safety position:\nMoving to: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
			locations[11].x, locations[11].y, locations[11].z, locations[11].rotatex, locations[11].rotatey);
		tempstr += str;
		SetDlgItemText(IDC_STATIC2, tempstr );

		//move out of way
		Robot->MoveTo_Actual_Point(locations[11].x, locations[11].y, locations[11].z, locations[11].rotatex, locations[11].rotatey);

		//Wait till robot finishes moving
		Sleep( MOVWAIT );
		//tone
		PlaySound( _T("slow.wav"), NULL, SND_ASYNC | SND_FILENAME );

	}

	if(current_trial_counter>=MAXTRIALS){
		AfxMessageBox(_T("Done with this trial condition"), MB_OK, -1);
		radbut1.EnableWindow( true );
		radbut2.EnableWindow( true );
		radbut3.EnableWindow( true );
		current_trial_counter = 1;
	}

	/*turn on 'Stop Recording'*/
	pbutton = GetDlgItem( IDC_BUTTON3);
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
	else condition_type = 3;

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


	initialized = true;		//set flag if connected to tracker

	CString tempstr(""),str("");
	tempstr = "=>Connecting to the Robot...\n";
	SetDlgItemText(IDC_STATIC2, tempstr );

	/*Connect to robot*/
	Robot = new Robot_Control();
	int x = Robot->Initialize();
	Initialize_locations();
	Randomize_trials();
	//Robot->Ready();		//move robot into Ready position
	
	/*move to initial position*/
	Robot->MoveTo_Actual_Point(locations[10].x, locations[10].y, locations[10].z, locations[10].rotatex, locations[10].rotatey);


	tempstr += "Connected to the Robot...\n";
	SetDlgItemText(IDC_STATIC2, tempstr );


	tempstr += "=>Connecting to the tracker...\n";
	SetDlgItemText(IDC_STATIC2, tempstr );

	pdi.Connect(); //connect to the device
	str.Format( _T("Connected to the tracker...\nTrial #%d\n"),current_trial_counter);
	tempstr += str;
	SetDlgItemText(IDC_STATIC2, tempstr );

	CString tempstr1(""), tstr("");
	tempstr1.Format(_T("Trial #%d \nMoving to: location #%d:\n x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
		current_trial_counter, current_trial[current_trial_counter], locations[current_trial[current_trial_counter]].x, 
		locations[current_trial[current_trial_counter]].y, locations[current_trial[current_trial_counter]].z,
		locations[current_trial[current_trial_counter]].rotatex, locations[current_trial[current_trial_counter]].rotatey);
	tstr += tempstr1;
	tempstr1.Format(_T("\nSafe: \nMoving to: x:%.2f, y:%.2f, z:%.2f, xrot:%.2f, yrot:%.2f"),
		 locations[10].x, locations[10].y, locations[10].z, locations[10].rotatex, locations[10].rotatey);
	tstr += tempstr1;
	SetDlgItemText(IDC_STATIC2, tstr );

	pdi.StartCapture( BUFFER, BUFFER_SIZE); //initialize data buffer, ready to record data
	pdi.ClearBuffer(); //buffer goes back to the beginning
	sensors = pdi.GetSennum();   //get number of connected sensors

	pfin = pdi.GetCurBuf();
	pdi.GetCurFrmTim( pfin, 1, &newTime );
	for(int i=0;i<sensors;i++){
		pdi.GetCurData( pfin, i, curData );
		for(int j=0;j<DATA_LENGTH;j++){
			data[i].push_back(curData[j]); 
		}
	}

	/*turn on "Next Trial", turn off "Init Robot" button*/
	pbutton = GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(true);	

}

/******** Stop Trial  ************/
void CRobot_ReachingDlg::OnBnClickedButton3()
{
	pdi.ClearBuffer();	//clear buffer to start from new time
	flag = true;  //set flag for no recording
	Sleep(500);   //sleep for 100 ms so that all IO is done before closing the file
	fp.Flush();	  //flush just in case:-)
	fp.Close();   //close old file
	Sleep(500);   //sleep for 100 ms so that all IO is done before closing the file
	pdi.ClearBuffer();	//clear buffer to start from new time

	SetDlgItemText(IDC_STATIC2, _T("Stopped recording") );

	/*turn on "Next Trial" and "redo trial", turn off "Stop Recording" button*/
	CWnd * pbutton = GetDlgItem( IDC_BUTTON1);
	pbutton->EnableWindow(true);	
	pbutton = GetDlgItem( IDC_BUTTON3);
	pbutton->EnableWindow(false);
	pbutton = GetDlgItem( IDC_BUTTON4);
	pbutton->EnableWindow(true);	
}

void CRobot_ReachingDlg::OnDestroy()
{
	/*release control and lock axes*/
	if( Robot ){
		delete Robot;
	}
	if ( initialized ){   //if user stoped recording => she was recording:-)
		pdi.ClearBuffer();	//clear buffer to start from new time
		pdi.StopCapture();
		pdi.Disconnect();
	}

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
	tstr.Format( _T("%s\\%s_cond_%d_%d_%d_%d_bad.dat"), name, name, condition_type, current_trial_counter,
		current_trial[current_trial_counter], repetitions[current_trial[current_trial_counter]]);
	str.Format( _T("%s\\%s_cond_%d_%d_%d_%d.dat"), name, name, condition_type, current_trial_counter,
		current_trial[current_trial_counter], repetitions[current_trial[current_trial_counter]]);
		
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


//get all data from all sensors
void getFrametoFile(CStdioFile *fp){

	CString work = NULL;
	for(int i=0;i<sensors;i++){
		work.Format( _T("%d %d %d %f %f %f %f %f %f %f %f %f %f %f %f\n"),framec, newTime, i, data[i][0], data[i][1], data[i][2],
			data[i][3], data[i][4], data[i][5],data[i][6],data[i][7],data[i][8],data[i][9],data[i][10],data[i][11]);//rotation of sensor
		fp->WriteString(work);
		fp->Flush();
	}
	framec++;
}

UINT __cdecl threadFoo( LPVOID pParam ){
	
	//pdi.ClearBuffer(); //buffer goes back to the beginning
	while( !flag ){
		pfin = pdi.GetCurBuf();
		pdi.GetCurFrmTim( pfin, 1, 0, newTime );
			for(int i=0;i<sensors;i++){
			pdi.GetCurData( pfin, i, curData );
			for(int j=0;j<DATA_LENGTH;j++){
				data[i][j] = curData[j]; 
			}

		}
		
		if (newTime != oldTime){   //check if this frame had been recorded
			getFrametoFile( &fp);
		}
		oldTime = newTime;
	}
	AfxEndThread(0);	//Terminate the thread
	return 0;
}





	/*for(int i = 0; i<7; i++){
	locations[i].x=(float) CENTERX + i*20;
	locations[i].y=(float) CENTERY + i*20;
	locations[i].z=(float) CENTERZ + i*10;
	locations[i].rotatex=(float) 0.0;
	locations[i].rotatey=(float) 45.0;
	}*/

/*	locations[0].x=(float) CENTERX;
	locations[0].y=(float) CENTERY + YOFFSET;
	locations[0].z=(float) CENTERZ + ZOFFSET;
	locations[0].rotatex=(float) 0.0;
	locations[0].rotatey=(float) 45.0;

	locations[1].x=(float) CENTERX;
	locations[1].y=(float) (CENTERY-250) + YOFFSET;
	locations[1].z=(float) CENTERZ + ZOFFSET;
	locations[1].rotatex=(float) 0.0;
	locations[1].rotatey=(float) 45.0;

	locations[2].x=(float) CENTERX;
	locations[2].y=(float) (CENTERY+250) + YOFFSET;
	locations[2].z=(float) (CENTERZ) + ZOFFSET;
	locations[2].rotatex=(float) 0.0;
	locations[2].rotatey=(float) 45.0;

	locations[3].x=(float) CENTERX;
	locations[3].y=(float) (CENTERY) + YOFFSET;
	locations[3].z=(float) (CENTERZ+250) + ZOFFSET;
	locations[3].rotatex=(float) 0.0;
	locations[3].rotatey=(float) 60.0;

	locations[4].x=(float) (CENTERX);
	locations[4].y=(float) (CENTERY) + YOFFSET;
	locations[4].z=(float) CENTERZ - 250.0 + ZOFFSET;
	locations[4].rotatex=(float) 0.0;
	locations[4].rotatey=(float) 45.0;

	locations[5].x=(float) CENTERX - 6.25;
	locations[5].y=(float) CENTERY + YOFFSET;
	locations[5].z=(float) CENTERZ + ZOFFSET;
	locations[5].rotatex=(float) 0.0;
	locations[5].rotatey=(float) 45.0;
	*/