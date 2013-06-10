#pragma once

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
