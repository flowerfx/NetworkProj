/*
chien todo: (chien.truong@gogame.net)
*/
#ifndef __NETWORK_MGR_H__
#define __NETWORK_MGR_H__

#include <stack>
#include <thread>
#include <chrono>

#include "platform/CCPlatformMacros.h"
#include "base/CCRef.h"
#include "base/CCVector.h"
#include "math/CCMath.h"


NS_CC_BEGIN

/**
 * @addtogroup base
 * @{
 */

/* Forward declarations. */

class DirectorDelegate;
class Scheduler;
class Console;
class SocketClient;
class NetHttpClient;
class CC_DLL NetworkMgr : public Ref
{
public:


    
    /** 
     * @js _getInstance
     */
    static NetworkMgr* getInstance();

    /**
     * @js ctor
     */
	NetworkMgr();
    virtual ~NetworkMgr();
    bool init();

    /** Whether or not the Director is paused. */
    bool isPaused() { return _paused; }
    void end();
    void pause();
    void resume();
    void restart();

    void setDefaultValues();


    void mainLoop(float dt);

   /*
   * Gets socket client 
   */
	SocketClient * getSocketClient() const { return _socket_client; }

	/*
	* Gets http client
	*/

	NetHttpClient * getHttpClient() const { return _http_client;  }

    /** Gets the Scheduler associated with this director.
     * @since v2.0
     */
    Scheduler* getScheduler() const { return _scheduler; }
    
    void setScheduler(Scheduler* scheduler);

    Console* getConsole() const { return _console; }
    

    const std::thread::id& getMainThreadId() const { return _cocos2d_thread_id; }

    /**
     * returns whether or not the Director is in a valid state
     */
    bool isValid() const { return !_invalid; }

protected:
    void reset();
  
    /** Scheduler associated with this 
     @since v2.0
     */
    Scheduler *_scheduler;
    
    /** Whether or not the Director is paused */
    bool _paused;

    /* Console for the director */
    Console *_console;

    /* cocos2d thread id */
    std::thread::id _cocos2d_thread_id;

    /* whether or not the director is in a valid state */
    bool _invalid;


	//socket client
	SocketClient * _socket_client;

	//http client
	NetHttpClient * _http_client;
};

// end of base group
/** @} */

NS_CC_END

#endif // __CCDIRECTOR_H__
#define NetMgr netlib::NetworkMgr::getInstance()
