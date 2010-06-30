// Robot_Reaching.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CRobot_ReachingApp:
// See Robot_Reaching.cpp for the implementation of this class
//

class CRobot_ReachingApp : public CWinApp
{
public:
	CRobot_ReachingApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRobot_ReachingApp theApp;