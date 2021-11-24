#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>


bool disable_service(const wchar_t* szSvcName)
{
	wprintf(L"\n------------------------------------------------------------------------\n");
	wprintf(L"Service %s...\n", szSvcName);

	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	DWORD dwError = 0;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManagerW(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		wprintf(L"OpenSCManager(%s) failed (error %d (5 == ERROR_ACCESS_DENIED))\n", szSvcName, GetLastError());
		return false;
	}

	// Get a handle to the service.

	schService = OpenServiceW(
		schSCManager,            // SCM database 
		szSvcName,               // name of service 
		SERVICE_ALL_ACCESS);  // need change config access 

	if (schService == NULL)
	{
		dwError = GetLastError();
		
		if (dwError == ERROR_SERVICE_DOES_NOT_EXIST) {
			wprintf(L"Service %s is not isntalled, it's very nice.\n", szSvcName);
			CloseServiceHandle(schSCManager);
			return true;
		}

		wprintf(L"OpenService(%s) failed (error %d (5 == ERROR_ACCESS_DENIED))\n", szSvcName, dwError);
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Query the servic info

	LPQUERY_SERVICE_CONFIGW lpsc = NULL;
	LPSERVICE_DESCRIPTIONW lpsd = NULL;
	DWORD dwBytesNeeded = 0;
	DWORD cbBufSize = 0;

	if (!QueryServiceConfig2W(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();

		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			wprintf(L"QueryServiceConfig2(%s) failed (%d)", szSvcName, dwError);
		}
	}

	if (!QueryServiceConfig2W(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		(LPBYTE)lpsd,
		cbBufSize,
		&dwBytesNeeded))
	{
		wprintf(L"QueryServiceConfig2(%s) failed (%d)", szSvcName, GetLastError());
	}

	wprintf(L"%s\n", lpsd != NULL ? lpsd->lpDescription : L"query descriotion failed");

	// Change the service start type.

	if (!ChangeServiceConfigW(
		schService,        // handle of service 
		SERVICE_NO_CHANGE, // service type: no change 
		SERVICE_DISABLED,  // service start type 
		SERVICE_NO_CHANGE, // error control: no change 
		NULL,              // binary path: no change 
		NULL,              // load order group: no change 
		NULL,              // tag ID: no change 
		NULL,              // dependencies: no change 
		NULL,              // account name: no change 
		NULL,              // password: no change 
		NULL))            // display name: no change
	{
		wprintf(L"ChangeServiceConfig(%s) failed (%d)\n", szSvcName, GetLastError());
	}

	SERVICE_STATUS ss;
	BOOL bCS = ControlService(schService, SERVICE_CONTROL_STOP, &ss);

	if (!bCS) {
		dwError = GetLastError();

		if (dwError == ERROR_SERVICE_NOT_ACTIVE) {
			wprintf(L"Service %s is not running, it's very nice.\n", szSvcName);
		}
		else
		if (dwError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL) {
			wprintf(L"ControlService(%s) Warning: ERROR_SERVICE_CANNOT_ACCEPT_CTRL\n", szSvcName);
		}
		else {
			wprintf(L"ControlService(%s) failed (%d)\n", szSvcName, GetLastError());
		}
	}
	else {
		SERVICE_STATUS_PROCESS ssp;
		ssp.dwCurrentState = 0;

		for (int i = 0; i < 10; ++i) {
			Sleep(1000);

			DWORD dwBytesNeeded = 0;

			if (!QueryServiceStatusEx(
				schService,
				SC_STATUS_PROCESS_INFO,
				(LPBYTE)&ssp,
				sizeof(SERVICE_STATUS_PROCESS),
				&dwBytesNeeded))
			{
				wprintf(L"QueryServiceStatusEx(%s) failed (error %d)\n", szSvcName, GetLastError());
				break;
			}

			if (ssp.dwCurrentState == SERVICE_STOPPED)
				break;
		}

		if (ssp.dwCurrentState == SERVICE_STOP_PENDING)
			wprintf(L"QueryServiceStatusEx(%s) Warning: SERVICE_STOP_PENDING after 10s\n", szSvcName);
	}

	if (!QueryServiceConfigW(
		schService,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();

		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			wprintf(L"QueryServiceConfig(%s) failed (%d)", szSvcName, dwError);
		}
	}

	if (!QueryServiceConfigW(
		schService,
		lpsc,
		cbBufSize,
		&dwBytesNeeded))
	{
		wprintf(L"QueryServiceConfig(%s) failed (%d)", szSvcName, GetLastError());
	}

	wprintf(L"Service %s disabled successfully (check: %d == %d).\n", szSvcName, lpsc != NULL ? lpsc->dwStartType : 0, SERVICE_DISABLED);
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return true;
}
