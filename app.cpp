#include "stdafx.h"
#include "wnd.h"
#include "app.h"

CFractalApp::~CFractalApp(void)
{
	delete m_pMainWnd;
}

BOOL CFractalApp::InitInstance(void)
{
	CWinApp::InitInstance();
	m_pMainWnd = &m_Wnd;
	m_Wnd.CreateEx(0,
		AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW), 0, 0),
		_T("Fractal - Powered by Devymex"), WS_OVERLAPPEDWINDOW,
		CRect(0, 0, 350, 300), NULL, 0);
	m_Wnd.CenterWindow();
	m_Wnd.ShowWindow(SW_SHOW);
	m_Wnd.UpdateWindow();
	return TRUE;
}

BOOL CFractalApp::OnIdle(LONG lCount)
{
	((CFractalWnd*)m_pMainWnd)->UpdateImage();
	return CWinApp::OnIdle(lCount);
}
