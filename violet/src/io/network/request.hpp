#pragma once
#include <windows.h>
#include <wininet.h>
#include <string>
#include <vector>
#include "../../security/obfuscator.hpp"

namespace network {

    typedef HINTERNET(APIENTRY* PINTERNET_OPEN_A)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
    typedef HINTERNET(APIENTRY* PINTERNET_OPEN_URL_A)(HINTERNET, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
    typedef BOOL(APIENTRY* PINTERNET_READ_FILE)(HINTERNET, LPVOID, DWORD, LPDWORD);
    typedef BOOL(APIENTRY* PINTERNET_CLOSE_HANDLE)(HINTERNET);

    inline std::string send_request(const std::string& url) {
        static HMODULE hWininet = LoadLibraryA(HIDE_STR("wininet.dll").c_str());
        if (!hWininet) return "";

        static auto pInternetOpenA = (PINTERNET_OPEN_A)GetProcAddress(hWininet, HIDE_STR("InternetOpenA").c_str());
        static auto pInternetOpenUrlA = (PINTERNET_OPEN_URL_A)GetProcAddress(hWininet, HIDE_STR("InternetOpenUrlA").c_str());
        static auto pInternetReadFile = (PINTERNET_READ_FILE)GetProcAddress(hWininet, HIDE_STR("InternetReadFile").c_str());
        static auto pInternetCloseHandle = (PINTERNET_CLOSE_HANDLE)GetProcAddress(hWininet, HIDE_STR("InternetCloseHandle").c_str());

        if (!pInternetOpenA || !pInternetOpenUrlA || !pInternetReadFile || !pInternetCloseHandle) return "";

        HINTERNET hInternet = pInternetOpenA(HIDE_STR("Chromium/6.1").c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hConnect = pInternetOpenUrlA(hInternet, url.c_str(),
            HIDE_STR("Content-Type: application/json\r\n").c_str(),
            -1,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE,
            0);

        if (!hConnect) {
            pInternetCloseHandle(hInternet);
            return "";
        }

        std::string response;
        char buffer[4096];
        DWORD bytesRead;
        while (pInternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        pInternetCloseHandle(hConnect);
        pInternetCloseHandle(hInternet);
        return response;
    }
}
