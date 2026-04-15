#include <WinSDKVer.h>
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ctype.h>
#include <tchar.h>
#include <strsafe.h>

#include "Keyswitch.h"

#define MAX_LOADSTRING 100

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

BOOL KS_WritePrivateProfileInt(LPCTSTR lpszSection, LPCTSTR lpszKey, UINT uValue, LPCTSTR lpszPath)
{
    TCHAR szBuffer[8];
    ZeroMemory(szBuffer, sizeof(szBuffer));
    StringCchPrintf(szBuffer, _countof(szBuffer), _T("%u"), uValue);
    return WritePrivateProfileString(lpszSection, lpszKey, szBuffer, lpszPath);
}

void GetConfigPath(LPTSTR lpszBuffer)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szConfigName[MAX_LOADSTRING];
    GetModuleFileName(NULL, szPath, _countof(szPath));
    LoadString(GetModuleHandle(NULL), IDS_CONFIG_PATH, szConfigName, _countof(szConfigName));
    PathRemoveFileSpec(szPath);
    PathCombine(lpszBuffer, szPath, szConfigName);
}

void LoadSettings(KS_USERDATA* lpUserData)
{
    TCHAR szPath[MAX_PATH];
    GetConfigPath(szPath);

    lpUserData->isToggle = (BOOL)!!GetPrivateProfileInt(szTitle, _T("ToggleLayouts"), 0, szPath);
    lpUserData->uEscAction = GetPrivateProfileInt(szTitle, _T("EscapeAction"), 0, szPath);
    lpUserData->uLastCurKl = GetPrivateProfileInt(szTitle, _T("CurrentLayout"), 0, szPath);
    lpUserData->uLastAltKl = GetPrivateProfileInt(szTitle, _T("AlternateLayout"), 1, szPath);
}

void SaveSettings(const KS_USERDATA userData)
{
    TCHAR szPath[MAX_PATH];
    GetConfigPath(szPath);

    KS_WritePrivateProfileInt(szTitle, _T("ToggleLayouts"), (UINT)(userData.isToggle), szPath);
    KS_WritePrivateProfileInt(szTitle, _T("EscapeAction"), userData.uEscAction, szPath);
    KS_WritePrivateProfileInt(szTitle, _T("CurrentLayout"), userData.uLastCurKl, szPath);
    KS_WritePrivateProfileInt(szTitle, _T("AlternateLayout"), userData.uLastAltKl, szPath);
}

LPTSTR SwitchLayouts(LPTSTR lpszDst, HKL hklDst, LPCTSTR lpszSrc, HKL hklSrc)
{
    if (lpszDst == NULL || lpszSrc == NULL)
    {
        return NULL;
    }

    BYTE keyState[256];
    SHORT vkCode;
    for (int i = 0; lpszSrc[i] != '\0'; i++)
    {
        vkCode = VkKeyScanEx(lpszSrc[i], hklSrc);
        BOOL isKeyState = GetKeyboardState(keyState);
        if (vkCode == -1 || !isKeyState || _istcntrl(lpszSrc[i]))
        {
            lpszDst[i] = lpszSrc[i];
        }
        else
        {
            keyState[VK_SHIFT] = (BYTE)((HIBYTE(vkCode) & 1) != 0 ? 0x80 : 0);
#ifdef UNICODE
            ToUnicodeEx(vkCode, 0, keyState, &lpszDst[i], 1, 0, hklDst);
#else
            ToAsciiEx(vkCode, 0, keyState, &lpszDst[i], 0, hklDst);
#endif
        }
    }
    return lpszDst;
}

INT_PTR CALLBACK AboutBox(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
        lpmmi->ptMinTrackSize.x = WINDOW_WIDTH;
        lpmmi->ptMinTrackSize.y = WINDOW_HEIGHT;
    }
    break;

    case WM_CREATE:
    {
        KS_USERDATA* lpUserData = HeapAlloc(GetProcessHeap(),
            HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(KS_USERDATA));
        if (lpUserData == NULL)
        {
            return -1;
        }
        LoadSettings(lpUserData);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpUserData);

        HMENU hMenu = GetMenu(hWnd);
        if (lpUserData->isToggle)
        {
            CheckMenuItem(hMenu, ID_VIEW_TOGGLE, MF_BYCOMMAND | MF_CHECKED);
        }

        UINT uEscCheck;
        switch (lpUserData->uEscAction)
        {
        case ESC_EXIT:
            uEscCheck = ID_VIEW_ESC_EXIT;
            break;

        case ESC_MINIMIZE:
            uEscCheck = ID_VIEW_ESC_MIN;
            break;

        default:
            uEscCheck = ID_VIEW_ESC_NOACT;
            break;
        }
        CheckMenuRadioItem(GetMenu(hWnd),
            ID_VIEW_ESC_NOACT, ID_VIEW_ESC_MIN,
            uEscCheck, MF_BYCOMMAND);

        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);

        HWND hEditBox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL,
            0, 0, WINDOW_WIDTH, CTRL_HEIGHT,
            hWnd, (HMENU)IDC_EDITBOX, GetModuleHandle(NULL), NULL);
        SendMessage(hEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hCurKlCbox = CreateWindow(WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            0, 0, WINDOW_WIDTH, CTRL_HEIGHT,
            hWnd, (HMENU)IDC_CUR_KL_COMBO, GetModuleHandle(NULL), NULL);
        SendMessage(hCurKlCbox, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hAltKlCbox = CreateWindow(WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            0, 0, WINDOW_WIDTH, CTRL_HEIGHT,
            hWnd, (HMENU)IDC_ALT_KL_COMBO, GetModuleHandle(NULL), NULL);
        SendMessage(hAltKlCbox, WM_SETFONT, (WPARAM)hFont, TRUE);

        int nhkl = GetKeyboardLayoutList(0, NULL);
        LPHKL ahkl = HeapAlloc(GetProcessHeap(),
            HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nhkl * sizeof(HKL));
        if (ahkl == NULL)
        {
            HeapFree(GetProcessHeap(), 0, lpUserData);
            return -1;
        }
        GetKeyboardLayoutList(nhkl, ahkl);
        TCHAR szLanguage[LOCALE_NAME_MAX_LENGTH];
        TCHAR szCountry[LOCALE_NAME_MAX_LENGTH];
        TCHAR szhkl[LOCALE_NAME_MAX_LENGTH];
        for (int i = 0; i < nhkl; i++)
        {
            ZeroMemory(szLanguage, sizeof(szLanguage));
            ZeroMemory(szCountry, sizeof(szCountry));
            ZeroMemory(szhkl, sizeof(szhkl));
            GetLocaleInfo(MAKELCID(LOWORD(ahkl[i]), SORT_DEFAULT),
                LOCALE_SENGLANGUAGE, szLanguage, _countof(szLanguage));
            GetLocaleInfo(MAKELCID(LOWORD(ahkl[i]), SORT_DEFAULT),
                LOCALE_SENGCOUNTRY, szCountry, _countof(szCountry));
            StringCchPrintf(szhkl, _countof(szhkl), _T("%s (%s)"), szLanguage, szCountry);
            SendMessage(hCurKlCbox, CB_ADDSTRING, 0, (LPARAM)szhkl);
            SendMessage(hCurKlCbox, CB_SETITEMDATA, i, (LPARAM)ahkl[i]);
            SendMessage(hAltKlCbox, CB_ADDSTRING, 0, (LPARAM)szhkl);
            SendMessage(hAltKlCbox, CB_SETITEMDATA, i, (LPARAM)ahkl[i]);
        }
        SendMessage(hCurKlCbox, CB_SETCURSEL, (WPARAM)(lpUserData->uLastCurKl % nhkl), 0);
        SendMessage(hAltKlCbox, CB_SETCURSEL, (WPARAM)(lpUserData->uLastAltKl % nhkl), 0);
        HeapFree(GetProcessHeap(), 0, ahkl);

        TCHAR szButton[MAX_LOADSTRING];
        LoadString(GetModuleHandle(NULL), IDS_SWITCH_BUTTON, szButton, _countof(szButton));
        HWND hSwitchBtn = CreateWindow(WC_BUTTON, szButton,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0, 0, WINDOW_WIDTH, CTRL_HEIGHT,
            hWnd, (HMENU)IDC_SWITCH_BUTTON, GetModuleHandle(NULL), NULL);
        SendMessage(hSwitchBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    break;

    case WM_SIZE:
    {
        HWND hEditBox = GetDlgItem(hWnd, IDC_EDITBOX);
        HWND hCurKlCbox = GetDlgItem(hWnd, IDC_CUR_KL_COMBO);
        HWND hAltKlCbox = GetDlgItem(hWnd, IDC_ALT_KL_COMBO);
        HWND hSwitchBtn = GetDlgItem(hWnd, IDC_SWITCH_BUTTON);

        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);

        int xEditBox = CTRL_PADDING;
        int yEditBox = CTRL_PADDING;
        int cxEditBox = cx - 2 * CTRL_PADDING;
        int cyEditBox = cy - 2 * CTRL_HEIGHT - 4 * CTRL_PADDING - 1;

        int ySwitchBtn = cy - CTRL_PADDING - CTRL_HEIGHT;

        int yCbox = ySwitchBtn - CTRL_PADDING - CTRL_HEIGHT;
        int cxCbox = (cxEditBox - CTRL_GAP) / 2 + (cxEditBox - CTRL_GAP) % 2;

        MoveWindow(hEditBox, xEditBox, yEditBox,
            cxEditBox, cyEditBox, TRUE);

        MoveWindow(hCurKlCbox, xEditBox, yCbox,
            cxCbox, CTRL_HEIGHT, TRUE);

        MoveWindow(hAltKlCbox, xEditBox + cxCbox + CTRL_GAP, yCbox,
            cxCbox, CTRL_HEIGHT, TRUE);

        MoveWindow(hSwitchBtn, xEditBox, ySwitchBtn,
            cxEditBox, CTRL_HEIGHT, TRUE);
    }
    break;

    case WM_COMMAND:
    {
        KS_USERDATA* lpUserData = (KS_USERDATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        switch (LOWORD(wParam))
        {
        case IDC_CUR_KL_COMBO:
        case IDC_ALT_KL_COMBO:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hCbox1 = (HWND)lParam;
                HWND hCbox2 = NULL;
                if (LOWORD(wParam) == IDC_CUR_KL_COMBO)
                {
                    hCbox2 = GetDlgItem(hWnd, IDC_ALT_KL_COMBO);
                }
                else if (LOWORD(wParam) == IDC_ALT_KL_COMBO)
                {
                    hCbox2 = GetDlgItem(hWnd, IDC_CUR_KL_COMBO);
                }
                else
                {
                    return 0;
                }

                int iCbox1 = (int)SendMessage(hCbox1, CB_GETCURSEL, 0, 0);
                int iCbox2 = (int)SendMessage(hCbox2, CB_GETCURSEL, 0, 0);
                int nCbox2 = (int)SendMessage(hCbox1, CB_GETCOUNT, 0, 0);

                if (iCbox1 == CB_ERR || iCbox2 == CB_ERR || nCbox2 <= 0)
                {
                    return 0;
                }

                if (iCbox1 == iCbox2)
                {
                    int i = (iCbox2 + 1) % nCbox2;
                    SendMessage(hCbox2, CB_SETCURSEL, (WPARAM)i, 0);
                }
            }
        }
        break;

        case IDC_SWITCH_BUTTON:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                HWND hCurKlCbox = GetDlgItem(hWnd, IDC_CUR_KL_COMBO);
                HWND hAltKlCbox = GetDlgItem(hWnd, IDC_ALT_KL_COMBO);
                int iCurKlCbox = (int)SendMessage(hCurKlCbox, CB_GETCURSEL, 0, 0);
                int iAltKlCbox = (int)SendMessage(hAltKlCbox, CB_GETCURSEL, 0, 0);
                HKL hklCur = (HKL)SendMessage(hCurKlCbox, CB_GETITEMDATA, iCurKlCbox, 0);
                HKL hklAlt = (HKL)SendMessage(hAltKlCbox, CB_GETITEMDATA, iAltKlCbox, 0);

                HWND hEditBox = GetDlgItem(hWnd, IDC_EDITBOX);
                int nEditBox = GetWindowTextLength(hEditBox);
                if (nEditBox > 0)
                {
                    LPTSTR szEditBox = HeapAlloc(GetProcessHeap(),
                        HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, (nEditBox + 1) * sizeof(TCHAR));
                    if (szEditBox == NULL)
                    {
                        return 0;
                    }
                    GetWindowText(hEditBox, szEditBox, nEditBox + 1);
                    SwitchLayouts(szEditBox, hklAlt, szEditBox, hklCur);
                    SetWindowText(hEditBox, szEditBox);
                    HeapFree(GetProcessHeap(), 0, szEditBox);

                    if (lpUserData->isToggle)
                    {
                        SendMessage(hCurKlCbox, CB_SETCURSEL, (WPARAM)iAltKlCbox, 0);
                        SendMessage(hAltKlCbox, CB_SETCURSEL, (WPARAM)iCurKlCbox, 0);
                    }
                    lpUserData->uLastCurKl = (UINT)iCurKlCbox;
                    lpUserData->uLastAltKl = (UINT)iAltKlCbox;
                }
            }
        }
        break;

        case ID_ESCAPE:
        {
            switch (lpUserData->uEscAction)
            {
            case ESC_EXIT:
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                break;

            case ESC_MINIMIZE:
                ShowWindow(hWnd, SW_MINIMIZE);
                break;

            default:
                break;
            }
        }
        break;

        case ID_FILE_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case ID_HELP_ABOUT:
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutBox);
            break;

        case ID_VIEW_TOGGLE:
        {
            lpUserData->isToggle = !(lpUserData->isToggle);
            UINT uCheck = (lpUserData->isToggle) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(GetMenu(hWnd), ID_VIEW_TOGGLE, uCheck | MF_BYCOMMAND);
        }
        break;

        case ID_VIEW_ESC_NOACT:
        case ID_VIEW_ESC_EXIT:
        case ID_VIEW_ESC_MIN:
        {
            CheckMenuRadioItem(GetMenu(hWnd),
                ID_VIEW_ESC_NOACT, ID_VIEW_ESC_MIN,
                LOWORD(wParam), MF_BYCOMMAND);

            switch (LOWORD(wParam))
            {
            case ID_VIEW_ESC_NOACT:
                lpUserData->uEscAction = ESC_NO_ACT;
                break;

            case ID_VIEW_ESC_EXIT:
                lpUserData->uEscAction = ESC_EXIT;
                break;

            case ID_VIEW_ESC_MIN:
                lpUserData->uEscAction = ESC_MINIMIZE;
                break;

            default:
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    break;

    case WM_CLOSE:
    {
        KS_USERDATA* lpUserData = (KS_USERDATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        SaveSettings(*lpUserData);
        HeapFree(GetProcessHeap(), 0, lpUserData);
        DestroyWindow(hWnd);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&iccex))
    {
        return 1;
    }

    LoadString(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
    LoadString(hInstance, IDC_KEYSWITCH, szWindowClass, _countof(szWindowClass));

    if (GetKeyboardLayoutList(0, NULL) < 2)
    {
        TCHAR szMessage[MAX_LOADSTRING];
        LoadString(hInstance, IDS_LESS_THAN_2, szMessage, _countof(szMessage));
        MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONHAND);
        return 1;
    }

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYSWITCH));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_KEYSWITCH);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_KEYSWITCH), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassEx(&wcex))
    {
        return 1;
    }

    int cxScreen = GetSystemMetrics(SM_CXSCREEN);
    int cyScreen = GetSystemMetrics(SM_CYSCREEN);
    int x = (cxScreen - WINDOW_WIDTH) / 2;
    int y = (cyScreen - WINDOW_HEIGHT) / 2;

    HWND hWnd = CreateWindow(szWindowClass, szTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL)
    {
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYSWITCH));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hWnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}