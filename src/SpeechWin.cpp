/** SPEECH ON WINDOWS, AS A PROCESS WE KEEP HOLD OF.

A reading used to be started with `start /b powershell …` and forgotten. Nothing could then stop
it, so every click started another voice over the one still talking — reported on the forum by
DaveVenom. Killing powershell.exe by name would take down whatever else the user is running in
one, so the reading is started here with CreateProcess instead, its handle is kept, and a new
reading, a click on the note or Escape ends exactly that process and no other.

IN A FILE OF ITS OWN because windows.h defines names — min, max, DrawText and others — that
collide with Rack's headers, so it is kept away from them. Compiled to nothing elsewhere.

UNTESTED ON WINDOWS HERE: there is no Windows machine in this project. It builds in the VCV
toolchain, and it fails the way the old code did: if PowerShell will not start, there is silence.
*/
#if defined ARCH_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>

static HANDLE gSpeech = NULL;

bool helpWinSpeaking() {
	return gSpeech && WaitForSingleObject(gSpeech, 0) == WAIT_TIMEOUT;
}

void helpWinSilence() {
	if (!gSpeech)
		return;
	if (WaitForSingleObject(gSpeech, 0) == WAIT_TIMEOUT)
		TerminateProcess(gSpeech, 0);
	CloseHandle(gSpeech);
	gSpeech = NULL;
}

void helpWinSay(const std::string& commandLine) {
	helpWinSilence();
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	// CreateProcessA may write into the command line, so it gets a copy it is allowed to change.
	std::string line = commandLine;
	if (CreateProcessA(NULL, &line[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		gSpeech = pi.hProcess;
	}
}
#endif
