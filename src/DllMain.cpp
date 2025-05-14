#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "../version/VersionInfo.h"
#include "Memory/MemoryUtility.h"
#include "StaticPatches.h"

char g_VersionStr[100] = "%s %s Compiled + PluginLoader v";
std::vector<HWND> g_AppWindows;
DWORD g_AppProcessId;

struct DllInfo
{
    std::wstring file;
    std::wstring path;
    bool operator<(const DllInfo& other) const { return file < other.file; }
};

bool FindPlugins(const std::wstring& directoryPath, std::vector<DllInfo>& dllList)
{
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::wstring searchPattern = directoryPath + L"\\*.dll";
    hFind = FindFirstFileW(searchPattern.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"FindFirstFileW failed for: " << searchPattern << " Error: " << GetLastError() << std::endl;
        return false;
    }

    std::wstring dirPath = directoryPath;
    PathRemoveBackslashW(&dirPath[0]);

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            DllInfo dllInfo;
            dllInfo.path = dirPath;
            dllInfo.file = findFileData.cFileName;
            std::vector<DllInfo>::iterator it = std::lower_bound(dllList.begin(), dllList.end(), dllInfo);
            dllList.insert(it, dllInfo);
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        std::wcerr << L"FindNextFileW error: " << GetLastError() << std::endl;
        FindClose(hFind);
        return false;
    }

    FindClose(hFind);
    return true;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD windowProcessId;
    GetWindowThreadProcessId(hWnd, &windowProcessId);

    if (windowProcessId == g_AppProcessId)
        g_AppWindows.push_back(hWnd);

    return TRUE;
}

std::vector<HWND> GetAllWindowsOfCurrentProcess()
{
    g_AppWindows.clear();
    g_AppProcessId = GetCurrentProcessId();
    EnumWindows(EnumWindowsProc, 0);
    return g_AppWindows;
}

void HideVanguardConsole(bool showhide)
{
    std::vector<HWND> windowList = GetAllWindowsOfCurrentProcess();

    if (!windowList.empty())
    {
        for (std::vector<HWND>::const_iterator it = windowList.begin(); it != windowList.end(); ++it)
        {
            HWND hWnd = *it;
            char windowTitle[256];
            if (!GetWindowTextA(hWnd, windowTitle, 256))
                continue;
            if (hWnd == GetConsoleWindow() && showhide)
                ShowWindow(hWnd, SW_HIDE);
        }
    }
}

void VersionWrite(DWORD dwAddr1, DWORD dwAddr2, DWORD dwAddr3, DWORD dwAddr4)
{
    int offset = strlen(g_VersionStr);

    sprintf(g_VersionStr + offset, "%d", PRODUCT_VERSION_MAJOR);
    offset = strlen(g_VersionStr);

    #if (PRODUCT_VERSION_MINOR > 0 || PRODUCT_VERSION_PATCH > 0 || PRODUCT_VERSION_BUILD > 0)
        sprintf(g_VersionStr + offset, ".%d", PRODUCT_VERSION_MINOR);
        offset = strlen(g_VersionStr);
    #endif

    #if (PRODUCT_VERSION_PATCH > 0 || PRODUCT_VERSION_BUILD > 0)
        sprintf(g_VersionStr + offset, ".%d", PRODUCT_VERSION_PATCH);
        offset = strlen(g_VersionStr);
    #endif

    #if (PRODUCT_VERSION_BUILD > 0)
        sprintf(g_VersionStr + offset, ".%d", PRODUCT_VERSION_BUILD);
        offset = strlen(g_VersionStr);
    #endif

    MEMUTIL_WRITE_VALUE(const char*, dwAddr1 + 1, g_VersionStr)
    MEMUTIL_WRITE_VALUE(const char*, dwAddr2 + 1, g_VersionStr)
    MEMUTIL_WRITE_VALUE(const char*, dwAddr3 + 1, g_VersionStr)
    MEMUTIL_WRITE_VALUE(const char*, dwAddr4 + 1, g_VersionStr)
}

void LoadDLL(const char* dllFilePath, const char* title = "Load")
{
    char errorMsg[1024], titleStr[20];
    const std::string& pathString(dllFilePath);
    size_t lastSeparator = pathString.find_last_of("\\/");
    sprintf(titleStr, "%s Error", title);

    if (std::string::npos == lastSeparator) {
        sprintf(errorMsg, "Failed to load %s", pathString.c_str());
    } else {
        sprintf(errorMsg, "Failed to load %s", pathString.substr(lastSeparator + 1).c_str());
    }

    HMODULE loadLibrary = LoadLibrary(dllFilePath);
    if (!loadLibrary)
    {
        MessageBoxA(NULL, _T(errorMsg), _T(titleStr), MB_ICONERROR | MB_OK);
        FreeLibrary(loadLibrary);
        exit(1);
    }
}

void LoadDLL(const std::string& dllDir, const char* moduleName, const char* title)
{
    std::vector<DllInfo> dllList;

    FindPlugins(TO_WSTRING(dllDir), dllList);

    if (dllList.empty())
        return;

    for (std::vector<DllInfo>::const_iterator it = dllList.begin(); it != dllList.end(); ++it)
    {
        char fullpath[MAX_PATH];
        const std::string& path = TO_STRING(it->path);
        const std::string& file = TO_STRING(it->file);

        sprintf(fullpath, "%s\\%s", path.c_str(), file.c_str());

        if (!std::strchk(file).startwith(moduleName))
            continue;

        LoadDLL(fullpath, title);
    }
}

void APIENTRY LibraryLoader()
{
    TCHAR szFullPath[MAX_PATH], szDriveLetter[MAX_PATH], szDirectory[MAX_PATH], szFileName[MAX_PATH],
          szPluginDIR[MAX_PATH], szINI[MAX_PATH];

    GetModuleFileName(NULL, szFullPath, MAX_PATH);

    _splitpath(szFullPath, szDriveLetter, szDirectory, szFileName, NULL);

    sprintf(szPluginDIR, "%s%sPlugins", szDriveLetter, szDirectory);

    if (std::strchk(szFullPath).endswith("agentserver.exe"))
    {
        VersionWrite(0x0046F06C, 0x0046F9C0, 0x0048B072, 0x0048B67A);

        CStaticPatches::AgentServerCertPatch();

        if (std::file(".\\Vanguard\\AgentServer.dll").exists())
        {
            LoadDLL(".\\Vanguard\\AgentServer.dll", "Vanguard");

            if (std::file(".\\Vanguard\\AgentServer.ini").exists())
            {
                std::inifile vg(".\\Vanguard\\AgentServer.ini");
                const std::string &sc(vg.ReadString(szFileName, "HideConsole", "FALSE"));
                HideVanguardConsole(std::strchk(sc).equal("true"));
            }
        }

        sprintf(szINI, "%s%sPlugins.ini", szDriveLetter, szDirectory);

        if (std::file(szINI).exists())
        {
            std::inifile config(szINI);

            const std::string& dllFile = config.ReadString(szFileName, "DllFile", "NULL");

            if (dllFile != "NULL" && std::file(dllFile).exists())
            {
                size_t lastSeparator = dllFile.find_last_of("\\/");

                if (lastSeparator == std::string::npos)
                    return;

                LoadDLL(dllFile.substr(0, lastSeparator), szFileName, "Plugin");
            }
        }
        else LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("downloadserver.exe"))
    {
        VersionWrite(0x0144D2EC, 0x014862B0, 0x01486852, 0x014872AA);

        CStaticPatches::DownloadServerCertPatch();

        if (std::file(".\\Vanguard\\DownloadServer.dll").exists())
        {
            LoadDLL(".\\Vanguard\\DownloadServer.dll", "Vanguard");

            if (std::file(".\\Vanguard\\DownloadServer.ini").exists())
            {
                std::inifile vg(".\\Vanguard\\DownloadServer.ini");
                const std::string &sc(vg.ReadString(szFileName, "HideConsole", "FALSE"));
                HideVanguardConsole(std::strchk(sc).equal("true"));
            }
        }

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("farmmanager.exe"))
    {
        VersionWrite(0x0145583C, 0x014938B0, 0x01493E52, 0x0149445A);

        CStaticPatches::FarmManagerCertPatch();

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("gatewayserver.exe"))
    {
        VersionWrite(0x014CD38C, 0x014D0490, 0x014DD122, 0x014DDB7A);

        CStaticPatches::GatewayServerCertPatch();

        if (std::file(".\\Vanguard\\GatewayServer.dll").exists())
        {
            LoadDLL(".\\Vanguard\\GatewayServer.dll", "Vanguard");

            if (std::file(".\\Vanguard\\GatewayServer.ini").exists())
            {
                std::inifile vg(".\\Vanguard\\GatewayServer.ini");
                const std::string &sc(vg.ReadString(szFileName, "HideConsole", "FALSE"));
                HideVanguardConsole(std::strchk(sc).equal("true"));
            }
        }

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("globalmanager.exe"))
    {
        VersionWrite(0x0172BB5C, 0x01735A60, 0x01750342, 0x0175094A);

        CStaticPatches::GlobalManagerCertPatch();

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("machinemanager.exe"))
    {
        VersionWrite(0x0148D09C, 0x014CEE20, 0x014CF3C2, 0x014CF91A);

        CStaticPatches::MachineManagerCertPatch();

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("gameserver.exe"))
    {
        VersionWrite(0x00BC9A8C, 0x00BCDD40, 0x00BF94B2, 0x00BF9A2A);

        CStaticPatches::GameServerCertPatch();

        if (std::file(".\\Vanguard\\GameServer.dll").exists())
        {
            LoadDLL(".\\Vanguard\\GameServer.dll", "Vanguard");

            if (std::file(".\\Vanguard\\GameServer.ini").exists())
            {
                std::inifile vg(".\\Vanguard\\GameServer.ini");
                const std::string &sc(vg.ReadString("GameServer", "HideConsole", "FALSE"));
                HideVanguardConsole(std::strchk(sc).equal("true"));
            }
        }

        sprintf(szINI, "%s%sPlugins.ini", szDriveLetter, szDirectory);

        if (std::file(szINI).exists())
        {
            std::inifile config(szINI);

            const std::string& dllFile = config.ReadString(szFileName, "DllFile", "NULL");

            if (dllFile != "NULL" && std::file(dllFile).exists())
            {
                size_t lastSeparator = dllFile.find_last_of("\\/");

                if (lastSeparator == std::string::npos)
                    return;

                LoadDLL(dllFile.substr(0, lastSeparator), szFileName, "Plugin");
            }
        }
        else LoadDLL(szPluginDIR, szFileName, "Plugin");
    }

    if (std::strchk(szFullPath).endswith("shardmanager.exe"))
    {
        VersionWrite(0x009F113C, 0x009F1FC0, 0x00A034C2, 0x00A03A3A);

        CStaticPatches::ShardManagerCertPatch();

        if (std::file(".\\Vanguard\\ShardManager.dll").exists())
        {
            LoadDLL(".\\Vanguard\\ShardManager.dll", "Vanguard");

            if (std::file(".\\Vanguard\\ShardManager.ini").exists())
            {
                std::inifile vg(".\\Vanguard\\ShardManager.ini");
                const std::string &sc(vg.ReadString("ShardManager", "HideConsole", "FALSE"));
                HideVanguardConsole(std::strchk(sc).equal("true"));
            }
        }

        LoadDLL(szPluginDIR, szFileName, "Plugin");
    }
}

EXTERN_C __declspec(dllexport) VOID PluginLoader() { return; }

BOOL APIENTRY DllMain(HMODULE, CONST DWORD ul_reason_for_call, LPVOID)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            LibraryLoader();
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default:
            break;
    }

    return TRUE;
}
