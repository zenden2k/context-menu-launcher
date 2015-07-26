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

*/
#include <Shellapi.h>
#include <vector>
#include <string>

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

int timeout = 400; // ms
enum { TIMER_ID = 1 };
LPTSTR *szArgList;
int argCount;
CLimitSingleInstance singleInstance(TEXT("Global\\{C30D92DD-DEE6-41A1-8907-B42FBE58C8A6}"));
std::vector<std::wstring> files;
HINSTANCE hInst;                                // current instance
TCHAR szWindowClass[256] = _T("SingleInstance_WindowClass");            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
    for (int i = 0; i < argCount; i++) {
        if (!lstrcmp(szArgList[i], _T("--si-timeout")) && i + 1 < argCount ) {
            timeout = std::stoi(szArgList[i + 1]);
        }
    }

    if (singleInstance.IsAnotherInstanceRunning()) {
        for (;;) {
            DWORD startTime = GetTickCount();
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
                if (GetTickCount() - startTime > timeout ) {
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
            MessageBox(0, L"Usage: singleinstance.exe \"%1\" <command> $files [arguments]\r\n\r\n"
                L"Optional arguments for singleinstance (not passed to command):\r\n"
                L"--si-timeout <time to wait in msecs>"
                , _T("singleinstance"), 0);
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

void ArgvQuote(const std::wstring& Argument, std::wstring& CommandLine, bool Force){
    if (Force == false &&
        Argument.empty() == false &&
        Argument.find_first_of(L" \t\n\v\"") == Argument.npos) {
        CommandLine.append(Argument);
    } else {
        CommandLine.push_back(L'"');

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

        CommandLine.push_back(L'"');
    }
    CommandLine.push_back(L' ');
}

void LaunchApp() {
    std::wstring cmdLine;
   
    for (int i = 3; i < argCount; i++) {
        if (!lstrcmp(szArgList[i], _T("$files"))) {
            for (const auto& file : files) {
                ArgvQuote(file, cmdLine, true);
            }
        } else if (!lstrcmp(szArgList[i], _T("--si-timeout"))) {
            i++; // skip
        }
        else {
            ArgvQuote(szArgList[i], cmdLine, true);
        }
    }
    //MessageBox(0, cmdLine.c_str(), szArgList[2], 0);
    HINSTANCE hinst =  ShellExecute(0, _T("open"), szArgList[2], cmdLine.c_str(), 0, SW_SHOWNORMAL);
    if (reinterpret_cast<int>(hinst) <= 32) {
        TCHAR buffer[256];
        wsprintf(buffer, _T("ShellExecute failed. Error code=%d"), reinterpret_cast<int>(hinst));
    }
}

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