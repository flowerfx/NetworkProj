
#ifndef __DEFINE_MARCO_H__
#define __DEFINE_MARCO_H__

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <assert.h>
#include "Windows.h"

namespace App
{
	enum class TYPE_DEBUG
	{
		T_ERROR = 0,
		T_WARNING ,
		T_OUTPUT,
		T_PRINT,
		T_OUTPUT_PRINT
	};
	std::string format(const char* format, ...);
	void PrintOut(TYPE_DEBUG type, const char * str, ...);
	void Assert(bool isCorrect, const char * msg, ...);

	typedef		char				s8;
	typedef		unsigned char		u8;
	typedef		short				s16;
	typedef		unsigned short		u16;
	typedef		int					s32;
	typedef		unsigned int		u32;
	typedef		long long			s64;
	typedef		unsigned long long	u64;

	typedef		unsigned char		BYTE;
	typedef		unsigned long 		DWORD;

	typedef		unsigned int		_ssize_t;

	// an original Abstract class that all class must inherritence
	class Core
	{
	public:
		Core() { /* nothing here */}
		virtual ~Core() { /* nothing here */}
	};

	//handle timer
	u64 GetTicks();
	u64 GetTicksPerSecond();
	void OffsetTimer(u64 ticks);
	inline double GetSecondsDouble(u64 offset = 0) {
		return double(GetTicks() - offset) / double(GetTicksPerSecond());
	}
	u64 GetMilliseconds();
	u64 GetMicroseconds();
	u64 GetNanoseconds();


	#define SYNC_VALUE(name, type)	private: type name; \
								public:  type get##name() { return name ;} \
										 void set##name(type d) { name = d ;}
									
	#define ASSERT App::Assert

	#define ERROR_OUT(msg, ...) App::PrintOut(App::TYPE_DEBUG::T_ERROR , msg ,__VA_ARGS__)
	#define WARN_OUT(msg, ...) App::PrintOut(App::TYPE_DEBUG::T_WARNING , msg ,__VA_ARGS__)
	#define PRINT_OUT(msg, ...) App::PrintOut(App::TYPE_DEBUG::T_PRINT , msg ,__VA_ARGS__)
	#define OUTPUT_OUT(msg, ...) App::PrintOut(App::TYPE_DEBUG::T_OUTPUT, msg ,__VA_ARGS__)
	#define OUTPUT_PRINT_OUT(msg, ...) App::PrintOut(App::TYPE_DEBUG::T_OUTPUT_PRINT , msg ,__VA_ARGS__ )

	#define HRESULT App::s8
	#define S_OK 0
	#define S_FAILED -1
}

#endif // __DEFINE_MARCO_H__