/* SetUserRightPermissions.h
 *
 * Author           : Alexander J. Yee
 * Date Created     : 01/06/2016
 * Last Modified    : 01/06/2016
 *
 *      Add permissions for a specific user without Group Policy.
 *  This is a simplified and updated version of: https://support.microsoft.com/en-us/kb/132958
 *
 *  Naturally, this needs to be run as administrator.
 *
 */

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Ntsecapi.h>
#include <Lmcons.h>

#include <string>
#include <memory>
#include <vector>
#include <iostream>

using std::wcout;
using std::endl;


std::wstring get_user_name() {
	wchar_t buffer[UNLEN + 1];
	DWORD length = UNLEN + 1;
	if (GetUserNameW(buffer, &length) == 0) {
		return L"";
	}
	return buffer;
}

bool InitLsaString(
	PLSA_UNICODE_STRING pLsaString,
	LPCWSTR pwszString
) {
	size_t dwLen = 0;

	if (NULL == pLsaString)
		return FALSE;

	if (NULL != pwszString)
	{
		dwLen = wcslen(pwszString);
		if (dwLen > 0x7ffe)   // String is too large
			return FALSE;
	}

	// Store the string.
	pLsaString->Buffer = (WCHAR *)pwszString;
	pLsaString->Length = (USHORT)dwLen * sizeof(WCHAR);
	pLsaString->MaximumLength = (USHORT)(dwLen + 1) * sizeof(WCHAR);

	return TRUE;
}

class LsaHandle {
	LSA_HANDLE handle = nullptr;
	LSA_OBJECT_ATTRIBUTES attr;

public:
	LSA_HANDLE get_handle() const {
		return handle;
	}
	const LSA_OBJECT_ATTRIBUTES& get_attributes() const {
		return attr;
	}

	DWORD open(const wchar_t* SystemName, ACCESS_MASK DesiredAccess) {
		if (SystemName == nullptr) {
			return open_internal((PLSA_UNICODE_STRING)nullptr, DesiredAccess);
		}

		LSA_UNICODE_STRING lstr;
		if (!InitLsaString(&lstr, SystemName)) {
			return ERROR_INSUFFICIENT_BUFFER;
		}
		return open_internal(&lstr, DesiredAccess);
	}
	~LsaHandle() {
		if (handle != nullptr) {
			LsaClose(handle);
		}
	}

private:
	DWORD open_internal(PLSA_UNICODE_STRING SystemName, ACCESS_MASK DesiredAccess) {
		memset(&attr, 0, sizeof(attr));
		NTSTATUS status = LsaOpenPolicy(
			SystemName,
			&attr,
			DesiredAccess,
			&handle
		);
		if (status == 0)
			return 0;
		handle = nullptr;
		return LsaNtStatusToWinError(status);
	}
};

struct SID_deletor {
	void operator()(void* ptr) const {
		delete[](char*)ptr;
	}
};

std::unique_ptr<SID, SID_deletor> get_SID(
	const wchar_t* SystemName,
	const wchar_t* AccountName
) {
	DWORD sid_size = 0;
	DWORD domain_size = 0;
	SID_NAME_USE type;
	LookupAccountNameW(
		SystemName,
		AccountName,
		nullptr,
		&sid_size,
		nullptr,
		&domain_size,
		&type
	);

	std::unique_ptr<SID, SID_deletor> ptr;
	ptr.reset((SID*)new char[sid_size]);
	return ptr;
}

class Sid {
	SID* sid = nullptr;
	std::wstring domain;

public:
	SID* get_sid() const {
		return sid;
	}
	const std::wstring& get_domain() const {
		return domain;
	}

	DWORD open(
		const wchar_t* SystemName,
		const wchar_t* AccountName
	) {
		DWORD sid_size = 1;
		DWORD domain_size = 1;
		SID_NAME_USE type;

		domain.resize(domain_size);
		sid = (SID*)new char[sid_size];
		if (LookupAccountNameW(
			SystemName,
			AccountName,
			sid,
			&sid_size,
			&domain[0],
			&domain_size,
			&type
		)) {
			domain = domain.c_str();
			return 0;
		}

		domain.resize(domain_size);
		sid = (SID*)new char[sid_size];
		if (LookupAccountNameW(
			SystemName,
			AccountName,
			sid,
			&sid_size,
			&domain[0],
			&domain_size,
			&type
		)) {
			domain = domain.c_str();
			return 0;
		}

		wcout << "Failed" << endl;
		domain.clear();
		return GetLastError();
	}
	~Sid() {
		delete[](char*)sid;
	}
};

DWORD set_permission(LSA_HANDLE handle, SID* sid, const wchar_t* permission, bool enable) {
	LSA_UNICODE_STRING lstr;
	if (!InitLsaString(&lstr, permission)) {
		return ERROR_INSUFFICIENT_BUFFER;
	}
	NTSTATUS status = enable
		? LsaAddAccountRights(handle, sid, &lstr, 1)
		: LsaRemoveAccountRights(handle, sid, false, &lstr, 1);
	if (status != 0) {
		return LsaNtStatusToWinError(status);
	}
	return 0;
}
std::vector<std::wstring> get_user_specific_permissions(LSA_HANDLE handle, SID* sid) {
	std::vector<std::wstring> out;
	ULONG size;
	LSA_UNICODE_STRING* str;
	NTSTATUS status = LsaEnumerateAccountRights(handle, sid, &str, &size);
	if (status != 0) {
		return out;
	}
	try {
		for (ULONG c = 0; c < size; c++) {
			out.emplace_back(str[c].Buffer, str[c].Length);
		}
	}
	catch (...) {}
	LsaFreeMemory(str);
	return out;
}
bool add_permission_for_current_user(const wchar_t* permission) {
	wprintf(L"\n------------------------------------------------------------------------\n");
	wcout << "Retrieving Account Name...  ";
	std::wstring name = get_user_name();
	if (name.empty()) {
		wcout << "Failed" << endl;
		return false;
	}
	else {
		wcout << "Account Name: " << name.c_str() << endl;
	}

	wcout << "Opening LSA Handle...       ";
	LsaHandle handle;
	DWORD status = handle.open(nullptr, POLICY_ALL_ACCESS);
	if (status != 0) {
		wcout << "Failed: " << status << endl;
		return false;
	}
	else {
		wcout << "Success" << endl;
	}

	wcout << "Obtaining Security ID...    ";
	Sid sid;
	status = sid.open(nullptr, name.c_str());
	if (status != 0) {
		wcout << "Failed: " << status << endl;
		return false;
	}
	else {
		wcout << "Domain: " << sid.get_domain() << endl;
	}

	wcout << "Adding permission for: " << permission << endl;
	status = set_permission(handle.get_handle(), sid.get_sid(), permission, true);
	if (status != 0) {
		wcout << "Failed" << endl;
		return false;
	}
	else {
		wcout << "Success." << endl;
	}
	wcout << endl;

	wcout << "Enumerating User-specific Permissions..." << endl;
	auto permissions = get_user_specific_permissions(handle.get_handle(), sid.get_sid());
	for (const std::wstring& str : permissions) {
		wcout << "  - " << str.c_str() << endl;
	}

	wcout << endl;

	return true;
}
