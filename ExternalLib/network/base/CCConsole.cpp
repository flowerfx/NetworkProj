/****************************************************************************
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

#include "base/CCConsole.h"

#include <thread>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <WS2tcpip.h>
#include <Winsock2.h>
#if defined(__MINGW32__)
#include "platform/win32/inet_pton_mingw.h"
#endif
#define bzero(a, b) memset(a, 0, b);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
#include "platform/winrt/inet_ntop_winrt.h"
#include "platform/winrt/inet_pton_winrt.h"
#include "platform/winrt/CCWinRTUtils.h"
#endif
#else
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#endif

#include "NetworkMgr.h"
#include "base/CCScheduler.h"
#include "platform/CCPlatformConfig.h"
#include "base/CCConfiguration.h"
//#include "2d/CCScene.h"
#include "platform/CCFileUtils.h"
//#include "renderer/CCTextureCache.h"
#include "base/base64.h"
#include "base/ccUtils.h"
#include "base/allocator/CCAllocatorDiagnostics.h"
NS_CC_BEGIN

extern const char* netlibVersion(void);

#define PROMPT  "> "

static const size_t SEND_BUFSIZ = 512;

/** private functions */
namespace {
#if defined(__MINGW32__)
    // inet
    const char* inet_ntop(int af, const void* src, char* dst, int cnt)
    {
        struct sockaddr_in srcaddr;
        
        memset(&srcaddr, 0, sizeof(struct sockaddr_in));
        memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
        
        srcaddr.sin_family = af;
        if (WSAAddressToStringA((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0)
        {
            return nullptr;
        }
        return dst;
    }
#endif
    
    //
    // Free functions to log
    //
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    void SendLogToWindow(const char *log)
    {
        //static const int CCLOG_STRING_TAG = 1;
        //// Send data as a message
        //COPYDATASTRUCT myCDS;
        //myCDS.dwData = CCLOG_STRING_TAG;
        //myCDS.cbData = (DWORD)strlen(log) + 1;
        //myCDS.lpData = (PVOID)log;
        //if (NetMgr->getOpenGLView())
        //{
        //    HWND hwnd = NetMgr->getOpenGLView()->getWin32Window();
        //    SendMessage(hwnd,
        //                WM_COPYDATA,
        //                (WPARAM)(HWND)hwnd,
        //                (LPARAM)(LPVOID)&myCDS);
        //}
    }
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
    void SendLogToWindow(const char *log)
    {
    }
#endif
    
    void _log(const char *format, va_list args)
    {
        int bufferSize = MAX_LOG_LENGTH;
        char* buf = nullptr;
        
        do
        {
            buf = new (std::nothrow) char[bufferSize];
            if (buf == nullptr)
                return; // not enough memory
            
            int ret = vsnprintf(buf, bufferSize - 3, format, args);
            if (ret < 0)
            {
                bufferSize *= 2;
                
                delete [] buf;
            }
            else
                break;
            
        } while (true);
        
        strcat(buf, "\n");
        
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        __android_log_print(ANDROID_LOG_DEBUG, "cocos2d-x debug info", "%s", buf);
        
#elif CC_TARGET_PLATFORM ==  CC_PLATFORM_WIN32 || CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
        
        int pos = 0;
        int len = strlen(buf);
        char tempBuf[MAX_LOG_LENGTH + 1] = { 0 };
        WCHAR wszBuf[MAX_LOG_LENGTH + 1] = { 0 };
        
        do
        {
            std::copy(buf + pos, buf + pos + MAX_LOG_LENGTH, tempBuf);
            
            tempBuf[MAX_LOG_LENGTH] = 0;
            
            MultiByteToWideChar(CP_UTF8, 0, tempBuf, -1, wszBuf, sizeof(wszBuf));
            OutputDebugStringW(wszBuf);
            WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, tempBuf, sizeof(tempBuf), nullptr, FALSE);
            printf("%s", tempBuf);
            
            pos += MAX_LOG_LENGTH;
            
        } while (pos < len);
        SendLogToWindow(buf);
        fflush(stdout);
#else
        // Linux, Mac, iOS, etc
        fprintf(stdout, "%s", buf);
        fflush(stdout);
#endif
        
        NetMgr->getConsole()->log(buf);
        delete [] buf;
    }
}

// FIXME: Deprecated
void CCLog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    _log(format, args);
    va_end(args);
}

void log(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    _log(format, args);
    va_end(args);
}

//
//  Utility code
//

std::string Console::Utility::_prompt(PROMPT);

//TODO: these general utils should be in a separate class
//
// Trimming functions were taken from: http://stackoverflow.com/a/217605
//
// trim from start

std::string& Console::Utility::ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string& Console::Utility::rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string& Console::Utility::trim(std::string& s) {
    return Console::Utility::ltrim(Console::Utility::rtrim(s));
}

std::vector<std::string>& Console::Utility::split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> Console::Utility::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    Console::Utility::split(s, delim, elems);
    return elems;
}

//isFloat taken from http://stackoverflow.com/questions/447206/c-isfloat-function
bool Console::Utility::isFloat(const std::string& myString) {
    std::istringstream iss(myString);
    float f;
    iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
}

ssize_t Console::Utility::sendToConsole(int fd, const void* buffer, size_t length, int flags)
{
    if (_prompt.length() == length) {
        if (strncmp(_prompt.c_str(), static_cast<const char*>(buffer), length) == 0) {
            fprintf(stderr,"bad parameter error: a buffer is the prompt string.\n");
            return 0;
        }
    }
    
    const char* buf = static_cast<const char*>(buffer);
    ssize_t retLen = 0;
    for (size_t i = 0; i < length; ) {
        size_t len = length - i;
        if (SEND_BUFSIZ < len) len = SEND_BUFSIZ;
        retLen += send(fd, buf + i, len, flags);
        i += len;
    }
    return retLen;
}

// dprintf() is not defined in Android
// so we add our own 'dpritnf'
ssize_t Console::Utility::mydprintf(int sock, const char *format, ...)
{
    va_list args;
    char buf[16386];
    
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return sendToConsole(sock, buf, strlen(buf));
}

void Console::Utility::sendPrompt(int fd)
{
    const char* prompt = _prompt.c_str();
    send(fd, prompt, strlen(prompt), 0);
}

void Console::Utility::setPrompt(const std::string &prompt)
{
    _prompt = prompt;
}

const std::string& Console::Utility::getPrompt()
{
    return _prompt;
}

//
// Command code
//

void Console::Command::addCallback(const Callback& callback_)
{
    callback = callback_;
}

void Console::Command::addSubCommand(const Command& subCmd)
{
    subCommands[subCmd.name] = subCmd;
}

const Console::Command* Console::Command::getSubCommand(const std::string& subCmdName) const
{
    auto it = subCommands.find(subCmdName);
    if(it != subCommands.end()) {
        auto& subCmd = it->second;
        return &subCmd;
    }
    return nullptr;
}

void Console::Command::delSubCommand(const std::string& subCmdName)
{
    auto it = subCommands.find(subCmdName);
    if(it != subCommands.end()) {
        subCommands.erase(it);
    }
}

void Console::Command::commandHelp(int fd, const std::string& /*args*/)
{
    if (! help.empty()) {
        Console::Utility::mydprintf(fd, "%s\n", help.c_str());
    }
    
    if (! subCommands.empty()) {
        sendHelp(fd, subCommands, "");
    }
}

void Console::Command::commandGeneric(int fd, const std::string& args)
{
    // The first argument (including the empty)
    std::string key(args);
    auto pos = args.find(" ");
    if ((pos != std::string::npos) && (0 < pos)) {
        key = args.substr(0, pos);
    }
    
    // help
    if (key == "help" || key == "-h") {
        commandHelp(fd, args);
        return;
    }
    
    // find sub command
    auto it = subCommands.find(key);
    if (it != subCommands.end()) {
        auto subCmd = it->second;
        if (subCmd.callback) {
            subCmd.callback(fd, args);
        }
        return;
    }
    
    // can not find
    if (callback) {
        callback(fd, args);
    }
}

//
// Console code
//

Console::Console()
: _listenfd(-1)
, _running(false)
, _endThread(false)
, _isIpv6Server(false)
, _sendDebugStrings(false)
, _bindAddress("")
{

}

Console::~Console()
{
    stop();
}

bool Console::listenOnTCP(int port)
{
    int listenfd = -1, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;
    char serv[30];

    snprintf(serv, sizeof(serv)-1, "%d", port );

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    WSADATA wsaData;
    n = WSAStartup(MAKEWORD(2, 2),&wsaData);
#endif

    if ( (n = getaddrinfo(nullptr, serv, &hints, &res)) != 0) {
        fprintf(stderr,"net_listen error for %s: %s", serv, gai_strerror(n));
        return false;
    }

    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue;       /* error, try next one */

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

        // bind address
        if (!_bindAddress.empty())
        {
            if (res->ai_family == AF_INET)
            {
                struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin_addr);
            }
            else if (res->ai_family == AF_INET6)
            {
                struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin6_addr);
            }
        }

        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break;          /* success */

/* bind error, close and try next one */
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
        closesocket(listenfd);
#else
        close(listenfd);
#endif
    } while ( (res = res->ai_next) != nullptr);
    
    if (res == nullptr) {
        perror("net_listen:");
        freeaddrinfo(ressave);
        return false;
    }

    listen(listenfd, 50);

    if (res->ai_family == AF_INET) {
        _isIpv6Server = false;
        char buf[INET_ADDRSTRLEN] = {0};
        struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
        if( inet_ntop(res->ai_family, &sin->sin_addr, buf, sizeof(buf)) != nullptr )
            CCLOG("Console: IPV4 server is listening on %s:%d", buf, ntohs(sin->sin_port));
        else
            perror("inet_ntop");
    } else if (res->ai_family == AF_INET6) {
        _isIpv6Server = true;
        char buf[INET6_ADDRSTRLEN] = {0};
        struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
        if( inet_ntop(res->ai_family, &sin->sin6_addr, buf, sizeof(buf)) != nullptr )
            CCLOG("Console: IPV6 server is listening on [%s]:%d", buf, ntohs(sin->sin6_port));
        else
            perror("inet_ntop");
    }

    freeaddrinfo(ressave);
    return listenOnFileDescriptor(listenfd);
}

bool Console::listenOnFileDescriptor(int fd)
{
    if(_running) {
        CCLOG("Console already started. 'stop' it before calling 'listen' again");
        return false;
    }

    _listenfd = fd;
    _thread = std::thread( std::bind( &Console::loop, this) );

    return true;
}

void Console::stop()
{
    if( _running ) {
        _endThread = true;
        if (_thread.joinable())
        {
            _thread.join();
        }
    }
}

void Console::addCommand(const Command& cmd)
{
    _commands[cmd.name] = cmd;
}

void Console::addSubCommand(const std::string& cmdName, const Command& subCmd)
{
    auto it = _commands.find(cmdName);
    if(it != _commands.end()) {
        auto& cmd = it->second;
        addSubCommand(cmd, subCmd);
    }
}

void Console::addSubCommand(Command& cmd, const Command& subCmd)
{
    cmd.subCommands[subCmd.name] = subCmd;
}

const Console::Command* Console::getCommand(const std::string& cmdName)
{
    auto it = _commands.find(cmdName);
    if(it != _commands.end()) {
        auto& cmd = it->second;
        return &cmd;
    }
    return nullptr;
}

const Console::Command* Console::getSubCommand(const std::string& cmdName, const std::string& subCmdName)
{
    auto it = _commands.find(cmdName);
    if(it != _commands.end()) {
        auto& cmd = it->second;
        return getSubCommand(cmd, subCmdName);
    }
    return nullptr;
}

const Console::Command* Console::getSubCommand(const Command& cmd, const std::string& subCmdName)
{
    return cmd.getSubCommand(subCmdName);
}

void Console::delCommand(const std::string& cmdName)
{
    auto it = _commands.find(cmdName);
    if(it != _commands.end()) {
        _commands.erase(it);
    }
}

void Console::delSubCommand(const std::string& cmdName, const std::string& subCmdName)
{
    auto it = _commands.find(cmdName);
    if(it != _commands.end()) {
        auto& cmd = it->second;
        delSubCommand(cmd, subCmdName);
    }
}

void Console::delSubCommand(Command& cmd, const std::string& subCmdName)
{
    cmd.delSubCommand(subCmdName);
}

void Console::log(const char* buf)
{
    if( _sendDebugStrings ) {
        _DebugStringsMutex.lock();
        _DebugStrings.push_back(buf);
        _DebugStringsMutex.unlock();
    }
}

void Console::setBindAddress(const std::string &address)
{
    _bindAddress = address;
}

bool Console::isIpv6Server() const
{
    return _isIpv6Server;
}

//
// Main Loop
//
void Console::loop()
{
    fd_set copy_set;
    struct timeval timeout, timeout_copy;
    
    _running = true;
    
    FD_ZERO(&_read_set);
    FD_SET(_listenfd, &_read_set);
    _maxfd = _listenfd;
    
    timeout.tv_sec = 0;
    
    /* 0.016 seconds. Wake up once per frame at 60PFS */
    timeout.tv_usec = 16000;
    
    while(!_endThread) {
        
        copy_set = _read_set;
        timeout_copy = timeout;
        
        int nready = select(_maxfd+1, &copy_set, nullptr, nullptr, &timeout_copy);
        
        if( nready == -1 )
        {
            /* error */
            if(errno != EINTR)
                CCLOG("Abnormal error in select()\n");
            continue;
        }
        else if( nready == 0 )
        {
            /* timeout. do something ? */
        }
        else
        {
            /* new client */
            if(FD_ISSET(_listenfd, &copy_set)) {
                addClient();
                if(--nready <= 0)
                    continue;
            }
            
            /* data from client */
            std::vector<int> to_remove;
            for(const auto &fd: _fds) {
                if(FD_ISSET(fd,&copy_set))
                {
                    //fix Bug #4302 Test case ConsoleTest--ConsoleUploadFile crashed on Linux
                    //On linux, if you send data to a closed socket, the sending process will
                    //receive a SIGPIPE, which will cause linux system shutdown the sending process.
                    //Add this ioctl code to check if the socket has been closed by peer.
                    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
                    u_long n = 0;
                    ioctlsocket(fd, FIONREAD, &n);
#else
                    int n = 0;
                    ioctl(fd, FIONREAD, &n);
#endif
                    if(n == 0)
                    {
                        //no data received, or fd is closed
                        continue;
                    }
                    
                    if( ! parseCommand(fd) )
                    {
                        to_remove.push_back(fd);
                    }
                    if(--nready <= 0)
                        break;
                }
            }
            
            /* remove closed connections */
            for(int fd: to_remove) {
                FD_CLR(fd, &_read_set);
                _fds.erase(std::remove(_fds.begin(), _fds.end(), fd), _fds.end());
            }
        }
        
        /* Any message for the remote console ? send it! */
        if( !_DebugStrings.empty() ) {
            if (_DebugStringsMutex.try_lock())
            {
                for (const auto &str : _DebugStrings) {
                    for (auto fd : _fds) {
                        Console::Utility::sendToConsole(fd, str.c_str(), str.length());
                    }
                }
                _DebugStrings.clear();
                _DebugStringsMutex.unlock();
            }
        }
    }
    
    // clean up: ignore stdin, stdout and stderr
    for(const auto &fd: _fds )
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
        closesocket(fd);
#else
        close(fd);
#endif
    }
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    closesocket(_listenfd);
    WSACleanup();
#else
    close(_listenfd);
#endif
    _running = false;
}

//
// Helpers
//

ssize_t Console::readline(int fd, char* ptr, size_t maxlen)
{
    size_t n, rc;
    char c;
    
    for( n = 0; n < maxlen - 1; n++ ) {
        if( (rc = recv(fd, &c, 1, 0)) ==1 ) {
            *ptr++ = c;
            if(c == '\n') {
                break;
            }
        } else if( rc == 0 ) {
            return 0;
        } else if( errno == EINTR ) {
            continue;
        } else {
            return -1;
        }
    }
    
    *ptr = 0;
    return n;
}

ssize_t Console::readBytes(int fd, char* buffer, size_t maxlen, bool* more)
{
    size_t n, rc;
    char c, *ptr = buffer;
    *more = false;
    for( n = 0; n < maxlen; n++ ) {
        if( (rc = recv(fd, &c, 1, 0)) ==1 ) {
            *ptr++ = c;
            if(c == '\n') {
                return n;
            }
        } else if( rc == 0 ) {
            return 0;
        } else if( errno == EINTR ) {
            continue;
        } else {
            return -1;
        }
    }
    *more = true;
    return n;
}

bool Console::parseCommand(int fd)
{
    char buf[512];
    bool more_data;
    auto h = readBytes(fd, buf, 6, &more_data);
    if( h < 0)
    {
        return false;
    }
        if(!more_data)
    {
        buf[h] = 0;
    }
    else
    {
        char *pb = buf + 6;
        auto r = readline(fd, pb, sizeof(buf)-6);
        if(r < 0)
        {
            const char err[] = "Unknown error!\n";
            Console::Utility::sendPrompt(fd);
            Console::Utility::sendToConsole(fd, err, strlen(err));
            return false;
        }
    }
    std::string cmdLine;
    
    std::vector<std::string> args;
    cmdLine = std::string(buf);
    
    args = Console::Utility::split(cmdLine, ' ');
    if(args.empty())
    {
        const char err[] = "Unknown command. Type 'help' for options\n";
        Console::Utility::sendToConsole(fd, err, strlen(err));
        Console::Utility::sendPrompt(fd);
        return true;
    }
    
    auto it = _commands.find(Console::Utility::trim(args[0]));
    if(it != _commands.end())
    {
        std::string args2;
        for(size_t i = 1; i < args.size(); ++i)
        {
            if(i > 1)
            {
                args2 += ' ';
            }
            args2 += Console::Utility::trim(args[i]);
            
        }
        auto cmd = it->second;
        cmd.commandGeneric(fd, args2);
    }else if(strcmp(buf, "\r\n") != 0) {
        const char err[] = "Unknown command. Type 'help' for options\n";
        Console::Utility::sendToConsole(fd, err, strlen(err));
    }
    Console::Utility::sendPrompt(fd);
    
    return true;
}

void Console::addClient()
{
    struct sockaddr_in6 ipv6Addr;
    struct sockaddr_in ipv4Addr;
    struct sockaddr* addr = _isIpv6Server ? (struct sockaddr*)&ipv6Addr : (struct sockaddr*)&ipv4Addr;
    socklen_t addrLen = _isIpv6Server ? sizeof(ipv6Addr) : sizeof(ipv4Addr);
    
    /* new client */
    int fd = accept(_listenfd, addr, &addrLen);
    
    // add fd to list of FD
    if( fd != -1 ) {
        FD_SET(fd, &_read_set);
        _fds.push_back(fd);
        _maxfd = std::max(_maxfd,fd);
        
        Console::Utility::sendPrompt(fd);
        
        /**
         * A SIGPIPE is sent to a process if it tried to write to socket that had been shutdown for
         * writing or isn't connected (anymore) on iOS.
         *
         * The default behaviour for this signal is to end the process.So we make the process ignore SIGPIPE.
         */
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        int set = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif
    }
}

// helper free functions

void Console::printSceneGraphBoot(int fd)
{
    Console::Utility::sendToConsole(fd,"\n",1);
    //auto scene = NetMgr->getRunningScene();
    //int total = printSceneGraph(fd, scene, 0);
   // Console::Utility::mydprintf(fd, "Total Nodes: %d\n", total);
    Console::Utility::sendPrompt(fd);
}

void Console::printFileUtils(int fd)
{
    FileUtils* fu = FileUtils::getInstance();
    
    Console::Utility::mydprintf(fd, "\nSearch Paths:\n");
    auto& list = fu->getSearchPaths();
    for( const auto &item : list) {
        Console::Utility::mydprintf(fd, "%s\n", item.c_str());
    }
    
    Console::Utility::mydprintf(fd, "\nResolution Order:\n");
    auto& list1 = fu->getSearchResolutionsOrder();
    for( const auto &item : list1) {
        Console::Utility::mydprintf(fd, "%s\n", item.c_str());
    }
    
    Console::Utility::mydprintf(fd, "\nWritable Path:\n");
    Console::Utility::mydprintf(fd, "%s\n", fu->getWritablePath().c_str());
    
    Console::Utility::mydprintf(fd, "\nFull Path Cache:\n");
    auto& cache = fu->getFullPathCache();
    for( const auto &item : cache) {
        Console::Utility::mydprintf(fd, "%s -> %s\n", item.first.c_str(), item.second.c_str());
    }
    Console::Utility::sendPrompt(fd);
}

void Console::sendHelp(int fd, const std::map<std::string, Command>& commands, const char* msg)
{
    Console::Utility::sendToConsole(fd, msg, strlen(msg));
    for(auto& it : commands)
    {
        auto command = it.second;
        if (command.help.empty()) continue;
        
        Console::Utility::mydprintf(fd, "\t%s", command.name.c_str());
        ssize_t tabs = strlen(command.name.c_str()) / 8;
        tabs = 3 - tabs;
        for(int j=0;j<tabs;j++){
            Console::Utility::mydprintf(fd, "\t");
        }
        Console::Utility::mydprintf(fd,"%s\n", command.help.c_str());
    }
}

NS_CC_END
