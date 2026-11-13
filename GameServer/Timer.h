// 게임 타이머

#pragma once

const ULONG MAX_SAMPLE_COUNT = 50;

// 고해상도 카운터로 프레임 간 시간과 FPS를 잰다.
// GetTimeElapsed()는 최근 프레임 시간들의 평균값(최대 50개)을 돌려준다.
class CGameTimer
{
public:
	CGameTimer();
	virtual ~CGameTimer();

	void Tick(float fLockFPS = 0.0f);
	void Start();
	void Stop();
	void Reset();

	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int nCharacters = 0);
	float GetTimeElapsed();
	float GetTotalTime();

private:
	double							m_fTimeScale;
	float							m_fTimeElapsed;

	__int64							m_nBasePerformanceCounter;
	__int64							m_nPausedPerformanceCounter;
	__int64							m_nStopPerformanceCounter;
	__int64							m_nCurrentPerformanceCounter;
	__int64							m_nLastPerformanceCounter;

	__int64							m_PerformanceFrequencyPerSec;

	float							m_fFrameTime[MAX_SAMPLE_COUNT];
	ULONG							m_nSampleCount;

	unsigned long					m_nCurrentFrameRate;
	unsigned long					m_FramePerSecond;
	float							m_fFPSTimeElapsed;

	bool							m_bStopped;
};
