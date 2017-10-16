/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2013 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2017 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

// game includes
#include "NetworkMgr.h"

// standard includes
#include <string>


#include "base/CCScheduler.h"
#include "base/ccMacros.h"
#include "base/CCConsole.h"
#include "base/CCAutoreleasePool.h"
#include "base/CCConfiguration.h"
#include "base/CCAsyncTaskPool.h"

//
#include "SocketClient.h"
#include "NetHttpClient.h"

using namespace std;

NS_CC_BEGIN
// FIXME: it should be a Director ivar. Move it there once support for multiple directors is added

// singleton stuff
static NetworkMgr *s_SharedDirector = nullptr;

CC_DLL const char* netlibVersion()
{
	return "netlib 1.0.0a";
}


#define kDefaultFPS        60  // 60 frames per second

NetworkMgr* NetworkMgr::getInstance()
{
    if (!s_SharedDirector)
    {
        s_SharedDirector = new (std::nothrow) NetworkMgr;
        CCASSERT(s_SharedDirector, "FATAL: Not enough memory");
        s_SharedDirector->init();
    }

    return s_SharedDirector;
}

NetworkMgr::NetworkMgr():
  _invalid(true)
, _socket_client(nullptr)
, _console(nullptr)
, _scheduler(nullptr)
{
}

bool NetworkMgr::init(void)
{
    setDefaultValues();

    // paused ?
    _paused = false;
    
    // invalid ?
    _invalid = false;

    //console
    _console = new (std::nothrow) Console;

    // scheduler
    _scheduler = new (std::nothrow) Scheduler();

	// socket client
	_socket_client = new(std::nothrow) SocketClient();

	// http client
	_http_client = new (std::nothrow) NetHttpClient();

    return true;
}

NetworkMgr::~NetworkMgr(void)
{
    CCLOGINFO("deallocing Director: %p", this);


    CC_SAFE_RELEASE(_scheduler);
	CC_SAFE_DELETE(_socket_client);
	CC_SAFE_DELETE(_console);
	CC_SAFE_DELETE(_http_client);


    Configuration::destroyInstance();

    s_SharedDirector = nullptr;
}

void NetworkMgr::setDefaultValues(void)
{
    
}





void NetworkMgr::reset()
{
	// cleanup scheduler
	getScheduler()->unscheduleAll();
}




void NetworkMgr::pause()
{
	if (_paused)
	{
		return;
	}

    _paused = true;
}

void NetworkMgr::resume()
{
    if (! _paused)
    {
        return;
    }

    _paused = false;
}

void NetworkMgr::setScheduler(Scheduler* scheduler)
{
    if (_scheduler != scheduler)
    {
        CC_SAFE_RETAIN(scheduler);
        CC_SAFE_RELEASE(_scheduler);
        _scheduler = scheduler;
    }
}

void NetworkMgr::mainLoop(float dt)
{
	if (! _invalid)
    {
		//update the socket
		if (_socket_client)
		{
			_socket_client->onUpdate(dt);
		}

		//update http client
		if (_http_client)
		{
			_http_client->onUpdate(dt);
		}

		//update the scheduler
		_scheduler->update(dt);

        // release the objects
        PoolManager::getInstance()->getCurrentPool()->clear();
    }
}


NS_CC_END
