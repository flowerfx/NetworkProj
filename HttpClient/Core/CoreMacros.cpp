// stdafx.cpp : source file that includes just the standard includes
// HttpClient.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <mutex>
#include <iostream>
#include "CoreMacros.h"
namespace App
{
	/*
		1: Blue
		2 : Green
		3 : Cyan
		4 : Red
		5 : Purple
		6 : Yellow(Dark)
		7 : Default white
		8 : Gray / Grey
		9 : Bright blue
		10 : Brigth green
		11 : Bright cyan
		12 : Bright red
		13 : Pink / Magenta
		14 : Yellow
		15 : Bright white
	*/
#define GREEN		1
#define RED			4
#define YELLOW		14
#define BRIGHT		15
#define PINK		13
#define GREEN		10
	std::mutex& get_cout_mutex()
	{
		static std::mutex m;
		return m;
	}

	std::string format(const char* format, ...)
	{
#define MAX_STRING_LENGTH (1024*100)

		std::string ret;

		va_list ap;
		va_start(ap, format);

		char* buf = (char*)malloc(MAX_STRING_LENGTH);
		if (buf != nullptr)
		{
			vsnprintf(buf, MAX_STRING_LENGTH, format, ap);
			ret = buf;
			free(buf);
		}
		va_end(ap);

		return ret;
	}

	void PrintOut(App::TYPE_DEBUG type, const char * strformat, ...)
	{
	
		va_list args;
		va_start(args, strformat);
		std::string str;
		char* buf = (char*)malloc(MAX_STRING_LENGTH);
		if (buf != nullptr)
		{
			vsnprintf(buf, MAX_STRING_LENGTH, strformat, args);
			str = buf;
			free(buf);
		}
		va_end(args);
		switch (type)
		{
		case App::TYPE_DEBUG::T_ERROR:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
			str.insert(0, "ERROR: ");
			OutputDebugStringA(str.c_str());
			std::cout << str.c_str();
			break;
		case App::TYPE_DEBUG::T_WARNING:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), YELLOW);
			str.insert(0, "WARNING: ");
			OutputDebugStringA(str.c_str());
			std::cout << str.c_str();
			break;
		case App::TYPE_DEBUG::T_OUTPUT:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BRIGHT);
			//str.insert(0, "WARNING: ");
			OutputDebugStringA(str.c_str());
			//printf_s(str.c_str());
			break;
		case App::TYPE_DEBUG::T_PRINT:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), PINK);
			//str.insert(0, "WARNING: ");
			//OutputDebugStringA(str.c_str);
			std::cout << str.c_str();
			break;
		case App::TYPE_DEBUG::T_OUTPUT_PRINT:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), GREEN);
			str.insert(0, "OUTPUT: ");
			OutputDebugStringA(str.c_str());
			std::cout<<str.c_str();
			break;
		default:
			break;
		}
		std::cout<<std::endl;
	}

	void Assert(bool isCorrect, const char * msg, ...)
	{
		if (isCorrect == false)
		{
			assert(0);
			va_list args;
			va_start(args, msg);
			PrintOut(TYPE_DEBUG::T_ERROR, msg, args);
			va_end(args);
		}
	}

	u64 GetTicks() {
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);

		return ticks.QuadPart;
	}

	u64 GetTicksPerSecond() { //maybe cache this value !
		LARGE_INTEGER tickPerSeconds;
		QueryPerformanceFrequency(&tickPerSeconds);

		return tickPerSeconds.QuadPart;
	}

	u64 sTimerTickOffset = 0;

	void OffsetTimer(u64 ticks) {
		sTimerTickOffset += ticks;
	}

	u64 GetMilliseconds() {
		return u64(GetSecondsDouble() * 1000);
	}

	u64 GetMicroseconds() {
		return u64(GetSecondsDouble() * 1000 * 1000);
	}
	u64 GetNanoseconds() {
		return u64(GetSecondsDouble() * 1000 * 1000 * 1000);
	}
}