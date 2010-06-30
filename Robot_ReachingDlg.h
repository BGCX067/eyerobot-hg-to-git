// Robot_ReachingDlg.h : header file
//

#include "Robot_Control/Robot_Control.h"
#pragma once#include "afxwin.h"

#define MAXTRIALS 90
#define LOCATIONNUM 4


// CRobot_ReachingDlg dialog
class CRobot_ReachingDlg : public CDialog
{
// Construction
public:
	CRobot_ReachingDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_ROBOT_REACHING_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

//Class vars
public:
	int Initialize_locations(void);
	void Randomize_trials( void );
	void RandomizeLeds( void );
	int current_trial[MAXTRIALS];
	int current_leds[MAXTRIALS];
//	int trialNum[4] = {30, 30, 60, 90};
	int current_trial_counter;
	int condition_type;
	typedef struct
	{
		float x;
		float y;
		float z;
		float rotatex;
		float rotatey;
	} RLOCATION;

	RLOCATION locations[LOCATIONNUM+1];
	int repetitions[LOCATIONNUM-1];	//array to keep track of repetitions of locations
	Robot_Control* Robot;
	ICRSLocationPtr temploc;


public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedButton1();
public:
	afx_msg void OnBnClickedButton2();

public:
	CButton radbut1;
	CButton radbut2;
	CButton radbut3;
	CButton radbut4;
//public:
//	afx_msg void OnBnClickedButton3();
public:
	afx_msg void OnDestroy();
public:
	CComboBox m_combo1;
public:
	CComboBox m_combo2;
public:
	CComboBox m_combo3;
public:
	CComboBox m_combo4;
public:
	CComboBox m_combo5;
public:
	CComboBox m_combo6;
public:
	CComboBox m_combo7;
public:
	CComboBox m_combo8;
public:
	afx_msg void OnBnClickedButton4();
};
