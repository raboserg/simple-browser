//  WinHttpAsyncSample
//  Copyright (c) Microsoft Corporation. All rights reserved. 

#ifndef _SIMPLE_BROWSER_H
#define _SIMPLE_BROWSER_H

#include <string>
#include <vector>
#include <boost/function.hpp>

using namespace std;

typedef struct BROWSER_CONFIG
{
    PWSTR  pwszHomePage;
    BOOL   fProxyAutoDiscovery;
    BOOL   fEnableProxyFailover;
    UINT   nTimeLimit;         // in seconds
    UINT   nFailureRetries;    // retry the request upon some errors (e.g. ERROR_WINHTTP_TIMEOUT)
    PWSTR  pwszEmbeddedLinks;  // download these embedded links (in parallel) once the Home Page has been downloaded
} *P_BROWSER_CONFIG;

class ASYNC_REQUESTER;

class SIMPLE_BROWSER
{
public:
    SIMPLE_BROWSER(UINT nID);
    ~SIMPLE_BROWSER();

    BOOL Open(P_BROWSER_CONFIG); // initialize the object and kick start the Home Page download

    VOID Close(VOID);   // will block until all pending downloads gracefully shutdown

	BOOL SaveToResponse(const char*);
	std::string getResponse();
	BOOL GenerateRequests(const std::string& response, vector<wstring>& generateUrls);

private:
    VOID OnRequesterStopped(ASYNC_REQUESTER*);   // indicate a request transaction stopped for some reason
    VOID OnRequesterClosed(ASYNC_REQUESTER*);    // indicate a request is shutdown gracefully
	
	VOID OnStartChildResponse(ASYNC_REQUESTER*);

private:
    UINT        m_nID;
    HINTERNET   m_hSession;

    P_BROWSER_CONFIG    m_pBrowserConfig;
    ASYNC_REQUESTER*    m_pHomePageRequester;

    LIST_ENTRY          m_EmbeddedLinks;
    CRITICAL_SECTION    m_LinksCritSec;

    HANDLE  m_hShutdownEvent;
    BOOL    m_fShutdownInProgress;

    UINT    m_nRequesterIDSeed;
	PCHAR  m_pwszResponse;
	std::string m_stdstrResponse;

friend class ASYNC_REQUESTER;
};

#endif // _SIMPLE_BROWSER_H