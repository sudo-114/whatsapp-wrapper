#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <wrl.h>
#include <WebView2.h>
#include "window.h"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {
constexpr wchar_t kClassName[] = L"WhatsAppWrapperWindow";
constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT WM_SWITCH_PROFILE = WM_APP + 2;
constexpr UINT ID_TRAY = 1001;
constexpr UINT ID_RELOAD = 2001;
constexpr UINT ID_ZOOM_IN = 2002;
constexpr UINT ID_ZOOM_OUT = 2003;
constexpr UINT ID_ACCOUNT_1 = 2004;
constexpr UINT ID_ACCOUNT_2 = 2005;
constexpr UINT ID_EXIT = 2006;

HWND g_hwnd = nullptr;
NOTIFYICONDATAW g_nid{};
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
ComPtr<ICoreWebView2Environment> g_environment;
std::wstring g_profile;
bool g_trayAdded = false;
HINSTANCE g_instance = nullptr;
HICON g_trayIcon = nullptr;

std::wstring ProfilePath(const wchar_t* profileName) {
    wchar_t localAppData[MAX_PATH]{};
    DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    std::filesystem::path base = size ? localAppData : L".";
    base /= L"WhatsAppWrapper";
    base /= profileName;
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    return base.wstring();
}

void ResizeWebView() {
    if (g_controller && g_hwnd) {
        RECT rect{};
        GetClientRect(g_hwnd, &rect);
        g_controller->put_Bounds(rect);
    }
}

void ShowContextMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_ACCOUNT_1, L"Open Account 1");
    AppendMenuW(menu, MF_STRING, ID_ACCOUNT_2, L"Open Account 2");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_RELOAD, L"Reload");
    AppendMenuW(menu, MF_STRING, ID_ZOOM_IN, L"Zoom in");
    AppendMenuW(menu, MF_STRING, ID_ZOOM_OUT, L"Zoom out");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_EXIT, L"Exit");

    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(g_hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, g_hwnd, nullptr);
    DestroyMenu(menu);
}

void AddTrayIcon() {
    if (g_trayAdded) return;
    g_nid = {};
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAY;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    if (!g_trayIcon) {
        g_trayIcon = static_cast<HICON>(LoadImageW(
            g_instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR));
    }
    g_nid.hIcon = g_trayIcon ? g_trayIcon : LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"WhatsApp Wrapper");
    g_trayAdded = Shell_NotifyIconW(NIM_ADD, &g_nid) == TRUE;
}

void RemoveTrayIcon() {
    if (g_trayAdded) Shell_NotifyIconW(NIM_DELETE, &g_nid);
    g_trayAdded = false;
}

void ShowWindowFromTray() {
    ShowWindow(g_hwnd, SW_SHOW);
    ShowWindow(g_hwnd, SW_RESTORE);
    SetForegroundWindow(g_hwnd);
}

void StartWebView() {
    if (!g_hwnd) return;
    const std::wstring userData = ProfilePath(g_profile.c_str());

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userData.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result) || !environment) {
                    MessageBoxW(g_hwnd,
                        L"Could not initialize WebView2. Install the Microsoft Edge WebView2 Runtime.",
                        L"WhatsApp Wrapper", MB_ICONERROR | MB_OK);
                    return result;
                }
                g_environment = environment;
                return environment->CreateCoreWebView2Controller(
                    g_hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(controllerResult) || !controller) return controllerResult;
                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);

                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(g_webview->get_Settings(&settings)) && settings) {
                                ComPtr<ICoreWebView2Settings2> settings2;

                                if (SUCCEEDED(settings.As(&settings2)) && settings2) {
                                    settings2->put_UserAgent(
                                        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                                        L"AppleWebKit/537.36 (KHTML, like Gecko) "
                                        L"Chrome/155.0.0.0 Safari/537.36 "
                                        L"Edg/155.0.0.0"
                                    );
                                }

                                settings->put_AreDevToolsEnabled(FALSE);
                                settings->put_IsStatusBarEnabled(FALSE);
                            }

                            ResizeWebView();

                            g_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*) -> HRESULT {
                                        return S_OK;
                                    }).Get(), nullptr);

                            g_webview->Navigate(L"https://web.whatsapp.com/");
                            return S_OK;
                        }).Get());
            }).Get());
}

void SwitchAccount(const wchar_t* name) {
    if (!name || !*name) return;

    if (g_profile == name) {
        if (g_webview) {
            g_webview->Reload();
        }
        if (g_hwnd) {
            ShowWindow(g_hwnd, SW_SHOW);
            ShowWindow(g_hwnd, SW_RESTORE);
            SetForegroundWindow(g_hwnd);
        }
        return;
    }

    g_profile = name;
    if (g_controller) {
        g_controller->Close();
        g_controller.Reset();
        g_webview.Reset();
        g_environment.Reset();
    }
    StartWebView();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            ShowWindow(hwnd, SW_HIDE);
            AddTrayIcon();
        } else {
            ResizeWebView();
        }
        return 0;
    case WM_TRAY:
        if (lParam == WM_LBUTTONDBLCLK) ShowWindowFromTray();
        else if (lParam == WM_RBUTTONUP) ShowContextMenu();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_RELOAD:
            if (g_webview) g_webview->Reload();
            return 0;
        case ID_ZOOM_IN:
        case ID_ZOOM_OUT: {
            if (g_controller) {
                double zoom = 1.0;
                g_controller->get_ZoomFactor(&zoom);
                zoom += (LOWORD(wParam) == ID_ZOOM_IN ? 0.1 : -0.1);
                if (zoom < 0.5) zoom = 0.5;
                if (zoom > 2.0) zoom = 2.0;
                g_controller->put_ZoomFactor(zoom);
            }
            return 0;
        }
        case ID_ACCOUNT_1: SwitchAccount(L"account1"); return 0;
        case ID_ACCOUNT_2: SwitchAccount(L"account2"); return 0;
        case ID_EXIT:
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_COPYDATA: {
        auto* cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
        if (cds && cds->dwData == 1 && cds->lpData) {
            const wchar_t* profile = reinterpret_cast<const wchar_t*>(cds->lpData);
            if (profile && *profile) {
                SwitchAccount(profile);
            }
            return TRUE;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    case WM_CLOSE:
        // Closing the window minimizes to the tray; Exit in the tray menu quits.
        ShowWindow(hwnd, SW_HIDE);
        AddTrayIcon();
        return 0;
    case WM_DESTROY:
        RemoveTrayIcon();
        if (g_controller) {
            g_controller->Close();
            g_controller.Reset();
        }
        g_webview.Reset();
        g_environment.Reset();
        if (g_trayIcon) { DestroyIcon(g_trayIcon); g_trayIcon = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
}

int RunApplication(HINSTANCE instance, int showCommand) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring requestedProfile = L"account1";
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--profile") == 0 && i + 1 < argc) {
                requestedProfile = (wcscmp(argv[i + 1], L"account2") == 0) ? L"account2" : L"account1";
                ++i;
            }
        }
        LocalFree(argv);
    }

    HANDLE singleInstance = CreateMutexW(nullptr, TRUE, L"WhatsAppWrapperSingleInstance");
    bool alreadyRunning = (singleInstance != nullptr) && (GetLastError() == ERROR_ALREADY_EXISTS);
    if (alreadyRunning) {
        HWND existing = FindWindowW(kClassName, nullptr);
        if (existing) {
            std::wstring profileCopy = requestedProfile;
            COPYDATASTRUCT cds{};
            cds.dwData = 1;
            cds.cbData = static_cast<DWORD>((profileCopy.size() + 1) * sizeof(wchar_t));
            cds.lpData = profileCopy.data();
            SendMessageW(existing, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        CloseHandle(singleInstance);
        return 0;
    }

    g_instance = instance;
    g_profile = requestedProfile;

    HICON appIcon = static_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    HICON appSmallIcon = static_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = appIcon ? appIcon : LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = appSmallIcon ? appSmallIcon : wc.hIcon;
    wc.lpszClassName = kClassName;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&wc)) return 1;

    g_hwnd = CreateWindowExW(0, kClassName, L"WhatsApp Wrapper",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        nullptr, nullptr, instance, nullptr);
    if (!g_hwnd) return 1;

    AddTrayIcon();
    ShowWindow(g_hwnd, showCommand);
    UpdateWindow(g_hwnd);
    StartWebView();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CloseHandle(singleInstance);
    return static_cast<int>(msg.wParam);
}
