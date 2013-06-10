#pragma once

struct MYTHREADINFO
{
	DWORD dwId;
	HANDLE hThread;
	CSemaphore *pSyncProc;
	CSemaphore *pSyncCtrl;
	BOOL bFlag;
};

struct SEGMENT
{
	USHORT usBeg;
	USHORT usEnd;
};

class CFractalWnd : public CWnd
{
public:
	CFractalWnd();
	~CFractalWnd();
	void ThreadDispatch();
	void UpdateImage();

protected:
	CDC m_MemDC;
	CBitmap m_MemBmp;
	CFont m_Font;
	UINT m_nMaxRep;
	double m_dRange;
	double m_dBegX;
	double m_dBegY;
	double m_dScale;
	double m_dDragX;
	double m_dDragY;
	DWORD m_dwCpuCnt;
	CPoint m_DragPos;
	LPVOID m_pBits;
	CSyncObject **m_ppSyncProc;
	CSyncObject **m_ppSyncCtrl;
	BITMAPINFOHEADER m_Bih;
	CCriticalSection m_cs;
	std::vector<MYTHREADINFO> m_Threads;
	std::list<SEGMENT> m_ValidSegs;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//afx_msg void OnDisplayChange(WPARAM wParam, LPARAM lParam);

	void ScaleImage(POINT pt, double dMul);
	void ScanLine(UINT nLine);
	BOOL GetNewLine(std::list<SEGMENT>::iterator &out);
	COLORREF CalcPixColor(POINT &pt) const;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
