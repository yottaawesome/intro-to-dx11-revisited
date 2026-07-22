export module shared:gametimer;
import std;
import :win32;

export class GameTimer
{
public:
	GameTimer()
	{
		auto countsPerSec = std::uint64_t{};
		Win32::QueryPerformanceFrequency(reinterpret_cast<Win32::LARGE_INTEGER*>(&countsPerSec));
		mSecondsPerCount = 1.0 / (double)countsPerSec;
	}

	auto TotalTime()const->float  // in seconds
	{
		// If we are stopped, do not count the time that has passed since we stopped.
		// Moreover, if we previously already had a pause, the distance 
		// mStopTime - mBaseTime includes paused time, which we do not want to count.
		// To correct this, we can subtract the paused time from mStopTime:  
		//
		//                     |<--paused time-->|
		// ----*---------------*-----------------*------------*------------*------> time
		//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

		if (mStopped)
		{
			return static_cast<float>(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
		}

		// The distance mCurrTime - mBaseTime includes paused time,
		// which we do not want to count.  To correct this, we can subtract 
		// the paused time from mCurrTime:  
		//
		//  (mCurrTime - mPausedTime) - mBaseTime 
		//
		//                     |<--paused time-->|
		// ----*---------------*-----------------*------------*------> time
		//  mBaseTime       mStopTime        startTime     mCurrTime

		else
		{
			return static_cast<float>(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
		}
	}

	auto DeltaTime()const->float // in seconds
	{
		return static_cast<float>(mDeltaTime);
	}

	void Reset() // Call before message loop.
	{
		auto currTime = std::uint64_t{};
		Win32::QueryPerformanceCounter(reinterpret_cast<Win32::LARGE_INTEGER*>(&currTime));

		mBaseTime = currTime;
		mPrevTime = currTime;
		mStopTime = 0;
		mStopped = false;
	}

	void Start() // Call when unpaused.
	{
		auto startTime = std::uint64_t{};
		Win32::QueryPerformanceCounter(reinterpret_cast<Win32::LARGE_INTEGER*>(&startTime));

		// Accumulate the time elapsed between stop and start pairs.
		//
		//                     |<-------d------->|
		// ----*---------------*-----------------*------------> time
		//  mBaseTime       mStopTime        startTime     

		if (mStopped)
		{
			mPausedTime += (startTime - mStopTime);

			mPrevTime = startTime;
			mStopTime = 0;
			mStopped = false;
		}
	}

	void Stop()  // Call when paused.
	{
		if (mStopped)
			return;
		auto currTime = std::uint64_t{};
		Win32::QueryPerformanceCounter(reinterpret_cast<Win32::LARGE_INTEGER*>(&currTime));

		mStopTime = currTime;
		mStopped = true;
	}

	void Tick()  // Call every frame.
	{
		if (mStopped)
		{
			mDeltaTime = 0.0;
			return;
		}

		auto currTime = std::uint64_t{};
		Win32::QueryPerformanceCounter(reinterpret_cast<Win32::LARGE_INTEGER*>(&currTime));
		mCurrTime = currTime;

		// Time difference between this frame and the previous.
		mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

		// Prepare for next frame.
		mPrevTime = mCurrTime;

		// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
		// processor goes into a power save mode or we get shuffled to another
		// processor, then mDeltaTime can be negative.
		if (mDeltaTime < 0.0)
		{
			mDeltaTime = 0.0;
		}
	}
private:
	double mSecondsPerCount = 0;
	double mDeltaTime = -1;

	std::uint64_t mBaseTime = 0;
	std::uint64_t mPausedTime = 0;
	std::uint64_t mStopTime = 0;
	std::uint64_t mPrevTime = 0;
	std::uint64_t mCurrTime = 0;

	bool mStopped = false;
};