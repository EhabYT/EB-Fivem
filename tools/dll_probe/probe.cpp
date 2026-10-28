// tools/dll_probe/probe.cpp - minimal injection probe (row 243, guide item 6.4).
//
// Purpose: isolate "does injection work at all" from "does EBengin UI.dll
// crash".  Inject this DLL with the exact same injector and timing you use
// for EBengin UI.dll.  If %TEMP%\dll_probe.log gains a line, injection works
// and any crash lives in the full DLL; if no line appears, the problem is the
// injection method, timing, or protection software - not this repository.
//
// Build (x64 Native Tools Command Prompt; standalone on purpose - the probe is
// NOT part of EBengin UI.sln and must never ship next to the game):
//   cl /nologo /LD /MD /O2 /GS /EHsc /DYNAMICBASE /HIGHENTROPYVA ^
//      tools\dll_probe\probe.cpp /Fe:dll_probe.dll
//
// Deviation from the guide: its example calls MessageBoxA inside DllMain.  A
// message box under the loader lock can deadlock, and this repository's own
// minimal-DllMain rule (row 236 checklist) forbids work there - so DllMain
// only creates a worker thread, and the thread (running after the lock is
// released) writes the log line.  Same signal, no loader-lock UI.

#include <windows.h>

static DWORD WINAPI ProbeThread(LPVOID)
{
	char path[MAX_PATH] = { 0 };
	if (ExpandEnvironmentStringsA("%TEMP%\\dll_probe.log", path, MAX_PATH) == 0)
		return 1;

	SYSTEMTIME st;
	GetLocalTime(&st);
	char line[192];
	wsprintfA(line,
		"dll_probe: DLL_PROCESS_ATTACH seen, pid %u, %04u-%02u-%02u %02u:%02u:%02u\r\n",
		GetCurrentProcessId(), st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond);

	HANDLE file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 1;
	SetFilePointer(file, 0, NULL, FILE_END);
	DWORD written = 0;
	WriteFile(file, line, (DWORD)lstrlenA(line), &written, NULL);
	CloseHandle(file);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		HANDLE thread = CreateThread(NULL, 0, ProbeThread, NULL, 0, NULL);
		if (thread != NULL)
			CloseHandle(thread);
	}
	return TRUE;
}
