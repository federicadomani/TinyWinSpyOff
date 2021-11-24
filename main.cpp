/***********************************************************************************
Inspired by the following projects:
https://github.com/builtbybel/privatezilla
https://github.com/bitlog2/DisableWinTracking
************************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>


bool add_permission_for_current_user(const wchar_t* permission);
bool disable_service(const wchar_t* szSvcName);
bool set_registry(const wchar_t* path, const wchar_t* name, unsigned int type, const void* value);
bool wipe_file(const wchar_t* szPath, const void* pData, size_t szData);

const char* quotes = "\"\"";

int main() {
	add_permission_for_current_user(L"SeLockMemoryPrivilege");
	
#ifdef _DEBUG
	disable_service(L"Fax");
#endif
	disable_service(L"diagnosticshub.standardcollector.service");
	disable_service(L"DiagTrack");
	disable_service(L"dmwappushservice");
	disable_service(L"lfsvc");
	disable_service(L"MapsBroker");
	disable_service(L"WbioSrvc");
	disable_service(L"ndu");

	DWORD nZero = 0;
	set_registry(L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", REG_DWORD, &nZero);

	wipe_file(L"\\ProgramData\\Microsoft\\Diagnosis\\ETLLogs\\AutoLogger\\AutoLogger-Diagtrack-Listener.etl", quotes, strlen(quotes));

	wprintf(L"\n------------------------------------------------------------------------\n");
	wprintf(L"\nPlease restart for changes to take effect.\n");
	system("pause");
	return 0;
}
