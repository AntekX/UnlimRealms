///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// COM helpers
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	template <class T>
	class shared_ref
	{
	private:

		T * ref;

	public:

		explicit shared_ref(T *p = ur_null) : ref(ur_null)
		{
			this->reset(p);
		}

		shared_ref(shared_ref<T> &r) : ref(ur_null)
		{
			this->reset(r.ref);
		}

		template<class B>
		shared_ref(shared_ref<B>& r) : ref(ur_null)
		{
			this->reset((T*)r.ref);
		}

		~shared_ref()
		{
			this->reset();
		}

		T* get() const
		{
			return this->ref;
		}

		void reset(T *p = ur_null)
		{
			if (this->ref != ur_null)
			{
				this->ref->Release();
				this->ref = ur_null;
			}
			if (p != ur_null)
			{
				this->ref = p;
				this->ref->AddRef();
			}
		}

		bool empty() const
		{
			return (ur_null == this->ref);
		}

		template<class B>
		operator shared_ref<B>()
		{
			return shared_ref<B>(*this);
		}

		template<class B>
		shared_ref<T>& operator=(shared_ref<B>& r)
		{
			this->reset((T*)r.get());
			return *this;
		}

		shared_ref<T>& operator=(shared_ref<T>& r)
		{
			this->reset(r.get());
			return *this;
		}

		shared_ref<T>& operator=(T* p)
		{
			this->reset(p);
			return *this;
		}

		T& operator*() const
		{
			return *this->ref;
		}

		T* operator->() const
		{
			return this->ref;
		}

		operator T*() const
		{
			return (T*)this->ref;
		}

		operator T**() const
		{
			return (T**)&this->ref;
		}

		template <class B>
		operator B**() const
		{
			return (B**)&this->ref;
		}

		operator void**() const
		{
			return (void**)&this->ref;
		}
	};

} // end namespace UnlimRealms