#include "framework.h"
#include "Keyswitch.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_     HINSTANCE hInstance, 
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_     LPTSTR    lpCmdLine, 
                       _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&iccex))
    {
        return EXIT_FAILURE;
    }

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_KEYSWITCH, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (GetKeyboardLayoutList(0, NULL) < 2)
    {
        TCHAR szMessage[MAX_LOADSTRING];
        LoadString(hInst, IDS_LESS_THAN_2, szMessage, MAX_LOADSTRING);
        MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    if (!InitInstance(hInstance, nCmdShow))
    {
        return EXIT_FAILURE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYSWITCH));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
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

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, CW_USEDEFAULT, 
       WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInst, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);

            HWND hEditBox = CreateWindow(WC_EDIT, NULL,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 200, 24, hWnd, (HMENU)IDC_EDITBOX, hInst, NULL);
            SendMessage(hEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);

            HWND hCurKlCbox = CreateWindow(WC_COMBOBOX, NULL,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | CBS_DROPDOWNLIST,
                0, 0, 200, 24, hWnd, (HMENU)IDC_CUR_KL_COMBO, hInst, NULL);
            SendMessage(hCurKlCbox, WM_SETFONT, (WPARAM)hFont, TRUE);

            HWND hAltKlCbox = CreateWindow(WC_COMBOBOX, NULL,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | CBS_DROPDOWNLIST,
                0, 0, 200, 24, hWnd, (HMENU)IDC_ALT_KL_COMBO, hInst, NULL);
            SendMessage(hAltKlCbox, WM_SETFONT, (WPARAM)hFont, TRUE);

            int nhkl = GetKeyboardLayoutList(0, NULL);
            LPHKL ahkl = HeapAlloc(GetProcessHeap(),
                HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, nhkl * sizeof(HKL));
            GetKeyboardLayoutList(nhkl, ahkl);
            TCHAR szhkl[LOCALE_NAME_MAX_LENGTH];
            for (int i = 0; i < nhkl; i++)
            {
                ZeroMemory(szhkl, LOCALE_NAME_MAX_LENGTH * sizeof(TCHAR));
                GetLocaleInfo(MAKELCID(LOWORD(ahkl[i]), SORT_DEFAULT), 
                    LOCALE_SENGLANGUAGE, szhkl, LOCALE_NAME_MAX_LENGTH);
                SendMessage(hCurKlCbox, CB_ADDSTRING, 0, (LPARAM)szhkl);
                SendMessage(hCurKlCbox, CB_SETITEMDATA, i, (LPARAM)ahkl[i]);
                SendMessage(hAltKlCbox, CB_ADDSTRING, 0, (LPARAM)szhkl);
                SendMessage(hAltKlCbox, CB_SETITEMDATA, i, (LPARAM)ahkl[i]);
            }
            SendMessage(hCurKlCbox, CB_SETCURSEL, 0, 0);
            SendMessage(hAltKlCbox, CB_SETCURSEL, 1, 0);
            HeapFree(GetProcessHeap(), 0, ahkl);

            TCHAR szButton[MAX_LOADSTRING];
            LoadString(hInst, IDS_SWITCH_BUTTON, szButton, MAX_LOADSTRING);
            HWND hSwitchBtn = CreateWindow(WC_BUTTON, szButton,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                0, 0, 200, 24, hWnd, (HMENU)IDC_SWITCH_BUTTON, hInst, NULL);
            SendMessage(hSwitchBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        break;
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
            lpmmi->ptMinTrackSize.x = WINDOW_WIDTH;
            lpmmi->ptMaxTrackSize.x = WINDOW_WIDTH;
            lpmmi->ptMinTrackSize.y = WINDOW_HEIGHT;
            lpmmi->ptMaxTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }
        break;
    case WM_SIZE:
        {
            HWND hEditBox = GetDlgItem(hWnd, IDC_EDITBOX);
            HWND hCurKlCbox = GetDlgItem(hWnd, IDC_CUR_KL_COMBO);
            HWND hAltKlCbox = GetDlgItem(hWnd, IDC_ALT_KL_COMBO);
            HWND hSwitchBtn = GetDlgItem(hWnd, IDC_SWITCH_BUTTON);

            int nWndWidth = LOWORD(lParam);
            int nWndHeight = HIWORD(lParam);

            int nCtrlPadding = 7;
            int nCtrlHeight = 24;
            int nCtrlGap = nCtrlPadding;

            int nEditX = nCtrlPadding;
            int nEditY = nCtrlPadding;
            int nEditWidth = nWndWidth - 2 * nCtrlPadding;

            int nComboY = nEditY + nCtrlHeight + nCtrlPadding;
            int nComboWidth = (nEditWidth - nCtrlGap) / 2;
            int nComboRem = (nEditWidth - nCtrlGap) % 2;

            int nButtonY = nComboY + nCtrlHeight + nCtrlPadding;

            MoveWindow(hEditBox, nEditX, nEditY,
                nEditWidth, nCtrlHeight, TRUE);

            MoveWindow(hCurKlCbox, nEditX, nComboY,
                nComboWidth, nCtrlHeight, TRUE);

            MoveWindow(hAltKlCbox, nEditX + nComboWidth + nCtrlGap, nComboY,
                nComboWidth + nComboRem, nCtrlHeight, TRUE);

            MoveWindow(hSwitchBtn, nEditX, nButtonY,
                nEditWidth, nCtrlHeight, TRUE);
        }
        break;
    case WM_COMMAND:
        {
            WORD wId = LOWORD(wParam);
            WORD wCmd = HIWORD(wParam);
            HWND hCtrl = (HWND)lParam;

            HWND hCurKlCbox = GetDlgItem(hWnd, IDC_CUR_KL_COMBO);
            HWND hAltKlCbox = GetDlgItem(hWnd, IDC_ALT_KL_COMBO);

            if (wCmd == 0 && hCtrl == NULL)
            {
                switch (wId)
                {
                case ID_HELP_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case ID_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            else if (wCmd == CBN_SELCHANGE)
            {
                HWND hOtherCbox = NULL;
                if (hCtrl == hCurKlCbox)
                {
                    hOtherCbox = hAltKlCbox;
                }
                else if (hCtrl == hAltKlCbox)
                {
                    hOtherCbox = hCurKlCbox;
                }
                else 
                {
                    return 0;
                }

                int iThisCbox = (int)SendMessage(hCtrl, CB_GETCURSEL, 0, 0);
                int iOtherCbox = (int)SendMessage(hOtherCbox, CB_GETCURSEL, 0, 0);
                int nOtherCbox = (int)SendMessage(hOtherCbox, CB_GETCOUNT, 0, 0);

                if (iThisCbox == CB_ERR || iOtherCbox == CB_ERR || nOtherCbox <= 0)
                {
                    return 0;
                }

                if (iThisCbox == iOtherCbox)
                {
                    int i = (iOtherCbox + 1) % nOtherCbox;
                    SendMessage(hOtherCbox, CB_SETCURSEL, (WPARAM)i, 0);
                }
            }
            else if (wCmd == BN_CLICKED)
            {
                int iCurKlCbox = (int)SendMessage(hCurKlCbox, CB_GETCURSEL, 0, 0);
                int iAltKlCbox = (int)SendMessage(hAltKlCbox, CB_GETCURSEL, 0, 0);

                HKL hklCur = (HKL)SendMessage(hCurKlCbox, CB_GETITEMDATA, iCurKlCbox, 0);
                HKL hklAlt = (HKL)SendMessage(hAltKlCbox, CB_GETITEMDATA, iAltKlCbox, 0);

                HWND hEditBox = GetDlgItem(hWnd, IDC_EDITBOX);
                int nEditBox = GetWindowTextLength(hEditBox);
                if (nEditBox > 0)
                {
                    LPTSTR szEditBox = HeapAlloc(GetProcessHeap(),
                        HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 
                        (nEditBox + 1) * sizeof(TCHAR));
                    if (szEditBox == NULL)
                    {
                        return 0;
                    }
                    GetWindowText(hEditBox, szEditBox, nEditBox + 1);

                    BYTE mods, keyState[256];
                    SHORT vkCode;
                    for (int i = 0; i < nEditBox; i++)
                    {
                        vkCode = VkKeyScanEx(szEditBox[i], hklCur);
                        BOOL isKeyState = GetKeyboardState(keyState);
                        if (vkCode == -1 || !isKeyState)
                        {
                            continue;
                        }
                        /* To detect capital letter. */
                        mods = (BYTE)((vkCode >> 8) & 0xFF);
                        keyState[0x10] = (BYTE)((mods & 1) != 0 ? 0x80 : 0);
#ifdef UNICODE
                        ToUnicodeEx(vkCode, 0, keyState, &szEditBox[i], 1, 0, hklAlt);
#else
                        ToAsciiEx(vkCode, 0, keyState, &szEditBox[i], 0, hklAlt);
#endif
                    }
                    SetWindowText(hEditBox, szEditBox);
                    SendMessage(hCurKlCbox, CB_SETCURSEL, (WPARAM)iAltKlCbox, 0);
                    SendMessage(hAltKlCbox, CB_SETCURSEL, (WPARAM)iCurKlCbox, 0);
                    HeapFree(GetProcessHeap(), 0, szEditBox);
                }
            }
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR) TRUE;
        }
        break;
    }
    return (INT_PTR) FALSE;
}
