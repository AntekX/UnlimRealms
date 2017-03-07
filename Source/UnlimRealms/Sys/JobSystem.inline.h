///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline ur_float Job::GetProgress() const
	{
		return this->progress;
	}

	inline Job::State Job::GetState() const
	{
		return this->state;
	}

	inline ur_bool Job::InProgress() const
	{
		return (State::InProgress == this->state);
	}

	inline ur_bool Job::Finished() const
	{
		return (State::Finished == this->state);
	}

	inline ur_bool Job::Interrupted() const
	{
		return this->interrupt;
	}

	inline Result::UID Job::GetResultCode() const
	{
		return this->resultCode;
	}

} // end namespace UnlimRealms