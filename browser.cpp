//  WinHttpAsyncSample
//  Copyright (c) Microsoft Corporation. All rights reserved. #include <windows.h>

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string>

#include "defs_link_list.h"
#include "browser.h"
#include "AsyncRequester.h"
//#include "ParserXml.h"

//
// SIMPLE_BROWSER class implementation
//
//const std::wstring urlId = L"http://tetraforex.anthill.by/management/components/signals/handler.php?action=deactivateSignals&id=";
static const LPCWSTR userAgent = L"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.79 Safari/537.4";

SIMPLE_BROWSER::SIMPLE_BROWSER(UINT nID)
{
	m_nID = nID + 1;
	m_hSession = NULL;

	m_pBrowserConfig = NULL;
	m_pHomePageRequester = NULL;

	m_hShutdownEvent = NULL;
	m_fShutdownInProgress = FALSE;

	m_nRequesterIDSeed = m_nID * 10; // for example, if the browser is 1, then the 
	// embedded links are numbered 10, 11, 12, and etc.
}

SIMPLE_BROWSER::~SIMPLE_BROWSER()
{
}

BOOL SIMPLE_BROWSER::Open(P_BROWSER_CONFIG pBrowserConfig)
{
	ASSERT(pBrowserConfig != NULL);

	m_pBrowserConfig = pBrowserConfig;

	InitializeListHead(&m_EmbeddedLinks);

	::InitializeCriticalSection(&m_LinksCritSec);

	PWSTR pwszEmbeddedLinks = NULL;
	SIZE_T cchEmbeddedLinks = 0;

	fprintf(stdout, "Browser #%d is initializing...\n", m_nID);

	m_hShutdownEvent = ::CreateEvent(NULL,
		TRUE,  // manual reset
		FALSE, // not initially set
		NULL);

	if (m_hShutdownEvent == NULL)
	{
		fprintf(stderr, "Browser #%d failed to open; CreateEvent() failed; error = %d.\n", m_nID, ::GetLastError());
		goto error_cleanup;
	}

	DWORD dwAccessType = pBrowserConfig->fProxyAutoDiscovery ? WINHTTP_ACCESS_TYPE_NO_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

	m_hSession = ::WinHttpOpen(userAgent, dwAccessType, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);

	if (m_hSession == NULL) {
		fprintf(stderr, "Browser #%d failed to open; WinHttpOpen() failed; error = %d.\n", m_nID, ::GetLastError());
		goto error_cleanup;
	}

	m_pHomePageRequester = new ASYNC_REQUESTER(m_nRequesterIDSeed++, this);

	if (m_pHomePageRequester == NULL) {
		fprintf(stderr, "Browser #%d failed to open; not enough memory.\n", m_nID);
		goto error_cleanup;
	}

	if (m_pHomePageRequester->Open(pBrowserConfig->pwszHomePage,
		pBrowserConfig->fProxyAutoDiscovery,
		pBrowserConfig->fEnableProxyFailover,
		pBrowserConfig->nFailureRetries,
		pBrowserConfig->nTimeLimit) == FALSE)
	{
		fprintf(stderr, "Browser #%d failed to open; can not initialize a requester.\n", m_nID);
		goto error_cleanup;
	}

	if (m_pBrowserConfig->pwszEmbeddedLinks == NULL)
	{
		fprintf(stderr, "Browser #%d failed to open; invalid list of embedded links.\n", m_nID);
		goto error_cleanup;
	}

	cchEmbeddedLinks = ::wcslen(m_pBrowserConfig->pwszEmbeddedLinks) + 1;

	if (cchEmbeddedLinks < 1)
	{
		fprintf(stderr, "Browser #%d failed to open; integer overflow.\n", m_nID);
		goto error_cleanup;
	}

	pwszEmbeddedLinks = new WCHAR[cchEmbeddedLinks];
	if (pwszEmbeddedLinks == NULL)
	{
		fprintf(stderr, "Browser #%d failed to open; not enough memory.\n", m_nID);
		goto error_cleanup;
	}

	::wcscpy_s(pwszEmbeddedLinks, cchEmbeddedLinks, m_pBrowserConfig->pwszEmbeddedLinks);

	PWSTR pwszContext = NULL;
	PWSTR pwszLink = ::wcstok_s(pwszEmbeddedLinks, L";", &pwszContext);
	while (pwszLink != NULL)
	{
		fprintf(stdout, "Browser #%d is initializing link %S...\n", m_nID, pwszLink);

		ASYNC_REQUESTER* pNewRequester = new ASYNC_REQUESTER(m_nRequesterIDSeed++, this);
		if (pNewRequester == NULL)
		{
			fprintf(stderr, "Browser #%d failed to create a new request; not enough memory.\n", m_nID);
			goto next_link;
		}

		if (pNewRequester->Open(pwszLink,
			m_pBrowserConfig->fProxyAutoDiscovery,
			m_pBrowserConfig->fEnableProxyFailover,
			m_pBrowserConfig->nFailureRetries,
			m_pBrowserConfig->nTimeLimit) == FALSE)
		{
			fprintf(stderr, "Browser #%d failed to initialize a new requester.\n", m_nID);
			goto next_link;
		}

		InsertHeadList(&m_EmbeddedLinks, (PLIST_ENTRY)pNewRequester);
		pNewRequester = NULL;

	next_link:

		if (pNewRequester)
		{
			if (m_pHomePageRequester->m_State == ASYNC_REQUESTER::OPENED)
			{
				m_pHomePageRequester->Close();
			}
			delete pNewRequester;
		}
		pwszLink = ::wcstok_s(NULL, L";", &pwszContext);
	}

	delete[] pwszEmbeddedLinks;
	pwszEmbeddedLinks = NULL;

	if (m_pHomePageRequester->Start() == FALSE)
	{
		fprintf(stderr, "Browser #%d failed to open; can not start a requester.\n", m_nID);
		goto error_cleanup;
	}

	return TRUE;

error_cleanup:

	if (m_pHomePageRequester)
	{
		if (m_pHomePageRequester->m_State == ASYNC_REQUESTER::OPENED) {
			m_pHomePageRequester->Close();
		}
		delete m_pHomePageRequester;
		m_pHomePageRequester = NULL;
	}

	if (m_hSession)
	{
		::WinHttpCloseHandle(m_hSession);
		m_hSession = NULL;
	}
	if (m_hShutdownEvent)
	{
		::CloseHandle(m_hShutdownEvent);
		m_hShutdownEvent = NULL;
	}

	if (pwszEmbeddedLinks)
	{
		delete[] pwszEmbeddedLinks;
	}
	::DeleteCriticalSection(&m_LinksCritSec);
	return FALSE;
}

BOOL SIMPLE_BROWSER::SaveToResponse(const char* buffer)
{
	::EnterCriticalSection(&m_LinksCritSec);

	//std::string temp(buffer, strlen(buffer));
	//m_stdstrResponse += temp;

	size_t bufferLen = ::strlen(buffer) + 1;
	//m_pwszResponse = new char();
	m_pwszResponse = new CHAR[bufferLen];
	//StrCmp(m_pwszResponse, buffer);
	::strcpy_s(m_pwszResponse, bufferLen, buffer);

	::LeaveCriticalSection(&m_LinksCritSec);
	return TRUE;
}

PCHAR SIMPLE_BROWSER::getResponse()
{
	return this->m_pwszResponse;
	//return m_stdstrResponse;
}

VOID SIMPLE_BROWSER::Close(VOID)
{
	m_fShutdownInProgress = TRUE;

	BOOL fNeedToWait = FALSE;

	::EnterCriticalSection(&m_LinksCritSec);

	if (m_pHomePageRequester)
	{
		if (m_pHomePageRequester->m_State != ASYNC_REQUESTER::CLOSED)
		{
			m_pHomePageRequester->RequestToShutdown();
			fNeedToWait = TRUE;
		}
	}

	for (PLIST_ENTRY entry = (&m_EmbeddedLinks)->Flink;
		entry != (PLIST_ENTRY)&((&m_EmbeddedLinks)->Flink);
		entry = entry->Flink)
	{
		ASYNC_REQUESTER* pRequester = (ASYNC_REQUESTER*)entry;

		if (pRequester->m_State != ASYNC_REQUESTER::CLOSED)
		{
			pRequester->RequestToShutdown();
			fNeedToWait = TRUE;
		}
	}

	::LeaveCriticalSection(&m_LinksCritSec);

	if (fNeedToWait)
	{
		fprintf(stdout, "Browser #%d waiting for all requesters to exit...\n", m_nID);

		if (::WaitForSingleObject(m_hShutdownEvent, /*60000*/INFINITE) == WAIT_TIMEOUT)
		{
			fprintf(stderr, "...and Browser #%d timed out waiting all requesters to exit... exit anyway...\n", m_nID);
		}
		else
		{
			fprintf(stdout, "...and Browser #%d is notified that all requesters are exited...\n", m_nID);
		}
	}

	while (!IsListEmpty(&m_EmbeddedLinks))
	{
		PLIST_ENTRY pEntry = RemoveHeadList(&m_EmbeddedLinks);
		ASYNC_REQUESTER* pRequester = (ASYNC_REQUESTER*)pEntry;

		delete pRequester;
	}

	if (m_hSession)
	{
		::WinHttpCloseHandle(m_hSession);
		m_hSession = NULL;
	}

	if (m_hShutdownEvent)
	{
		::CloseHandle(m_hShutdownEvent);
		m_hShutdownEvent = NULL;
	}
	::DeleteCriticalSection(&m_LinksCritSec);
}

VOID SIMPLE_BROWSER::OnRequesterStopped(ASYNC_REQUESTER* pRequester)
{
	ASSERT(pRequester != NULL);

	if (m_fShutdownInProgress  &&
		pRequester->m_State != ASYNC_REQUESTER::CLOSING) {
		return;
	}

	::EnterCriticalSection(&m_LinksCritSec);

	if (pRequester == m_pHomePageRequester)
	{
		if (pRequester->m_State == ASYNC_REQUESTER::DATA_EXHAUSTED)
		{
			fprintf(stdout, "Browser #%d fetched its Home Page, now it attempts to download all embedded lnks.\n", m_nID);
			//????fprintf(stdout, pRequester->getResponse().c_str());

			OnStartChildResponse(pRequester);

			for (PLIST_ENTRY entry = (&m_EmbeddedLinks)->Flink;
				entry != (PLIST_ENTRY)&((&m_EmbeddedLinks)->Flink);
				entry = entry->Flink)
			{
				ASYNC_REQUESTER* pRequesterNew = (ASYNC_REQUESTER*)entry;
				if (pRequesterNew->Start() == FALSE)
				{
					fprintf(stderr, "Browser #%d failed to start a new requester.\n", m_nID);
				}
			}

			// now we can close the home page request
			pRequester->Close();

			goto exit;
		}
	}

	BOOL fClosingRequest = TRUE;

	if (pRequester->m_State == ASYNC_REQUESTER::CLOSING)
	{
		OnStartChildResponse(pRequester);
		pRequester->Close();    // a live download has been aborted, now close it.
	}
	else if (pRequester->m_State == ASYNC_REQUESTER::ERROR)
	{
		if (m_pBrowserConfig->nFailureRetries)
		{
			switch (pRequester->m_dwLastError)
			{
			case ERROR_WINHTTP_CANNOT_CONNECT:
			case ERROR_WINHTTP_TIMEOUT:
			case ERROR_NOT_ENOUGH_MEMORY:
			case ERROR_WINHTTP_INTERNAL_ERROR:
			case ERROR_WINHTTP_NAME_NOT_RESOLVED:

				if (pRequester->Start() == FALSE)
				{
					pRequester->Close(); // retry upon certain errors
				}
				else
				{
					fClosingRequest = FALSE;
				}
				break;
			default:
				pRequester->Close();
			}
		}
		else  // close the request since we don't need to retry 
		{
			pRequester->Close();
		}
	}
	else // close the request if some other state is hit
	{
		pRequester->Close();
	}

	// the main page is interrupted, so we'll need to close all embedded links
	if (pRequester == m_pHomePageRequester && fClosingRequest)
	{
		for (PLIST_ENTRY entry = (&m_EmbeddedLinks)->Flink;
			entry != (PLIST_ENTRY)&((&m_EmbeddedLinks)->Flink);
			entry = entry->Flink)
		{
			ASYNC_REQUESTER* pRequesterToC = (ASYNC_REQUESTER*)entry;
			pRequesterToC->Close();
		}
	}

exit:
	::LeaveCriticalSection(&m_LinksCritSec);
}

VOID SIMPLE_BROWSER::OnStartChildResponse(ASYNC_REQUESTER* pRequester)
{
	fprintf(stdout, pRequester->getResponse().c_str());

	vector<wstring> genetateUrls;
	//GenerateRequests(pRequester->getResponse(), genetateUrls);
	if (!genetateUrls.empty())
	{
		for (size_t i = 0; i < genetateUrls.size(); i++)
		{
			ASYNC_REQUESTER* pNewRequester = new ASYNC_REQUESTER(m_nRequesterIDSeed++, this);

			if (pNewRequester == NULL)
			{
				fprintf(stderr, "Browser #%d failed to create a new request; not enough memory.\n", m_nID);
				goto next_link;
			}

			SIZE_T ccLink = ::wcslen(genetateUrls[i].c_str()) + 1;
			PWSTR pwszLink = new WCHAR[ccLink];
			::wcscpy_s(pwszLink, ccLink, genetateUrls[i].c_str());

			if (pNewRequester->Open(pwszLink,
				m_pBrowserConfig->fProxyAutoDiscovery,
				m_pBrowserConfig->fEnableProxyFailover,
				m_pBrowserConfig->nFailureRetries,
				m_pBrowserConfig->nTimeLimit) == FALSE)
			{
				fprintf(stderr, "Browser #%d failed to initialize a new requester.\n", m_nID);
				goto next_link;
			}

			InsertHeadList(&m_EmbeddedLinks, (PLIST_ENTRY)pNewRequester);
			pNewRequester = NULL;
		next_link:
			delete[] pwszLink;
			delete pNewRequester;
		}
	}

}

// Generate requests for children requestors
/*BOOL SIMPLE_BROWSER::GenerateRequests(const std::string& response, vector<wstring>& generateUrls) {
	Signals signalsLine;

	if(!response.empty()) {
		ParserXml::get_mutable_instance().getSignalsVector(response, signalsLine);
	}

	if(!signalsLine.empty()) {
		for(int i = 0; i < signalsLine.size(); i++) {
			wchar_t id[10] ;
			_itow_s<10>(signalsLine[i].id, id, 10);
			std::wstring tmp(id);
			generateUrls.push_back(urlId + tmp);
		}
	}
	return TRUE;
}*/

VOID SIMPLE_BROWSER::OnRequesterClosed(ASYNC_REQUESTER* pRequester)
{
	::EnterCriticalSection(&m_LinksCritSec);

	if (m_fShutdownInProgress)
	{
		BOOL fAllClosed = TRUE;
		if (m_pHomePageRequester && m_pHomePageRequester->m_State != ASYNC_REQUESTER::CLOSED)
		{
			fAllClosed = FALSE;
		}
		for (PLIST_ENTRY entry = (&m_EmbeddedLinks)->Flink;
			entry != (PLIST_ENTRY)&((&m_EmbeddedLinks)->Flink);
			entry = entry->Flink)
		{
			ASYNC_REQUESTER* pRequesterNew = (ASYNC_REQUESTER*)entry;
			if (pRequesterNew->m_State != ASYNC_REQUESTER::CLOSED)
			{
				fAllClosed = FALSE;
			}
		}

		if (fAllClosed)
		{
			::SetEvent(m_hShutdownEvent);
		}
	}

	::LeaveCriticalSection(&m_LinksCritSec);
}
