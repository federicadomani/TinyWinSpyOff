#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <string>


int WriteBinary(const wchar_t* cfilename, const void* pChar, size_t szChar)
{
	FILE* fp = NULL;
	errno_t err = 0;

	err = _wfopen_s(&fp, cfilename, L"wb");
	if ((fp == NULL) || (err != 0))
	{
		return __LINE__;
	}

	size_t szw = fwrite(pChar, szChar, 1, fp);
	if (szw != 1)
		return __LINE__;

	fclose(fp);
	return 0;
}

bool wipe_file(const wchar_t* szPath, const void* pData, size_t szData) {
	wprintf(L"\n------------------------------------------------------------------------\n");
	
	wchar_t* pBuffer = NULL;
	size_t szBuffer = 0;
	errno_t err = _wdupenv_s(&pBuffer, &szBuffer, L"SYSTEMDRIVE");
	if (err != 0)
		return false;

	std::wstring strDrivePath = std::wstring(pBuffer) + std::wstring(szPath);
	int ret = WriteBinary(strDrivePath.c_str(), pData, szData);
	
	wprintf(L"Wiped file %s (check: %d == %d).\n", strDrivePath.c_str(), ret, 0);
	return ret == 0 ? true : false;
}
