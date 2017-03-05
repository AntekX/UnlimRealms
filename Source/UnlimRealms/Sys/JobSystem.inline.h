///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline void* Job::GetData() const
	{
		return this->data;
	}

	inline void Job::Terminate()
	{
		this->terminate = true;
	}

	inline bool Job::Terminate() const
	{
		return this->terminate;
	}

	inline bool Job::Finished() const
	{
		return this->finished;
	}

	inline void Job::SetProgress(float progress)
	{
		this->progress = progress;
	}

	inline float Job::GetProgress() const
	{
		return this->progress;
	}

	inline void Job::SetResultCode(Result::UID resultCode)
	{
		this->resultCode = resultCode;
	}

	inline Result::UID Job::GetResultCode() const
	{
		return this->resultCode;
	}

} // end namespace UnlimRealms