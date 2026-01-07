// singleinstance.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

/**
This app allows to select multiple files and launch one instance of command process from windows explorer's context menu.

Example of usage:

Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge]
"MultiSelectModel"="Player"

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge\Command]
@="\"d:\\singleinstance.exe\" %1 \"C:\\Program Files\\Perforce\\p4merge.exe\" $files --si-timeout 400"

OR

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge\Command]
@="\"d:\\singleinstance_hidden.exe\" %1 \"C:\\Program Files\\Perforce\\p4merge.exe\" $files --si-timeout 400"

*/
#include <Shellapi.h>
#include <atomic>
#include <vector>
#include <string>
#include <regex>

class CLimitSingleInstance {
    protected:
        DWORD  m_dwLastError;
        HANDLE m_hMutex;

    public:
        CLimitSingleInstance(TCHAR *strMutexName)
        {
            m_hMutex = CreateMutex(NULL, FALSE, strMutexName); //do early
            m_dwLastError = GetLastError(); //save for use later...
        }

        ~CLimitSingleInstance(){
            if (m_hMutex)  //Do not forget to close handles.
            {
                CloseHandle(m_hMutex); //Do as late as possible.
                m_hMutex = NULL; //Good habit to be in.
            }
        }

        BOOL IsAnotherInstanceRunning(){
            return (ERROR_ALREADY_EXISTS == m_dwLastError);
        }
};

DWORD timeout = 400; // ms
enum { TIMER_ID = 1 };
LPTSTR *szArgList;
int argCount;

// Give each version a unique name and UUID
#ifdef BUILD_HIDDEN_VERSION
    #define EXE_NAME L"singleinstance_hidden.exe"
    #define APP_TITLE L"singleinstance (Hidden)"
    // Unique UUID for the Hidden version
    CLimitSingleInstance singleInstance(TEXT("Global\\{C30D92DD-DEE6-41A1-8907-B42FBE58C8A7}"));
    // Unique Window Class so they don't talk to each other
    TCHAR szWindowClass[256] = _T("SingleInstanceHidden_WindowClass");
#else
    #define EXE_NAME L"singleinstance.exe"
    #define APP_TITLE L"singleinstance"
    // Original UUID for the Regular version
    CLimitSingleInstance singleInstance(TEXT("Global\\{C30D92DD-DEE6-41A1-8907-B42FBE58C8A6}"));
    // Original Window Class
    TCHAR szWindowClass[256] = _T("SingleInstance_WindowClass");
#endif

std::vector<std::wstring> files;
HINSTANCE hInst;                                // current instance

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
    for (int i = 0; i < argCount; i++) {
        if (!lstrcmp(szArgList[i], _T("--si-timeout")) && i + 1 < argCount ) {
            timeout = static_cast<DWORD>(std::stoul(szArgList[i + 1]));
        }
    }

	if (singleInstance.IsAnotherInstanceRunning()) {
			// Move startTime OUTSIDE the loop so it represents the actual start of the wait
			DWORD startTime = GetTickCount(); 
			for (;;) {
				HWND wnd = FindWindow(szWindowClass, NULL);
				if (wnd) {
					LPCTSTR lpszString = szArgList[1];
					COPYDATASTRUCT cds;
					cds.dwData = 1; // can be anything
					cds.cbData = sizeof(TCHAR) * (_tcslen(lpszString) + 1);
					cds.lpData = (void*)lpszString;
					SendMessage(wnd, WM_COPYDATA, (WPARAM)wnd, (LPARAM)(LPVOID)&cds);
					break;
				} else {
					Sleep(50);
					// GetTickCount() and startTime are both DWORD (unsigned). 
					// If 'timeout' is also a DWORD, the C4018 warning is resolved.
					if (GetTickCount() - startTime > timeout) {
						break; // failure
					}
				}
			}
			LocalFree(szArgList);
			return 0;
		}
       
    if (szArgList  ) {
        if (argCount > 3) {
            files.push_back(szArgList[1]);
        } else {
			std::wstring usageText = L"Usage: ";
            usageText += EXE_NAME;
            usageText += L" \"%1\" <command> $files [arguments]\r\n\r\n"
                         L"Optional arguments for singleinstance (not passed to command):\r\n"
                         L"--si-timeout <time to wait in msecs>";

            MessageBox(0, usageText.c_str(), APP_TITLE, 0);
            return 0;
        }
    }
   
    MSG msg;

    // Initialize global strings
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    LocalFree(szArgList);
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm = 0;

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable
   hWnd = CreateWindow(szWindowClass, _T("SingleInstance"), WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   return TRUE;
}

void ArgvQuote(const std::wstring& Argument, std::wstring& CommandLine, bool Force, bool singlequote){
    wchar_t mark;
    if (singlequote) {
        mark = L'\'';
    } else {
        mark = L'"';
    }
    if (Force == false &&
        Argument.empty() == false &&
        Argument.find_first_of(L" \t\n\v\"") == Argument.npos) {
        CommandLine.append(Argument);
    } else {
        CommandLine.push_back(mark);

        for (auto It = Argument.begin();; ++It) {
            unsigned NumberBackslashes = 0;

            while (It != Argument.end() && *It == L'\\') {
                ++It;
                ++NumberBackslashes;
            }

            if (It == Argument.end()) {
                // Escape all backslashes, but let the terminating
                // double quotation mark we add below be interpreted
                // as a metacharacter.
                CommandLine.append(NumberBackslashes * 2, L'\\');
                break;
            } else if (*It == L'"') {
                //
                // Escape all backslashes and the following
                // double quotation mark.
                //
                CommandLine.append(NumberBackslashes * 2 + 1, L'\\');
                CommandLine.push_back(*It);
            } else {
                //
                // Backslashes aren't special here.
                //
                CommandLine.append(NumberBackslashes, L'\\');
                CommandLine.push_back(*It);
            }
        }

        CommandLine.push_back(mark);
    }
    CommandLine.push_back(L' ');
}
#ifdef BUILD_HIDDEN_VERSION
	// This launches the .exe using CreateProcessW instead of ShellExecute, it creates a fully hidden console.
	void LaunchApp() {
		// szArgList[2] is our target app (e.g., py.exe)
		std::wstring targetApp = szArgList[2];
		
		// Build the full command line string
		std::wstring fullCmdLine = L"\"" + targetApp + L"\" ";
		
		std::wstring argsPart;
		for (int i = 3; i < argCount; i++) {
			std::wstring argStr = szArgList[i];
			std::size_t found = argStr.find(L"$files");
			
			if (found != std::string::npos) {
				std::wstring tmpstr;
				for (const auto& file : files) {
					ArgvQuote(file, tmpstr, true, true);
				}
				argStr = std::regex_replace(argStr, std::wregex(L"\\$files"), tmpstr);
				argsPart.append(argStr);
			} else if (argStr == L"--si-timeout") {
				i++; // skip the timeout value
			} else {
				ArgvQuote(argStr, argsPart, true, false);
			}
		}
		
		fullCmdLine.append(argsPart);

		// Prepare CreateProcess structures
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		
		// Optional: Debugging for failed launches
		// MessageBox(0, fullCmdLine.c_str(), L"Debug Hidden Command", 0);
		
		// Launch with CREATE_NO_WINDOW (0x08000000)
		// This forces the console-less environment that stops flashes from console applications
		BOOL success = CreateProcessW(
			NULL,               // No module name (taken from command line)
			&fullCmdLine[0],    // Mutable command line buffer
			NULL,               // Process handle not inheritable
			NULL,               // Thread handle not inheritable
			FALSE,              // Set handle inheritance to FALSE
			0x08000000,         // CREATE_NO_WINDOW flag
			NULL,               // Use parent's environment block
			NULL,               // Use parent's starting directory 
			&si,                // Pointer to STARTUPINFO
			&pi                 // Pointer to PROCESS_INFORMATION
		);

		if (success) {
			// We don't need to wait for the app, so close handles to avoid leaks
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		} else {
			// Optional: Debugging for failed launches
			// DWORD err = GetLastError();
		}
	}
#else
	void LaunchApp() {
		std::wstring cmdLine;
	   
		for (int i = 3; i < argCount; i++) {
			std::wstring argStr = szArgList[i];
			std::size_t found = argStr.find(L"$files");
			if (found!=std::string::npos) {
				std::wstring tmpstr;
				for (const auto& file : files) {
					ArgvQuote(file, tmpstr, true, true);
				}
				argStr = std::regex_replace(argStr, std::wregex(L"\\$files"), tmpstr);
				cmdLine.append(argStr);
			} else if (argStr == L"--si-timeout") {
				i++; // skip
			} else {
				ArgvQuote(argStr, cmdLine, true, false);
			}
		}
		// MessageBox(0, cmdLine.c_str(), szArgList[2], 0);
		HINSTANCE hinst =  ShellExecute(0, _T("open"), szArgList[2], cmdLine.c_str(), 0, SW_SHOWNORMAL);
		if (reinterpret_cast<int>(hinst) <= 32) {
			TCHAR buffer[256];
			wsprintf(buffer, _T("ShellExecute failed. Error code=%d"), reinterpret_cast<int>(hinst));
		}
	}
#endif	

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    COPYDATASTRUCT* pcds;

    switch (message)
    {
    case WM_CREATE:
        SetTimer(hWnd, TIMER_ID, timeout, 0);
        break;
    case WM_COPYDATA:
        SetTimer(hWnd, TIMER_ID, timeout, 0);
        pcds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
        if (pcds->dwData == 1) {  
            LPCTSTR lpszString = reinterpret_cast<LPCTSTR>(pcds->lpData);
            files.push_back(lpszString);
        }
        break;
    case WM_TIMER:
        KillTimer(hWnd, TIMER_ID);
        LaunchApp();
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}