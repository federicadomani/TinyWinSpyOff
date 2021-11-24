#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>


bool set_registry(const wchar_t* path, const wchar_t* name, unsigned int type, const void* value)
{
	wprintf(L"\n------------------------------------------------------------------------\n");

	LONG status = 0;
	HKEY hKey = NULL;

	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_ALL_ACCESS, &hKey);

	if (status != ERROR_SUCCESS)
	{
		wprintf(L"HKEY_LOCAL_MACHINE RegOpenKeyExW(%s) failed\n", path);
		return false;
	}

	if (type == REG_SZ)
		status = RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE*)value, ((DWORD)wcslen((const wchar_t*)value) + 1) * sizeof(wchar_t));

	if (type == REG_DWORD)
		status = RegSetValueExW(hKey, name, 0, REG_DWORD, (const BYTE*)value, sizeof(DWORD));


	if (status != ERROR_SUCCESS)
	{
		wprintf(L"HKEY_LOCAL_MACHINE RegSetValueExW(%s) failed\n", name);
		RegCloseKey(hKey);
		return false;
	}

	wprintf(L"HKEY_LOCAL_MACHINE Registry value(%s, %s) set successfully.\n", path, name);

	return true;
}
