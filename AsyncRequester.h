#ifndef _ASYNC_REQUESTER_H
#define _ASYNC_REQUESTER_H
//
// ASYNC_REQUESTER class implementation
//
#include <string>

class ASYNC_REQUESTER
{
public:
	ASYNC_REQUESTER(UINT nRequesterID, SIMPLE_BROWSER* pBrowser);

	~ASYNC_REQUESTER();

	BOOL Open(
		PWSTR pwszUrl,
		BOOL fProxyAutoDiscovery,
		BOOL fProxyFailover,
		UINT nFailureRetries,
		UINT nTimeLimit);

	BOOL Start();

	VOID RequestToShutdown();   // use to abort live download

	std::string getResponse();

private:
	VOID Close();

	VOID OnRequestError(LPWINHTTP_ASYNC_RESULT);
	VOID OnSendRequestComplete();
	VOID OnHeadersAvailable();
	VOID OnReadComplete(DWORD dwBytesRead);
	VOID OnHandleClosing(HINTERNET);
	VOID OnDataAvailable();

private:
	LIST_ENTRY  _List;

#ifdef ERROR
#undef ERROR
#endif

	enum STATE 
	{
		INIT,
		OPENED,
		ERROR,
		SENDING,
		RECEIVING,
		HEADERS_AVAILABLE,
		WAITING_FOR_DATA,
		DATA_AVAILABLE,
		DATA_EXHAUSTED,
		CLOSING,
		CLOSED
	};

	PWSTR m_pwszUrl;


	HINTERNET m_hConnect;
	HINTERNET m_hRequest;

	UINT m_nID;
	SIMPLE_BROWSER* m_pBrowser;

	BOOL m_fProxyAutoDiscovery;
	BOOL m_fProxyFailover;
	UINT m_nFailureRetries;

	WINHTTP_PROXY_INFO m_ProxyInfo;

	LPWSTR m_pwszNextProxies;

	DWORD m_dwLastError;
	DWORD m_dwErrorAPI;

	STATE m_State;

	BOOL m_fClosing;

	enum { READ_BUFFER_SIZE = 1024 };
	char m_ReadBuffer[READ_BUFFER_SIZE];

	int m_ContentLength;
	DWORD m_dwBytesReadSoFar;

	DWORD m_dwStartingTime; // in millisecond
	DWORD m_dwTimeLimit;    // in millisecond
	int   m_TimeRemaining;  // in millsecond

	int m_nNumAttempts;
	
	std::string m_Response;

	friend class SIMPLE_BROWSER;
	friend void CALLBACK RequesterStatusCallback (
		HINTERNET hInternet,
		DWORD_PTR dwContext,
		DWORD dwInternetStatus,
		LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength
		);
};

#endif // ASYNC_REQUESTER_H