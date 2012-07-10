#define _SECURE_ATL 1
#define VC_EXTRALEAN
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600 
#define _WIN32_WINDOWS 0x0410
#define _WIN32_IE 0x0700
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS

#include <afx.h>
#include <afxwin.h>
#include <afxext.h>
#include <list>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>

class CTimer
{
public:
    __forceinline CTimer()
	{
		QueryPerformanceFrequency((PLARGE_INTEGER)&m_nFreq);
        QueryPerformanceCounter((PLARGE_INTEGER)&m_nStart);
    }
    __forceinline double Stop()
	{
		__int64 nCur;
        QueryPerformanceCounter((PLARGE_INTEGER)&nCur);
		double dr = double(nCur - m_nStart) / double(m_nFreq);
		m_nStart = nCur;
        return dr;
    }
private:
    __int64 m_nFreq;
    __int64 m_nStart;
};

template<typename _Ty>
void bound_to(_Ty &v, const _Ty &Min, const _Ty &Max)
{
	if (v < Min)
	{
		v = Min;
	}
	else if (v > Max)
	{
		v = Max;
	}
}

struct SEGMENT
{
	USHORT usBeg;
	USHORT usEnd;
};

bool GreaterSeg(SEGMENT &s1, SEGMENT &s2)
{
	return (s1.usEnd - s1.usBeg < s2.usEnd - s2.usBeg);
}

struct MYTHREADINFO
{
	DWORD dwId;
	HANDLE hThread;
	HANDLE hEvent;
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
	CPoint m_DragPos;
	LPVOID m_pBits;
	BITMAPINFOHEADER m_Bih;
	CRITICAL_SECTION m_cs;
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

DWORD CALLBACK ThreadProc(LPVOID lpParam)
{
	((CFractalWnd*)lpParam)->ThreadDispatch();
	return 0;
}

class CFractalApp : public CWinApp
{
public:
	~CFractalApp(void);
	virtual BOOL InitInstance(void);
	CFractalWnd m_Wnd;
	virtual BOOL OnIdle(LONG lCount);
} theApp;

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

BEGIN_MESSAGE_MAP(CFractalWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()

CFractalWnd::CFractalWnd()
: m_dBegX(-2.2)
, m_dBegY(-1.1)
, m_dScale(120.0)
, m_dDragX(0.0)
, m_dDragY(0.0)
, m_nMaxRep(150)
, m_dRange(1e3)
, m_pBits(NULL)
{
}

CFractalWnd::~CFractalWnd()
{
	for (std::vector<MYTHREADINFO>::iterator i = m_Threads.begin();
		i != m_Threads.end(); ++i)
	{
		TerminateThread(i->hThread, 0);
		CloseHandle(i->hThread);
		CloseHandle(i->hEvent);
	}
	VirtualFree(m_pBits, 0, MEM_RELEASE);
}

BOOL CFractalWnd::GetNewLine(std::list<SEGMENT>::iterator &out)
{
	BOOL br = FALSE;
	if (!m_ValidSegs.empty())
	{
		//从任务段中找出最大的一段。
		std::list<SEGMENT>::iterator i = std::max_element(
			m_ValidSegs.begin(), m_ValidSegs.end(), GreaterSeg);
		USHORT usDist = i->usEnd - i->usBeg;
		//如果该段至少还有两行，则一分为二，将后者作为新段插入
		if (usDist > 1)
		{
			SEGMENT NewSeg = {(usDist) / 2 + i->usBeg, i->usEnd};
			i->usEnd = NewSeg.usBeg;
			out = m_ValidSegs.insert(++i, NewSeg);
			br = true;
		}
	}
	return br;
}

//所有的线程一起维护一个m_ValidSegs列表，该列表存储尚未计算的行段(可用段)。
//每一个元素为该段的起始行(x)与该段的结束行的下一行(y)。
//若m_ValidSegs里存储{{1,50}, {101, 200}}，表示当前图像中
//第1行到第49行与第101行到第199行尚未计算。
//每个线程进入循环后，每次先按照规则选择要处理的行号，计算后再行选择。
//选择的规则是：如果先前已计算的一行的下一行可用，则直接计算下一行；
//否则查找行数最多的可用段，将该段中间一分为二，选后面一段的第一行
void CFractalWnd::ThreadDispatch()
{
	MYTHREADINFO mti;
	for (std::vector<MYTHREADINFO>::const_iterator i = m_Threads.begin();
		i != m_Threads.end(); ++i)
	{
		if (i->dwId == GetCurrentThreadId())
		{
			mti = *i;
			break;
		}
	}
	SetEvent(mti.hEvent);

	for (; ; )
	{
		SuspendThread(mti.hThread);
		for (std::list<SEGMENT>::iterator i; ;)
		{
			//进入临界区，使多线程互斥的访问可用段列表
			EnterCriticalSection(&m_cs);

			//如果可用段为空，则全部处理完毕，退出线程
			if (m_ValidSegs.empty())
			{
				LeaveCriticalSection(&m_cs);
				break;
			}

			//如果是新图，则直接开始处理整段
			if (m_ValidSegs.front().usBeg == 0)
			{
				i = m_ValidSegs.begin();
			} //非新图，但线程第一次进入或线程原任务已结束时，分配新任务
			else if (i._Ptr == NULL || i->usBeg >= i->usEnd)
			{
				// 线程任务结束时删除原任务
				if (i._Ptr != NULL)
				{
					m_ValidSegs.erase(i);
				}
				//分配新任务
				if (!GetNewLine(i))
				{
					LeaveCriticalSection(&m_cs);
					break;
				}
			}

			//完成当前任务，并将支配的段的起点下移
			int nLine = i->usBeg++;
			LeaveCriticalSection(&m_cs);
			ScanLine(nLine);
		}
		SetEvent(mti.hEvent);
	}
}

void CFractalWnd::OnPaint(void)
{
	// 将当前位图绘置到屏幕上
	CRect rect;
	GetClientRect(rect);
	PAINTSTRUCT ps;
	CDC *pDC = BeginPaint(&ps);
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &m_MemDC, 0, 0, SRCCOPY);
	CRgn rgn;
	rgn.CreateRectRgnIndirect(rect);
	pDC->SelectClipRgn(&rgn);
	EndPaint(&ps);
	CWnd::OnPaint();
}

BOOL CFractalWnd::OnEraseBkgnd(CDC* pDC)
{
	UNREFERENCED_PARAMETER(pDC); 
	return TRUE;
}

void CFractalWnd::OnDestroy()
{
	CWnd::OnDestroy();
}

void CFractalWnd::ScaleImage(POINT pt, double dMul)
{
	double dNewScale = m_dScale * dMul;
	dNewScale = dNewScale > 1e30 ? 1e30 : dNewScale;
	dNewScale = dNewScale < 100 ? 100 : dNewScale;
	dMul = dNewScale / m_dScale;
	double dPosX = pt.x / m_dScale + m_dBegX;
	double dPosY = pt.y / m_dScale + m_dBegY;
	m_dBegX = dPosX - (dPosX - m_dBegX) / dMul;
	m_dBegY = dPosY - (dPosY - m_dBegY) / dMul;
	m_dScale = dNewScale;
}

__inline COLORREF SHL2RGB(double s, double h, double l)
{
	COLORREF clr = 0;
	h *= 6.0;
	double c = (1.0 - abs(2.0 * l - 1.0)) * s;
	double x = c * (1.0 - abs(h - int(h / 2.0) * 2 - 1.0));
	double t = (l - c / 2.0) * 255.0;
	c = c * 255.0 + t;
	x = x * 255.0 + t;
	switch (int(h))
	{
	case 0: clr = RGB(int(c), int(x), int(t)); break;
	case 1: clr = RGB(int(x), int(c), int(t)); break;
	case 2: clr = RGB(int(t), int(c), int(x)); break;
	case 3: clr = RGB(int(t), int(x), int(c)); break;
	case 4: clr = RGB(int(x), int(t), int(c)); break;
	case 5: clr = RGB(int(c), int(t), int(x)); break;
	}
	return clr;
}


UINT Mandelbrot(double Coord[2], double dRange, UINT nMaxRep)
{
	double dX = Coord[0], dY = Coord[1];
	double dX2, dY2;
	UINT nRep = 0;
	for (nRep = 0; nRep < nMaxRep; ++nRep)
	{
		dX2 = dX * dX;
		dY2 = dY * dY;
		if (dX2 + dY2 > dRange)
		{
			break;
		}
		dY = 2 * dX * dY + Coord[1];
		dX = dX2 - dY2 + Coord[0];
	}
	Coord[0] = dX2;
	Coord[1] = dY2;
	return nRep;
}

//
COLORREF CFractalWnd::CalcPixColor(POINT &pt) const
{

	double dXY[2] =
	{
		(pt.x + 0.5) / m_dScale + m_dBegX,
		(pt.y + 0.5) / m_dScale + m_dBegY
	};
	UINT nRep = Mandelbrot(dXY, m_dRange, m_nMaxRep);

	COLORREF clr = 0;
	if (nRep < m_nMaxRep)
	{
		double dClr = -1;
		double a = log(dXY[0] + dXY[1]) / log(m_dRange);
		dClr = (nRep - log(a)) / m_nMaxRep;
		clr = SHL2RGB(1.0f, float(dClr), 0.5f);
	}
	return clr;
}

void CFractalWnd::UpdateImage()
{
	if (NULL == m_MemDC.GetSafeHdc() || m_Bih.biHeight == 0 ||
		this != GetForegroundWindow())
	{
		return;
	}

	SEGMENT seg = {0, (USHORT)m_Bih.biHeight};
	m_ValidSegs.clear();
	m_ValidSegs.push_back(seg);


	CTimer Timer;
	std::vector<HANDLE> Events;
	for (std::vector<MYTHREADINFO>::iterator i = m_Threads.begin();
		i != m_Threads.end(); ++i)
	{
		ResumeThread(i->hThread);
		Events.push_back(i->hEvent);
	}
	WaitForMultipleObjects(Events.size(), &Events[0], TRUE, INFINITE);
	double dFr = 1.0 / Timer.Stop();

	BITMAPINFO bi = {0};
	bi.bmiHeader = m_Bih;
	SetDIBits(m_MemDC, m_MemBmp, 0, m_Bih.biHeight, m_pBits, &bi, 0);
	CString strMsg;
	strMsg.Format(_T("m=%d, p=%e, f=%f\nl=%f, r=%f, t=%f, b=%f"),
		m_nMaxRep, m_dRange, dFr,
		m_dBegX, m_Bih.biWidth / m_dScale + m_dBegX,
		-m_dBegY, -m_Bih.biHeight / m_dScale + m_dBegY);

	CRect rect;
	GetClientRect(rect);

	m_MemDC.DrawText(strMsg, -1, rect, DT_LEFT | DT_TOP);
	Invalidate(FALSE);
}

void CFractalWnd::ScanLine(UINT nLine)
{
	char *pLine = (char*)m_pBits + (m_Bih.biHeight - nLine - 1) * m_Bih.biSizeImage;
	POINT pt = {0, nLine};
	for (; pt.x < m_Bih.biWidth; ++pt.x)
	{
		COLORREF clr = 0;
		clr = CalcPixColor(pt);
		*(COLORREF*)pLine = clr;
		pLine += m_Bih.biBitCount / 8;
	}
}

void CFractalWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_DragPos = point;
	m_dDragX = m_dBegX;
	m_dDragY = m_dBegY;
	SetCapture();
	CWnd::OnLButtonDown(nFlags, point);
}

void CFractalWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	CWnd::OnLButtonUp(nFlags, point);
}

void CFractalWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (*this == *GetCapture() && (nFlags & MK_LBUTTON))
	{
		m_dBegX = m_dDragX + (m_DragPos.x - point.x) / m_dScale;
		m_dBegY = m_dDragY + (m_DragPos.y - point.y) / m_dScale;
	}
	CWnd::OnMouseMove(nFlags, point);
}

BOOL CFractalWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	zDelta /= WHEEL_DELTA;
	if (nFlags & MK_CONTROL)
	{
		//按住Ctrl键滚动滚轮改变迭代次数
		int nMaxRep = m_nMaxRep;
		nMaxRep += zDelta;
		bound_to(nMaxRep, 2, 500);
		m_nMaxRep = nMaxRep;
	}
	else if (nFlags & MK_SHIFT)
	{
		//按住Shift键滚动滚轮改变收敛范围
		m_dRange += (m_dRange / 15) * zDelta;
		bound_to(m_dRange, 2.0, 1e100);
	}
	else
	{
		double dScale = 1.05;
		dScale = (zDelta < 0 ? 1 / dScale : dScale);
		ScreenToClient(&pt);
		for (int i = 0; i < abs(zDelta); ++i)
		{
			ScaleImage(pt, dScale);
		}
	}
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CFractalWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	CRect rect;
	GetClientRect(rect);
	m_Bih.biWidth = rect.Width();
	m_Bih.biHeight = rect.Height();

	//利用m_Bih.biSizeImage存储位图的数据长度，此数值对齐到4字节边界
	m_Bih.biSizeImage = m_Bih.biWidth * (m_Bih.biBitCount / 8);
	m_Bih.biSizeImage = ((m_Bih.biSizeImage + 3) / 4) * 4;

	VirtualFree(m_pBits, 0, MEM_RELEASE);
	m_pBits = VirtualAlloc(NULL, m_Bih.biSizeImage * m_Bih.biHeight + 4,
		MEM_COMMIT, PAGE_READWRITE);
}

int CFractalWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CWnd::OnCreate(lpCreateStruct);

	InitializeCriticalSection(&m_cs);

	//创建与CPU总数相同数量的绘图线程及各自的同步事件，并加入数组
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	si.dwNumberOfProcessors = 2;
	for (DWORD i = 0; i < si.dwNumberOfProcessors; ++i)
	{
		MYTHREADINFO mti;
		mti.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

		//创建线程后挂起，避免线程较早的访问尚未初始化的mti而导致出错
		mti.hThread = CreateThread(NULL, 0, ThreadProc,
			this, CREATE_SUSPENDED, &mti.dwId);
		m_Threads.push_back(mti);

		//mti初始化后运行线程，并等待线程初始化完毕
		ResumeThread(mti.hThread);
		WaitForSingleObject(mti.hEvent, INFINITE);
	}

	//初始化图像基本信息
	ZeroMemory(&m_Bih, sizeof(m_Bih));
	m_Bih.biSize = sizeof(m_Bih);
	m_Bih.biPlanes = 1;
	m_Bih.biBitCount = 24;
	m_Bih.biCompression = BI_RGB;

	//初始化绘图对象
	CDC *pDC = GetDC();
	m_MemDC.CreateCompatibleDC(pDC);
	m_MemBmp.CreateCompatibleBitmap(pDC,
		GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	ReleaseDC(pDC);
	m_MemDC.SelectObject(&m_MemBmp);
	m_MemDC.SelectObject(GetStockObject(DEFAULT_GUI_FONT));
	m_MemDC.SetTextColor(RGB(255, 255, 255));
	m_MemDC.SetBkMode(TRANSPARENT);
	return 0;
}

void CFractalWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	//避免按Alt键失去焦点
	if (nID == SC_KEYMENU)
	{
		return;
	}
	CWnd::OnSysCommand(nID, lParam);
}

BOOL CFractalWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_DISPLAYCHANGE)
	{
		pMsg->message = 0;
	}
	return CWnd::PreTranslateMessage(pMsg);
}
