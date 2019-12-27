///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"
#include "Core/BaseTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base operation result type
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	struct UR_DECL Result
	{
	public:

		typedef ur_uint UID;

		UID Code;

		Result();
		
		Result(UID code);

		static UID GenerateUID();

		bool operator == (const UID &code) const { return (this->Code == code); }

		bool operator != (const UID &code) const { return (this->Code != code); }

		Result operator &= (const Result &r) const;

		Result operator && (const Result &r) const;

		operator bool() const;

	private:

		static UID lastUID;
	};

	#define DECLARE_RESULT_CODE(resultName) extern UR_DECL Result::UID resultName;
	#define DEFINE_RESULT_CODE(resultName) Result::UID resultName = Result::GenerateUID();


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Common result types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_RESULT_CODE(Undefined);

	DECLARE_RESULT_CODE(Failure);

	DECLARE_RESULT_CODE(Success);

	DECLARE_RESULT_CODE(NotImplemented);

	DECLARE_RESULT_CODE(NotInitialized);

	DECLARE_RESULT_CODE(NotFound);

	DECLARE_RESULT_CODE(InvalidArgs);

	DECLARE_RESULT_CODE(OutOfMemory);


	// helper shortcuts
	#define Succeeded(result) (Success == (result).Code)
	#define Failed(result) (Success != (result).Code)
	#define CombinedResult(res0, res1) (Failed(res0) ? Result(res0) : (Failed(res1) ? Result(res1) : Result(Success)))

} // end namespace UnlimRealms