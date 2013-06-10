#pragma once

class CFractalApp : public CWinApp
{
public:
	~CFractalApp(void);
	virtual BOOL InitInstance(void);
	CFractalWnd m_Wnd;
	virtual BOOL OnIdle(LONG lCount);
} theApp;
