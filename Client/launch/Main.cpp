/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        launch/Main.cpp
 *  PURPOSE:     Unchanging .exe that doesn't change
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include <string>
#include <curl/include/curl/curl.h>
#include <iostream>
#include <fstream>
#pragma comment(lib, "libcurl")

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/*
    IMPORTANT

    If this project changes, a new release build should be copied into
    the Shared\data\launchers directory.

    The .exe in Shared\data\launchers will be used by the installer and updater.

    (set flag.new_client_exe on the build server to generate new exe)
*/

///////////////////////////////////////////////////////////////
//
// WinMain
//
//
//
///////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    CURL*       curl;
    CURLcode    res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://gtastolica.ru/connect.txt");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        std::ofstream myfile;
        myfile.open(CalcMTASAPath("connect.txt"));
        myfile << readBuffer;
        myfile.close();
    }

    if (!IsWindowsXPSP3OrGreater())
    {
        BrowseToSolution("launch-xpsp3-check", ASK_GO_ONLINE, "This version of MTA requires Windows XP SP3 or later");
        return 1;
    }

    // Load the loader.dll and continue the load
#ifdef MTA_DEBUG
    SString strLoaderDllFilename = "loader_d.dll";
#else
    SString strLoaderDllFilename = "loader.dll";
#endif

    SString strMTASAPath = PathJoin(GetLaunchPath(), "mta");
    SString strLoaderDllPathFilename = PathJoin(strMTASAPath, strLoaderDllFilename);

    // No Windows error box during first load attempt
    DWORD   dwPrevMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HMODULE hModule = LoadLibraryW(FromUTF8(strLoaderDllPathFilename));
    DWORD   dwLoadLibraryError = GetLastError();
    SetErrorMode(dwPrevMode);

    if (!hModule)
    {
        // Retry using MTA current directory
        SetCurrentDirectoryW(FromUTF8(strMTASAPath));
        hModule = LoadLibraryW(FromUTF8(strLoaderDllPathFilename));
        dwLoadLibraryError = GetLastError();
        if (hModule)
        {
            AddReportLog(5712, SString("LoadLibrary '%s' succeeded on change to directory '%s'", *strLoaderDllFilename, *strMTASAPath));
        }
    }

    int iReturnCode = 0;
    if (hModule)
    {
        // Find and call DoWinMain
        typedef int (*PFNDOWINMAIN)(HINSTANCE, HINSTANCE, LPSTR, int);
        PFNDOWINMAIN pfnDoWinMain = static_cast<PFNDOWINMAIN>(static_cast<PVOID>(GetProcAddress(hModule, "DoWinMain")));
        if (pfnDoWinMain)
            iReturnCode = pfnDoWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

        FreeLibrary(hModule);
    }
    else
    {
        iReturnCode = 1;
        SString strError = GetSystemErrorMessage(dwLoadLibraryError);
        SString strMessage("Failed to load: '%s'\n\n%s", *strLoaderDllPathFilename, *strError);
        AddReportLog(5711, strMessage);

        // Error could be due to missing VC Redist.
        // Online help page will have VC Redist download link.
        BrowseToSolution("loader-dll-not-loadable", ASK_GO_ONLINE, strMessage);
    }

    return iReturnCode;
}
