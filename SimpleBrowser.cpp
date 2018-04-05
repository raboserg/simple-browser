//  WinHttpAsyncSample
//  Copyright (c) Microsoft Corporation. All rights reserved. 

#include <windows.h>
#include <winhttp.h>
#include <string>
#include "defs_link_list.h"
#include "browser.h"
#include "AsyncRequester.h"

#include <stdio.h>

// Modify the config below to control the number of browser sessions this sample would "browse"
// in parallel. For each session, a block below, you can configure what "home page" it will
// begin with, how to retrieve proxy infomation (e.g. auto detect or use pre-configured settings), as 
// well as a ";" delimited "embedded links" that the browser would download after the main page
// is fetched.

BROWSER_CONFIG BrowserConfigs[] =
{
	/*
	{
        L"https://www.microsoft.com",   // home page
        TRUE,                           // use auto-proxy
        TRUE,                           // enable proxy fail-over
        30,                             // time limit (in seconds)
        2,                              // reties 2 times if a request should fail
        
        L"http://www.microsoft.com/servers/;"
        L"http://www.microsoft.com/windows/;"
        L"http://www.microsoft.com/security/",  // ";" delimited embedded links 
    }
    
    ,
	*/
    {
        L"https://www.yahoo.com",        // home page
        FALSE,                          // use default proxy (configured thru. "netsh winhttp set proxy")
        TRUE,                           // enable proxy fail-over
        50,
        3,                              // reties 3 times if a request should fail
        
        L"https://shopping.yahoo.com/;"
        L"https://finance.yahoo.com/;"
        L"https://news.yahoo.com/",      // ";" delimited embedded links 
    }
    
    ,
	
	{
		L"http://edition.cnn.com/",          // home page
        FALSE,                          // use default proxy (configured thru. "netsh winhttp set proxy")
        TRUE,                          // disable proxy fail-over
        50,
        3,                              // don't retry if a request should fail
		L"http://edition.cnn.com/world/;"
        L"http://edition.cnn.com/weather/;"
        L"http://edition.cnn.com/travel/",  // ";" delimited embedded links 
    }
};

SIMPLE_BROWSER* BrowserSessions[sizeof(BrowserConfigs) / sizeof (BROWSER_CONFIG)];



// we register a "Ctrl-C" handler to show how on-going downloads can be 
// aborted gracefully 

BOOL WINAPI ConsoleEventHandler(
    DWORD dwCtrlType   //  control signal type
)
{
    fprintf(stdout, "Ctrl-C detected...\n");

    UINT nNumBrowserSessions = sizeof(BrowserConfigs) / sizeof (BROWSER_CONFIG);

    for (UINT i = 0; i < nNumBrowserSessions; ++i)
    {
        if (BrowserSessions[i])
        {
            BrowserSessions[i]->Close();
            delete BrowserSessions[i];
            BrowserSessions[i] = NULL;
        }
    }

    ::ExitProcess(0);
}

int __cdecl main() {
    // first mask off Ctrl-C so that we can initialize w/o worrying being interrupted
    ::SetConsoleCtrlHandler(NULL, TRUE);

    UINT nNumBrowserSessions = sizeof(BrowserConfigs) / sizeof (BROWSER_CONFIG);

    for (UINT i = 0; i < nNumBrowserSessions; ++i) {
        SIMPLE_BROWSER* pBrowser = new SIMPLE_BROWSER(i);
        if (pBrowser) {
            if (pBrowser->Open(&BrowserConfigs[i]) == TRUE) {
                BrowserSessions[i] = pBrowser;
            } else {
                delete pBrowser;
            }
        }
    }

    // now that things are started let's unmask Ctrl-C and register a custom Ctrl handler
    // to faciliate gracefully shutdown
    ::SetConsoleCtrlHandler(NULL, FALSE);
    ::SetConsoleCtrlHandler(ConsoleEventHandler, TRUE);

    ::Sleep(INFINITE);

    return 0;
}