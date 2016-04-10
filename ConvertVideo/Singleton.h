#pragma once

#pragma warning(disable:4356)

namespace zen
{

	template<typename T>
	class Singleton
	{
	public:
		static void createInstance()
		{
			ZEN_ASSERT(!mpInstance, L"already created.\n");
			mpInstance = new T;
		}

		static void destoryInstance()
		{
			SAFE_DELETE(mpInstance);
		}

		static T* getInstance()
		{
			return mpInstance;
		}

	protected:
		static T* mpInstance;

		Singleton() {};
		virtual ~Singleton() {};
	};

}

#define ZEN_DECLARE_SINGLETON(type_)\
	friend class Singleton<type_>

#define ZEN_IMPLEMENT_SINGLETON(type_)\
	type_* type_::mpInstance = nullptr