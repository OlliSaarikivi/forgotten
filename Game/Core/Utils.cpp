#include <Core/stdafx.h>
#include "Utils.h"

std::wstring formatLastWin32Error(std::wstring file, int line, std::wstring function) {
	wchar_t* lpMsgBuf;
	DWORD dw = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	auto formatted = fmt::format(L"{}({}): error {} in {}: {}", file, line, dw, function, lpMsgBuf);
	LocalFree(lpMsgBuf);
	return formatted;
}

std::wstring formatError(std::wstring file, int line, std::wstring message) {
	return fmt::format(L"{}({}): error: {}", file, line, message);
}

void exitWithMessage(std::wstring message) {
	MessageBox(NULL, message.c_str(), L"Error", MB_OK);
	exit(1);
}
