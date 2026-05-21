#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

// Function to find process ID by executable name
DWORD GetProcId(const wchar_t* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32FirstW(hSnap, &procEntry)) {
            do {
                if (_wcsicmp(procEntry.szExeFile, procName) == 0) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

int main() {
    const wchar_t* processName = L"Flipping is Hard Demo.exe";
    const wchar_t* dllName = L"trainer.dll";

    std::wcout << L"=========================================" << std::endl;
    std::wcout << L"         Unity DLL Injector              " << std::endl;
    std::wcout << L"=========================================" << std::endl;

    std::wcout << L"[i] Searching for process: " << processName << L"..." << std::endl;
    DWORD procId = GetProcId(processName);

    if (procId == 0) {
        // Fallback check: try without .exe or with alternate names just in case
        procId = GetProcId(L"Flipping is Hard Demo");
    }

    if (procId == 0) {
        std::wcerr << L"[!] ERROR: Process not found! Make sure the game is running." << std::endl;
        return 1;
    }

    std::wcout << L"[+] Success! Found process ID: " << procId << std::endl;

    // Get the absolute path of the DLL to inject
    wchar_t dllAbsolutePath[MAX_PATH];
    if (GetFullPathNameW(dllName, MAX_PATH, dllAbsolutePath, nullptr) == 0) {
        std::wcerr << L"[!] ERROR: Could not resolve absolute path for " << dllName << std::endl;
        return 1;
    }

    std::wcout << L"[i] DLL Absolute Path: " << dllAbsolutePath << std::endl;

    // Open target process
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProc) {
        std::wcerr << L"[!] ERROR: Could not open process with ALL_ACCESS. Try running as Administrator." << std::endl;
        return 1;
    }

    // Allocate memory in target process for DLL path
    size_t pathSize = (wcslen(dllAbsolutePath) + 1) * sizeof(wchar_t);
    void* loc = VirtualAllocEx(hProc, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        std::wcerr << L"[!] ERROR: VirtualAllocEx failed in target process." << std::endl;
        CloseHandle(hProc);
        return 1;
    }

    // Write DLL path to allocated memory
    if (!WriteProcessMemory(hProc, loc, dllAbsolutePath, pathSize, nullptr)) {
        std::wcerr << L"[!] ERROR: WriteProcessMemory failed." << std::endl;
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 1;
    }

    std::wcout << L"[i] Wrote DLL path to target process memory." << std::endl;

    // Get address of LoadLibraryW in kernel32.dll
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    if (!loadLibraryAddr) {
        std::wcerr << L"[!] ERROR: Could not get address of LoadLibraryW from kernel32.dll." << std::endl;
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 1;
    }

    // Create remote thread in target process that executes LoadLibraryW(dllAbsolutePath)
    std::wcout << L"[i] Creating remote thread..." << std::endl;
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, loc, 0, nullptr);
    if (!hThread) {
        std::wcerr << L"[!] ERROR: CreateRemoteThread failed. Win32 Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 1;
    }

    // Wait for the remote thread to complete
    std::wcout << L"[i] Waiting for injection thread to finish..." << std::endl;
    WaitForSingleObject(hThread, INFINITE);

    // Get the return value of LoadLibraryW (the base address of injected DLL)
    DWORD hLibModule = 0;
    GetExitCodeThread(hThread, &hLibModule);

    if (hLibModule == 0) {
        std::wcerr << L"[!] ERROR: Remote LoadLibraryW returned 0 (failed to load DLL)." << std::endl;
    } else {
        std::wcout << L"[+] SUCCESS! DLL injected successfully into game process." << std::endl;
    }

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
    CloseHandle(hProc);

    return (hLibModule == 0) ? 1 : 0;
}
