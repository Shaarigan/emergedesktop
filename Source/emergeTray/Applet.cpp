// vim:tags+=../emergeLib/tags
//----  --------------------------------------------------------------------------------------------------------
//
//  This file is part of Emerge Desktop.
//  Copyright (C) 2004-2013  The Emerge Desktop Development Team
//
//  Emerge Desktop is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  Emerge Desktop is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//----  --------------------------------------------------------------------------------------------------------
//
// Many thanx go to Jaykul (of GeoShell fame) and Message (of LiteStep fame)
// for helping wrap my head around Windows deals with the systray.
//
//----  --------------------------------------------------------------------------------------------------------

#include "Applet.h"

bool InjectExplorerTrayHook(HWND messageHandler);

/**< Tray related class names */
WCHAR szTrayName[ ] = TEXT("Shell_TrayWnd");
WCHAR szNotifyName[ ] = TEXT("TrayNotifyWnd");
WCHAR szReBarName[ ] = TEXT("ReBarWindow32");
WCHAR szClockName[ ] = TEXT("TrayClockWClass");
WCHAR szTaskSwName[ ] = TEXT("MSTaskSwWClass");

WCHAR edTrayName[ ] = TEXT("ed_Shell_TrayWnd");

WCHAR myName[] = TEXT("emergeTray");

HMODULE hookDLLaddr;
DWORD taskBarThreadId, notifyWndThreadId, trayWndThreadId, clockWndThreadId, rebarWndThreadId, taskWndThreadId;
HHOOK taskBarCallWndRetHook, notifyWndCallWndRetHook, trayWndCallWndRetHook, clockWndCallWndRetHook, rebarWndCallWndRetHook, taskWndCallWndRetHook;


LRESULT CALLBACK Applet::TrayProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  COPYDATASTRUCT* cpData;
  CREATESTRUCT* cs;
  WINDOWPOS* wndPos;
  static Applet* pApplet = NULL;

  if (message == WM_CREATE)
  {
    cs = (CREATESTRUCT*)lParam;
    pApplet = reinterpret_cast<Applet*>(cs->lpCreateParams);
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  if (pApplet == NULL)
  {
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  switch (message)
  {
    // Handle the icon messages
  case WM_COPYDATA:
    cpData = (COPYDATASTRUCT*)lParam;

    switch (cpData->dwData)
    {
    case TRAY_MESSAGE:
      return pApplet->TrayIconEvent(cpData);
    case APPBAR_MESSAGE:
      return pApplet->AppBarEvent(cpData);
    case ICONIDENTIFIER_MESSAGE:
      return pApplet->IconIdentifierEvent(cpData);
    }
    break;

    // IrfanView forces the tray window to be visible when it returns from
    // fullscreen mode.  As such, capture the WM_WINDOWPOSCHANGING message...
  case WM_WINDOWPOSCHANGING:
    wndPos = (WINDOWPOS*)lParam;
    // ... make sure the window is at the bottom of the z-order...
    wndPos->hwndInsertAfter = HWND_BOTTOM;
    // ... and make sure it stays hidden
    wndPos->flags &= ~SWP_SHOWWINDOW;
    wndPos->flags |= SWP_HIDEWINDOW;
    break;

    // If not handled just forward the message on
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  return 0;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	WindowProcedure
// Required:	HWND hwnd - window handle that message was sent to
// 		UINT message - action to handle
// 		WPARAM wParam - dependant on message
// 		LPARAM lParam - dependant on message
// Returns:	LRESULT
// Purpose:	Handles messages sent from DispatchMessage
//----  --------------------------------------------------------------------------------------------------------
LRESULT CALLBACK Applet::WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  CREATESTRUCT* cs;
  static Applet* pApplet = NULL;

  if (message == WM_CREATE)
  {
    // Register to recieve the specified Emerge Desktop messages
    PostMessage(ELGetCoreWindow(), EMERGE_REGISTER, (WPARAM)hwnd, (LPARAM)EMERGE_CORE);

    cs = (CREATESTRUCT*)lParam;
    pApplet = reinterpret_cast<Applet*>(cs->lpCreateParams);
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  if (pApplet == NULL)
  {
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  switch (message)
  {
  case WM_COPYDATA:
    return pApplet->DoCopyData((COPYDATASTRUCT*)lParam);

    // Needed to handle changing the system colors.  It forces
    // a repaint of the window as well as the frame.
  case WM_SYSCOLORCHANGE:
    return pApplet->DoSysColorChange();

    // Allow for window dragging via Ctrl - Left - Click dragging
  case WM_NCLBUTTONDOWN:
    pApplet->DoNCLButtonDown();
    return DefWindowProc(hwnd, message, wParam, lParam);

    // Display the mainMenu via Ctrl - Right - Click
  case WM_NCRBUTTONUP:
    return pApplet->DoNCRButtonUp();

    // Forward the appropriate clicks to the icons
  case WM_LBUTTONUP:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  case WM_MBUTTONDBLCLK:
  case WM_MOUSEMOVE:
    return pApplet->TrayMouseEvent(message, lParam);

    // Reset the cursor back to the standard arrow after dragging
  case WM_NCLBUTTONUP:
    pApplet->DoNCLButtonUp();
    return DefWindowProc(hwnd, message, wParam, lParam);

  case WM_SETCURSOR:
    pApplet->DoSetCursor();
    return DefWindowProc(hwnd, message, wParam, lParam);

    // Handles the resizing of the window
  case WM_NCHITTEST:
    return pApplet->DoHitTest(lParam);

    // Repaint the icons as the window size is changing
  case WM_WINDOWPOSCHANGING:
    return pApplet->DoWindowPosChanging((WINDOWPOS*)lParam);

  case WM_ENTERSIZEMOVE:
    return pApplet->DoEnterSizeMove(hwnd);

  case WM_EXITSIZEMOVE:
    return pApplet->DoExitSizeMove(hwnd);

  case WM_SIZING:
    return pApplet->DoSizing(hwnd, (UINT)wParam, (LPRECT)lParam);

  case WM_MOVING:
    return pApplet->DoMoving(hwnd, (LPRECT)lParam);

    // Write the X and Y settings to the registry
  case WM_MOVE:
    return pApplet->MyMove();

    // Write the width height to the registry if the size changed
  case WM_SIZE:
    return pApplet->MySize(lParam);

  case WM_NOTIFY:
    return pApplet->DoNotify(hwnd, lParam);

  case WM_SYSCOMMAND:
    return pApplet->DoSysCommand(hwnd, message, wParam, lParam);

  case WM_TIMER:
    return pApplet->DoTimer((UINT)wParam);

  case WM_DISPLAYCHANGE:
    return pApplet->DoDisplayChange(hwnd);

  case WM_DESTROY:
  case WM_NCDESTROY:
    // Unregister the specified Emerge Desktop messages
    PostMessage(ELGetCoreWindow(), EMERGE_UNREGISTER, (WPARAM)hwnd, (LPARAM)EMERGE_CORE);

    PostQuitMessage(0);
    break;

    // If not handled just forward the message on
  default:
    return pApplet->DoDefault(hwnd, message, wParam, lParam);
  }

  return 0;
}

Applet::Applet(HINSTANCE hInstance)
  : BaseApplet(hInstance, myName, true, false)
{
  mainInst = hInstance;

  baseClassRegistered = false;
  trayClassRegistered = false;
  notifyClassRegistered = false;
  movesizeinprogress = false;

  autoHideLeft = false;
  autoHideRight = false;
  autoHideBottom = false;
  autoHideTop = false;

  activeIcon = NULL;
  trayWnd = NULL;
  oldTipIcon = NULL;

  InitializeCriticalSection(&trayVectorCS);
}

bool InjectExplorerTrayHook(HWND messageHandler)
{
  hookDLLaddr = ELLoadEmergeLibrary(TEXT("emergeTrayExplorerHook.dll"));
  if (!hookDLLaddr)
  {
    return false;
  }

  HOOKPROC CallWndRetProcAddr = (HOOKPROC)GetProcAddress(hookDLLaddr, "CallWndRetProc");
  if (!CallWndRetProcAddr)
  {
    FreeLibrary(hookDLLaddr);
    hookDLLaddr = NULL;
    return false;
  }

  HWND taskBarhWnd = FindWindow(szTrayName, NULL);

  if (taskBarhWnd)
  {
    taskBarThreadId = GetWindowThreadProcessId(taskBarhWnd, NULL);

    HWND notifyhWnd = FindWindowEx(taskBarhWnd, NULL, szNotifyName, NULL);
    if (notifyhWnd)
    {
      notifyWndThreadId = GetWindowThreadProcessId(notifyhWnd, NULL);

      HWND sysPagerhWnd = FindWindowEx(notifyhWnd, NULL, TEXT("SysPager"), NULL);
      if (sysPagerhWnd)
      {
        HWND trayhWnd = FindWindowEx(sysPagerhWnd, NULL, TEXT("ToolbarWindow32"), NULL);
        if (trayhWnd)
        {
          trayWndThreadId = GetWindowThreadProcessId(trayhWnd, NULL);
        }
      }

      HWND clockhWnd = FindWindowEx(notifyhWnd, NULL, szClockName, NULL);
      if (clockhWnd)
      {
        clockWndThreadId = GetWindowThreadProcessId(clockhWnd, NULL);
      }
    }

    HWND rebarhWnd = FindWindowEx(taskBarhWnd, NULL, szReBarName, NULL);
    if (rebarhWnd)
    {
      rebarWndThreadId = GetWindowThreadProcessId(rebarhWnd, NULL);

      HWND taskhWnd = FindWindowEx(rebarhWnd, NULL, szTaskSwName, NULL);
      if (taskhWnd)
      {
        taskWndThreadId = GetWindowThreadProcessId(taskhWnd, NULL);
      }
    }
  }

  if (taskBarThreadId)
  {
    taskBarCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, taskBarThreadId);
  }

  if (notifyWndThreadId)
  {
    notifyWndCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, notifyWndThreadId);
  }

  if (trayWndThreadId)
  {
    trayWndCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, trayWndThreadId);
  }

  if (clockWndThreadId)
  {
    clockWndCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, clockWndThreadId);
  }

  if (rebarWndThreadId)
  {
    rebarWndCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, rebarWndThreadId);
  }

  if (taskWndThreadId)
  {
    taskWndCallWndRetHook = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProcAddr, hookDLLaddr, taskWndThreadId);
  }

  SendMessage(taskBarhWnd, TRAYHOOK_MSGPROC_ATTACH, (WPARAM)NULL, (LPARAM)messageHandler);

  return true;
}

bool removeExplorerTrayHook()
{
  if (!hookDLLaddr)
  {
    return false;
  }

  if (taskBarCallWndRetHook)
  {
    UnhookWindowsHookEx(taskBarCallWndRetHook);
  }

  if (notifyWndCallWndRetHook)
  {
    UnhookWindowsHookEx(notifyWndCallWndRetHook);
  }

  if (trayWndCallWndRetHook)
  {
    UnhookWindowsHookEx(trayWndCallWndRetHook);
  }

  if (clockWndCallWndRetHook)
  {
    UnhookWindowsHookEx(clockWndCallWndRetHook);
  }

  if (rebarWndCallWndRetHook)
  {
    UnhookWindowsHookEx(rebarWndCallWndRetHook);
  }

  if (taskWndCallWndRetHook)
  {
    UnhookWindowsHookEx(taskWndCallWndRetHook);
  }

  FreeLibrary(hookDLLaddr);

  return true;
}

bool Applet::AcquireExplorerTrayIconList()
{
  HWND taskBarhWnd = FindWindow(szTrayName, NULL);
  if (!taskBarhWnd)
  {
    return false;
  }

  HWND notifyhWnd = FindWindowEx(taskBarhWnd, NULL, szNotifyName, NULL);
  if (!notifyhWnd)
  {
    return false;
  }

  HWND sysPagerhWnd = FindWindowEx(notifyhWnd, NULL, TEXT("SysPager"), NULL);
  if (!sysPagerhWnd)
  {
    return false;
  }

  HWND trayhWnd = FindWindowEx(sysPagerhWnd, NULL, TEXT("ToolbarWindow32"), NULL);
  if (!trayhWnd)
  {
    return false;
  }

  DWORD processId;
  DWORD threadId = GetWindowThreadProcessId(taskBarhWnd, &processId);
  if ((!threadId) || (!processId))
  {
    return false;
  }

  int trayButtonCount = SendMessage(trayhWnd, TB_BUTTONCOUNT, 0, 0);
  if (trayButtonCount == 0)
  {
    return false;
  }

  HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, processId);
  if (!hProcess)
  {
    return false;
  }

  TRAYICONDATA trayIconData;
  TBBUTTONINFO trayIconTBButtonInfo;
  void* tbButtonInfoRemote;
  void* sTipRemote;
  bool hidden;
  bool shared;
  WCHAR sTipText[1024];
  UINT uFlags = 0;
  int counter;

  for (counter = 0; counter < trayButtonCount; counter++)
  {
    tbButtonInfoRemote = VirtualAllocEx(hProcess, NULL, sizeof(TBBUTTONINFO), MEM_COMMIT, PAGE_READWRITE);
    if (!tbButtonInfoRemote)
    {
      break;
    }

    sTipRemote = VirtualAllocEx(hProcess, NULL, (sizeof(WCHAR) * (wcslen(sTipText) + 1)), MEM_COMMIT, PAGE_READWRITE);
    if (!sTipRemote)
    {
      break;
    }

    trayIconTBButtonInfo.cbSize = sizeof(TBBUTTONINFO);
    trayIconTBButtonInfo.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_LPARAM | TBIF_COMMAND;
    if (!WriteProcessMemory(hProcess, tbButtonInfoRemote, &trayIconTBButtonInfo, sizeof(TBBUTTONINFO), NULL))
    {
      break;
    }

    SendMessage(trayhWnd, TB_GETBUTTONINFO, counter, (LPARAM)tbButtonInfoRemote);

    if (!ReadProcessMemory(hProcess, tbButtonInfoRemote, (LPVOID)&trayIconTBButtonInfo, sizeof(TBBUTTONINFO), NULL))
    {
      break;
    }

    if (!ReadProcessMemory(hProcess, (void*)trayIconTBButtonInfo.lParam, (LPVOID)&trayIconData, sizeof(TRAYICONDATA), NULL))
    {
      break;
    }

    VirtualFreeEx(hProcess, tbButtonInfoRemote, 0, MEM_RELEASE);

    SendMessage(trayhWnd, TB_GETBUTTONTEXT, trayIconTBButtonInfo.idCommand, (LPARAM)sTipRemote);

    if (!ReadProcessMemory(hProcess, sTipRemote, (LPVOID)&sTipText, sizeof(sTipText), NULL))
    {
      break;
    }

    VirtualFreeEx(hProcess, sTipRemote, 0, MEM_RELEASE);

    hidden = ((trayIconData.dwState & NIS_HIDDEN) == NIS_HIDDEN);
    shared = ((trayIconData.dwState & NIS_SHAREDICON) == NIS_SHAREDICON);

    if (trayIconData.hIcon)
    {
      uFlags = uFlags | NIF_ICON;
    }
    if (trayIconData.uCallbackMessage)
    {
      uFlags = uFlags | NIF_MESSAGE;
    }
    if (wcslen(sTipText) > 0)
    {
      uFlags = uFlags | NIF_TIP;
    }

    AddTrayIcon(trayIconData.hWnd, trayIconData.uID, uFlags,
                trayIconData.uCallbackMessage, trayIconData.hIcon,
                (LPTSTR)sTipText, NULL, NULL, 0, hidden, shared);
  }

  CloseHandle(hProcess);

  return (counter == trayButtonCount);
}

Applet::~Applet()
{
  // Remove the tooltip region
  SendMessage(toolWnd, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);

  // A quit message was received, so unload the 2K/XP system icons and the Explorer tray hook

  if (ELIsExplorerShell()) //we're probably running on top of Explorer
  {
    removeExplorerTrayHook();
  }

  DestroyWindow(trayWnd);

  // Cleanup the icon vectors
  EnterCriticalSection(&trayVectorCS);
  while (!trayIconList.empty())
  {
    trayIconList.front()->DeleteBalloon();
    trayIconList.erase(trayIconList.begin());
  }
  LeaveCriticalSection(&trayVectorCS);

  DeleteCriticalSection(&trayVectorCS);

  // Return control to the default shell
  SendNotifyMessage(HWND_BROADCAST, RegisterWindowMessage(TEXT("TaskbarCreated")), 0, 0);
}

LRESULT Applet::DoNotify(HWND hwnd, LPARAM lParam)
{
  LPNMHDR pnmh = (LPNMHDR)lParam;

  // Fetch tooltip text
  if (pnmh->code == TTN_NEEDTEXT)
  {
    LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam;
    std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;
    POINT pt;
    RECT rt;

    rt = ELGetWindowRect(hwnd);
    GetCursorPos(&pt);
    pt.x -= rt.left;
    pt.y -= rt.top;

    // Traverse the valid icon vector to see if the mouse is in the bounding rectangle
    // of the current icon
    iter = trayIconList.begin();
    while (iter != trayIconList.end())
    {
      if (PtInRect((*iter)->GetRect(), pt))
      {
        // Update the tooltip
        lpttt->lpszText = (*iter)->GetTip();
        SetWindowPos(toolWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

        return 0;
      }

      ++iter;
    }

    lpttt->lpszText = (WCHAR*)TEXT("\0");
  }

  return 1;
}

UINT Applet::Initialize()
{
  WNDCLASSEX wincl;
  ZeroMemory(&wincl, sizeof(WNDCLASSEX));

  pSettings = std::tr1::shared_ptr<Settings>(new Settings(reinterpret_cast<LPARAM>(this)));
  UINT ret = BaseApplet::Initialize(WindowProcedure, this, pSettings);
  if (ret == 0)
  {
    return ret;
  }

  // Register the window class
  wincl.hInstance = mainInst;
  wincl.lpszClassName = szTrayName;
  wincl.lpfnWndProc = TrayProcedure;
  wincl.cbSize = sizeof (WNDCLASSEX);
  wincl.style = CS_DBLCLKS;
  wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
  wincl.hbrBackground = NULL;

  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  wincl.lpszClassName = szNotifyName;
  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  wincl.lpszClassName = szClockName;
  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  wincl.lpszClassName = szTaskSwName;
  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  wincl.lpszClassName = szReBarName;
  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  trayWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szTrayName, NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!trayWnd)
  {
    return 0;
  }

  notifyWnd = CreateWindowEx(0, szNotifyName, NULL, WS_CHILDWINDOW,
                             0, 0, 0, 0, trayWnd, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!notifyWnd)
  {
    return 0;
  }

  clockWnd = CreateWindowEx(0, szClockName, NULL, WS_CHILDWINDOW,
                            0, 0, 0, 0, notifyWnd, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!clockWnd)
  {
    return 0;
  }

  rebarWnd = CreateWindowEx(0, szReBarName, NULL, WS_CHILDWINDOW,
                            0, 0, 0, 0, trayWnd, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!rebarWnd)
  {
    return 0;
  }

  taskWnd = CreateWindowEx(0, szTaskSwName, NULL, WS_CHILDWINDOW,
                           0, 0, 0, 0, rebarWnd, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!rebarWnd)
  {
    return 0;
  }

  SetProp(trayWnd, TEXT("AllowConsentToStealFocus"), (HANDLE)1);
  SetProp(trayWnd, TEXT("TaskBandHWND"), trayWnd);

  movesizeinprogress = false;

  // Set the window transparency
  UpdateGUI();

  // Build the sticky list only after reading the settings (in UpdateGUI())
  pSettings->BuildHideList();

  // Tell the applications that a systray was created
  SendNotifyMessage(HWND_BROADCAST, RegisterWindowMessage(TEXT("TaskbarCreated")), 0, 0);

  return 1;
}

UINT Applet::PortableInitialize()
{
  HWND explorerTrayWnd;

  WNDCLASSEX wincl;
  ZeroMemory(&wincl, sizeof(WNDCLASSEX));

  pSettings = std::tr1::shared_ptr<Settings>(new Settings(reinterpret_cast<LPARAM>(this)));
  UINT ret = BaseApplet::Initialize(WindowProcedure, this, pSettings);
  if (ret == 0)
  {
    return ret;
  }

  // Register the window class
  wincl.hInstance = mainInst;
  wincl.lpszClassName = edTrayName;
  wincl.lpfnWndProc = TrayProcedure;
  wincl.cbSize = sizeof (WNDCLASSEX);
  wincl.style = CS_DBLCLKS;
  wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
  wincl.hbrBackground = NULL;

  if (!RegisterClassEx (&wincl))
  {
    return 0;
  }

  trayWnd = CreateWindowEx(WS_EX_TOOLWINDOW, edTrayName, NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, mainInst, reinterpret_cast<LPVOID>(this));
  if (!trayWnd)
  {
    return 0;
  }

  AcquireExplorerTrayIconList(); //it's likely the 2K/XP system icons are already showing in the Explorer tray; load them from Explorer

  InjectExplorerTrayHook(trayWnd);

  explorerTrayWnd = FindWindow(szTrayName, NULL);
  SetProp(explorerTrayWnd, TEXT("AllowConsentToStealFocus"), (HANDLE)1);
  SetProp(explorerTrayWnd, TEXT("TaskBandHWND"), trayWnd);

  // In Windows 7 (and possibly Vista) Explorer seems to keep it's hidden tray
  // icons in a separate location.  In order for emergeTray to pull them from
  // Explorer, they must first all be made visible.
  /*DWORD enableAutoTray = 0;
    ULONG_PTR result = 0;
    HKEY explorerKey;
    HWND trayWnd = FindWindow(szTrayName, NULL);
    if (trayWnd)
    {
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
    0,
    KEY_ALL_ACCESS,
    &explorerKey) == ERROR_SUCCESS)
    {
    if (RegSetValueEx(explorerKey,
    TEXT("EnableAutoTray"),
    0,
    REG_DWORD,
    (BYTE*)&enableAutoTray,
    sizeof(enableAutoTray)) == ERROR_SUCCESS)
    {
  // TODO (Chris#1#): Figure out a way to have Explorer take the EnableAutoTray settings change into effect.
  SendMessage(trayWnd, WM_WININICHANGE, 0, 0);
  SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0,
  SMTO_ABORTIFHUNG, 2000, &result);
  }

  RegCloseKey(explorerKey);
  }
  }*/

  movesizeinprogress = false;

  // Set the window transparency
  UpdateGUI();

  // Build the sticky list only after reading the settings (in UpdateGUI())
  pSettings->BuildHideList();

  return 1;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	PaintIcons
// Required:	HDC hdc - Device contect of calling window
// 		RECT r - client area of the calling window
// Returns:	LRESULT
// Purpose:	Paints the icons on the calling window
//----  --------------------------------------------------------------------------------------------------------
bool Applet::PaintItem(HDC hdc, size_t index, int x, int y, RECT rect)
{
  if (index > trayIconList.size())
  {
    return false;
  }

  EnterCriticalSection(&trayVectorCS);
  TrayIcon* pTrayIcon = trayIconList.at(index).get();

  // ignore hidden or invalid icons
  if (!IsIconVisible(pTrayIcon))
  {
    return false;
  }

  pTrayIcon->SetRect(rect);

  // Convert the icon
  pTrayIcon->CreateNewIcon(guiInfo.alphaForeground, guiInfo.alphaBackground);

  // Draw the icon
  DrawIconEx(hdc, x, y, pTrayIcon->GetIcon(), pSettings->GetIconSize(),
             pSettings->GetIconSize(), 0, NULL, DI_NORMAL);

  LeaveCriticalSection(&trayVectorCS);
  return true;
}

LRESULT Applet::DoTimer(UINT timerID)
{
  LRESULT res;

  if (movesizeinprogress)
  {
    return 1;
  }

  if (timerID == MOUSE_TIMER)
  {
    CleanTray();
  }

  res = BaseApplet::DoTimer(timerID);
  if ((res == 0) && !mouseOver)
  {
    ShowHiddenIcons(false, false);
  }

  return res;
}

void Applet::ShowConfig()
{
  Config config(mainInst, mainWnd, appletName, pSettings);
  if (config.Show() == IDOK)
  {
    ShowHiddenIcons(false, true);
    SortIcons();
    UpdateGUI();
  }
}

LRESULT Applet::DoSetCursor()
{
  if (!mouseOver)
  {
    ShowHiddenIcons(true, false);
  }

  return BaseApplet::DoSetCursor();
}

LRESULT Applet::DoEnterSizeMove(HWND hwnd)
{
  movesizeinprogress = true;
  return BaseApplet::DoEnterSizeMove(hwnd);
}

void Applet::UpdateIcons()
{
  EnterCriticalSection(&trayVectorCS);
  for (UINT i = 0; i < trayIconList.size(); i++)
  {
    trayIconList[i]->UpdateIcon();
  }
  LeaveCriticalSection(&trayVectorCS);
}

LRESULT Applet::DoSizing(HWND hwnd, UINT edge, LPRECT rect)
{
  UpdateIcons();
  return BaseApplet::DoSizing(hwnd, edge, rect);
}

LRESULT Applet::MyMove()
{
  RECT winRect, clientRect;
  movesizeinprogress = false;

  winRect = ELGetWindowRect(mainWnd);
  SetWindowPos(trayWnd, NULL, winRect.left, winRect.top, (winRect.right - winRect.left), (winRect.bottom - winRect.top), SWP_NOZORDER | SWP_NOACTIVATE);

  GetClientRect(trayWnd, &clientRect);
  SetWindowPos(notifyWnd, NULL, clientRect.left, clientRect.top, (clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top), SWP_NOZORDER | SWP_NOACTIVATE);
  SetWindowPos(rebarWnd, NULL, clientRect.left, clientRect.top, (clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top), SWP_NOZORDER | SWP_NOACTIVATE);
  SetWindowPos(taskWnd, NULL, clientRect.left, clientRect.top, (clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top), SWP_NOZORDER | SWP_NOACTIVATE);
  SetWindowPos(clockWnd, NULL, clientRect.left, clientRect.top, (clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top), SWP_NOZORDER | SWP_NOACTIVATE);

  return 0;
}

LRESULT Applet::MySize(LPARAM lParam)
{
  UINT dragBorder = guiInfo.dragBorder + guiInfo.bevelWidth + guiInfo.padding;

  // fill in the TOOLINFO structure
  ZeroMemory(&ti, sizeof(TOOLINFO));
  ti.cbSize = TTTOOLINFOW_V2_SIZE;
  ti.hwnd = mainWnd;
  ti.uId = (ULONG_PTR)mainWnd;
  ti.hinst =  mainInst;
  ti.uFlags = TTF_SUBCLASS;
  ti.lpszText = LPSTR_TEXTCALLBACK;

  // Remove the tooltip region
  SendMessage(toolWnd, TTM_DELTOOL, 0, (LPARAM)&ti);

  ti.rect.left = dragBorder;
  ti.rect.top = dragBorder;
  ti.rect.right = ti.rect.left + LOWORD(lParam) - dragBorder;
  ti.rect.bottom = ti.rect.top + HIWORD(lParam) - dragBorder;

  // Add the main window as a tooltip region
  SendMessage(toolWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);

  movesizeinprogress = false;

  return 0;
}

void Applet::AppletUpdate()
{
  UpdateIcons();
}

void Applet::ShowHiddenIcons(bool cmd, bool force)
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;
  RECT wndRect;

  if (((pSettings->GetUnhideIcons()) || (force)) && (!movesizeinprogress))
  {
    // Go through each of the elements in the trayIcons array
    EnterCriticalSection(&trayVectorCS);
    for (iter = trayIconList.begin(); iter != trayIconList.end(); ++iter)
    {
      if (pSettings->CheckHide((*iter)->GetTip()))
      {
        (*iter)->SetHidden(!cmd);
      }
    }
    LeaveCriticalSection(&trayVectorCS);

    // Added call here to ensure the icons would be sorted
    // even if one was modified during mouseover (unhide)
    SortIcons();

    if (pSettings->GetAutoSize())
    {
      wndRect = ELGetWindowRect(mainWnd);
      if (!IsRectEmpty(&wndRect))
      {
        AdjustRect(&wndRect);
        SetWindowPos(mainWnd, NULL, wndRect.left, wndRect.top,
                     wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
      }
    }
    DrawAlphaBlend();
  }
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	CleanTray
// Required:	Nothing
// Returns:	Nothing
//----  --------------------------------------------------------------------------------------------------------
void Applet::CleanTray()
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;
  RECT wndRect;
  bool hidden, removed = false;
  UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE;

  if (trayIconList.empty())
  {
    return;
  }

  // Go through each of the elements in the trayIcons array
  EnterCriticalSection(&trayVectorCS);
  for (iter = trayIconList.begin(); iter != trayIconList.end();)
  {
    if (!IsWindow((*iter)->GetWnd()))
    {
      hidden = (*iter)->GetHidden();

      (*iter)->HideBalloon();
      (*iter)->DeleteBalloon();
      removed = true;

      if (!hidden)
      {
        if (pSettings->GetAutoSize())
        {
          wndRect = ELGetWindowRect(mainWnd);
          if (!IsRectEmpty(&wndRect))
          {
            AdjustRect(&wndRect);
            if (GetVisibleIconCount() == 0)
            {
              SWPFlags |= SWP_HIDEWINDOW;
            }
            SetWindowPos(mainWnd, NULL, wndRect.left, wndRect.top,
                         wndRect.right - wndRect.left,
                         wndRect.bottom - wndRect.top,
                         SWPFlags);
          }
        }

      }

      iter = trayIconList.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  LeaveCriticalSection(&trayVectorCS);

  if (removed)
  {
    DrawAlphaBlend();
  }
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	FindTrayIcon
// Required:	NOTIFYICONDATA *iconData - extracted icon data
// Returns:	trayIconItem
// Purpose:	Determines if an icon is already in the valid icon
// 		vector and returns it
//----  --------------------------------------------------------------------------------------------------------
TrayIcon* Applet::FindTrayIcon(HWND hwnd, UINT uID)
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;

  // Traverse the trayIcons array
  EnterCriticalSection(&trayVectorCS);
  for (iter = trayIconList.begin(); iter != trayIconList.end(); ++iter)
  {
    // If the window and uID match an entry in the trayIcons array, return its position
    if ((*iter)->GetWnd() == hwnd && (*iter)->GetID() == uID)
    {
      break;
    }
  }
  LeaveCriticalSection(&trayVectorCS);

  if (iter == trayIconList.end())
  {
    return NULL;
  }

  return (*iter).get();
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	RemoveTrayIcon
// Required:	trayIconItem icon - pointer to icon in valid icon vector
// Returns:	LRESULT
// Purpose:	Removes an icon from the valid icon vector
//----  --------------------------------------------------------------------------------------------------------
LRESULT Applet::RemoveTrayIcon(HWND hwnd, UINT uID)
{
  TrayIcon* pTrayIcon = FindTrayIcon(hwnd, uID);
  RECT wndRect;
  UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE;

  if (!pTrayIcon)
  {
    return 0;
  }

  bool hidden = pTrayIcon->GetHidden();

  /*  if (activeIcon != NULL)
      {
      if (activeIcon->GetWnd() == (*iter)->GetWnd())
      {
      SendMessage(activeIcon->GetWnd(), activeIcon->GetCallback(), MAKEWPARAM(0, 0),
      MAKELPARAM(NIN_POPUPCLOSE, activeIcon->GetID()));
      activeIcon = NULL;
      }
      }*/

  pTrayIcon->HideBalloon();
  pTrayIcon->DeleteBalloon();
  RemoveTrayIconListItem(pTrayIcon);

  if (!hidden)
  {
    if (pSettings->GetAutoSize())
    {
      wndRect = ELGetWindowRect(mainWnd);
      if (IsRectEmpty(&wndRect))
      {
        AdjustRect(&wndRect);
        if (GetVisibleIconCount() == 0)
        {
          SWPFlags |= SWP_HIDEWINDOW;
        }
        SetWindowPos(mainWnd, NULL, wndRect.left, wndRect.top,
                     wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                     SWPFlags);
      }
    }

    DrawAlphaBlend();
  }

  return 1;
}

LRESULT Applet::SetTrayIconVersion(HWND hwnd, UINT uID, UINT iconVersion)
{
  TrayIcon* pTrayIcon = FindTrayIcon(hwnd, uID);

  if (!pTrayIcon)
  {
    return 0;
  }

  pTrayIcon->SetIconVersion(iconVersion);
  if ((iconVersion == NOTIFYICON_VERSION_4) && ((pTrayIcon->GetFlags() & NIF_SHOWTIP) != NIF_SHOWTIP))
  {
    pTrayIcon->SetTip((WCHAR*)TEXT(""));
  }

  return 1;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	ModifyTrayIcon
// Required:	trayIconItem icon - pointer to icon in valid icon vector
// 		NOTIFYICONDATA *iconData - extracted icon data
// 		LPTSTR newTip - new icon tip
// Returns:	LRESULT
// Purpose:	Updated the existing valid icon with the new data
//----  --------------------------------------------------------------------------------------------------------
LRESULT Applet::ModifyTrayIcon(HWND hwnd, UINT uID, UINT uFlags, UINT uCallbackMessage,
                               HICON icon, LPTSTR newTip, LPTSTR newInfo,
                               LPTSTR newInfoTitle, DWORD newInfoFlags, bool hidden,
                               bool shared)
{
  CleanTray();

  bool adjust = false;
  bool changed = false;
  TrayIcon* pTrayIcon = FindTrayIcon(hwnd, uID);
  RECT wndRect;
  UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE;

  if (!pTrayIcon)
  {
    return 0;
  }

  if ((uFlags & NIF_ICON) == NIF_ICON)
  {
    if (pTrayIcon->SetIcon(icon))
    {
      pTrayIcon->SetFlags(pTrayIcon->GetFlags() | NIF_ICON);
      adjust = true;
      changed = true;
    }
  }

  /* Some icons (i.e. network icons) will be displayed when they shouldn't be,
   * at least until a WM_MOUSEOVER.  They seem to have no TIP, so proactively
   * send icons with no tip the WM_MOUSEOVER message.
   */
  if ((uFlags & NIF_TIP) == NIF_TIP)
  {
    if (pTrayIcon->SetTip(newTip))
    {
      pTrayIcon->SetFlags(pTrayIcon->GetFlags() | NIF_TIP);
      changed = true;
    }
  }
  else
  {
    pTrayIcon->SendMessage(LPARAM(WM_MOUSEMOVE));
  }

  /* Due to some tray icons getting their tip text delayed (i.e. network icons)
   * check to see if they should be hidden based on tip text prior to using the
   * 'hidden' value.  However, do not re-hide them if the mouse is over the
   * applet.
   */
  if (pSettings->CheckHide(pTrayIcon->GetTip()))
  {
    if (!pSettings->GetUnhideIcons() || (!mouseOver && pSettings->GetUnhideIcons()))
    {
      pTrayIcon->SetHidden(true);
      adjust = true;
      changed = true;
    }
  }
  else
  {
    if (hidden != pTrayIcon->GetHidden())
    {
      pTrayIcon->SetHidden(hidden);
      adjust = true;
      changed = true;
    }
  }

  pTrayIcon->SetShared(shared);

  if ((uFlags & NIF_MESSAGE) == NIF_MESSAGE)
  {
    if (pTrayIcon->SetCallback(uCallbackMessage))
    {
      pTrayIcon->SetFlags(pTrayIcon->GetFlags() | NIF_MESSAGE);
      changed = true;
    }
  }

  // Only show balloon messages if not running on top of Explorer.
  if (((uFlags & NIF_INFO) == NIF_INFO) && !ELIsExplorerShell())
  {
    pTrayIcon->ShowBalloon(newInfoTitle, newInfo, newInfoFlags, icon);
    pTrayIcon->SetFlags(pTrayIcon->GetFlags() | NIF_INFO);
  }

  if (adjust)
  {
    SortIcons();
    if (pSettings->GetAutoSize())
    {
      wndRect = ELGetWindowRect(mainWnd);
      if (!IsRectEmpty(&wndRect))
      {
        AdjustRect(&wndRect);
        if ((GetVisibleIconCount() > 0) && !appletHidden && !fullScreen)
        {
          SWPFlags |= SWP_SHOWWINDOW;
        }
        SetWindowPos(mainWnd, NULL, wndRect.left, wndRect.top,
                     wndRect.right - wndRect.left,
                     wndRect.bottom - wndRect.top,
                     SWPFlags);
      }
    }
  }

  if (changed)
  {
    DrawAlphaBlend();
  }

  return 1;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	AddTrayIcon
// Required:	NOTIFYICONDATA *iconData - extracted icon data
// 		LPTSTR szTip - icon tip
// Returns:	LRESULT
// Purpose:	Adds a valid icon to the valid icon array
//----  --------------------------------------------------------------------------------------------------------
LRESULT Applet::AddTrayIcon(HWND hwnd, UINT uID, UINT uFlags, UINT uCallbackMessage,
                            HICON icon, LPTSTR szTip, LPTSTR szInfo, LPTSTR szInfoTitle,
                            DWORD dwInfoFlags, bool hidden, bool shared)
{
  CleanTray();

  TrayIcon* pTrayIcon = FindTrayIcon(hwnd, uID);
  RECT wndRect;
  UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE;

  if (pTrayIcon)
  {
    return 0;
  }

  pTrayIcon = new TrayIcon(mainInst, hwnd, uID, mainWnd, pSettings.get());

  if ((uFlags & NIF_MESSAGE) == NIF_MESSAGE)
  {
    pTrayIcon->SetCallback(uCallbackMessage);
  }

  if ((uFlags & NIF_INFO) == NIF_INFO)
  {
    pTrayIcon->ShowBalloon(szInfoTitle, szInfo, dwInfoFlags, icon);
  }

  /* Some icons (i.e. network icons) will be displayed when they shouldn't be,
   * at least until a WM_MOUSEOVER.  They seem to have no TIP, so proactively
   * send icons with no tip the WM_MOUSEOVER message.
   */
  if ((uFlags & NIF_TIP) == NIF_TIP)
  {
    pTrayIcon->SetTip(szTip);
  }
  else
  {
    pTrayIcon->SendMessage(LPARAM(WM_MOUSEMOVE));
  }

  pTrayIcon->SetFlags(uFlags);
  if (pSettings->CheckHide(szTip))
  {
    pTrayIcon->SetHidden(true);
  }
  else
  {
    pTrayIcon->SetHidden(hidden);
  }
  pTrayIcon->SetShared(shared);

  if ((uFlags & NIF_ICON) == NIF_ICON)
  {
    pTrayIcon->SetIcon(icon);
  }

  EnterCriticalSection(&trayVectorCS);
  trayIconList.push_back( std::tr1::shared_ptr<TrayIcon>(pTrayIcon) );
  LeaveCriticalSection(&trayVectorCS);

  SortIcons();

  if (!hidden)
  {
    if (pSettings->GetAutoSize())
    {
      wndRect = ELGetWindowRect(mainWnd);
      if (!IsRectEmpty(&wndRect))
      {
        AdjustRect(&wndRect);
        if ((GetVisibleIconCount() > 0) && !appletHidden && !fullScreen)
        {
          SWPFlags |= SWP_SHOWWINDOW;
        }
        SetWindowPos(mainWnd, NULL, wndRect.left, wndRect.top,
                     wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                     SWPFlags);
      }
    }

    DrawAlphaBlend();
  }

  return 1;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	TrayMouseEvent
// Required:	UINT message - action to handle
// 		LPARAM lParam - location of the mouse
// Returns:	bool
// Purpose:	Forwards the mouse message to the icon window
//----  --------------------------------------------------------------------------------------------------------
LRESULT Applet::TrayMouseEvent(UINT message, LPARAM lParam)
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;
  POINT lparamPT, cursorPT;
  DWORD processID = 0;

  WCHAR tmp[MAX_LINE_LENGTH], error[MAX_LINE_LENGTH];
  ZeroMemory(tmp, MAX_LINE_LENGTH);

  lparamPT.x = LOWORD(lParam);
  lparamPT.y = HIWORD(lParam);
  GetCursorPos(&cursorPT);

  // Traverse the valid icon vector to see if the mouse is in the bounding rectangle
  // of the current icon
  EnterCriticalSection(&trayVectorCS);
  for (iter = trayIconList.begin(); iter != trayIconList.end(); ++iter)
  {
    if (PtInRect((*iter)->GetRect(), lparamPT) && !(*iter)->GetHidden())
    {
      if (oldTipIcon != (*iter).get())
      {
        oldTipIcon = (*iter).get();
        SendMessage(toolWnd, TTM_UPDATE, 0, 0);
      }

      //  If holding down ALT while left-click, add tip to
      //  hideList and hide the icon from the tray.
      if (ELIsKeyDown(VK_MENU) && (message == WM_LBUTTONDOWN))
      {
        if (wcscmp((*iter)->GetTip(), TEXT("\0")) == 0) //((*iter)->GetTip())
        {
          swprintf(error, TEXT("This icon cannot be hidden because it has no text (tooltip)."), tmp);
          ELMessageBox(GetDesktopWindow(), error, TEXT("emergeTray"),
                       ELMB_OK | ELMB_ICONERROR | ELMB_MODAL);
          return 0;
        }
        if (!pSettings->CheckHide((*iter)->GetTip()))
        {
          pSettings->AddHideListItem((*iter)->GetTip());
          pSettings->WriteHideList();
        }

        if (!pSettings->GetUnhideIcons())
        {
          (*iter)->SetHidden(true);
          UpdateGUI();
        }

        ShowHiddenIcons(false, true);
        SortIcons();
        UpdateGUI();

        return 0;
      }

      GetWindowThreadProcessId((*iter)->GetWnd(), &processID);
      AllowSetForegroundWindow(processID);

      switch (message)
      {
      case WM_MOUSEMOVE:
        if ((ELOSVersionInfo() >= 6.0) && (((*iter)->GetFlags() & NIF_INFO) == NIF_INFO))
        {
          if (activeIcon != NULL)
          {
            if( activeIcon->GetWnd() != (*iter)->GetWnd())
            {
              activeIcon->SendMessage(NIN_POPUPCLOSE);
              activeIcon = (*iter).get();
            }
          }
          message = NIN_POPUPOPEN;
        }
        break;

        /**< For some reason some icons require WM_USER being passed to activate the left click action */
      case WM_LBUTTONUP:
        (*iter)->SendMessage(WM_USER);
        break;

      case WM_RBUTTONUP:
        (*iter)->SendMessage(message);
        message = WM_CONTEXTMENU;
        break;
      }

      (*iter)->SendMessage(message);
      return 0;
    }
  }
  LeaveCriticalSection(&trayVectorCS);

  return 1;
}

bool Applet::RemoveAppBar(AppBar* pAppBar)
{
  std::vector< std::tr1::shared_ptr<AppBar> >::iterator iter;

  for (iter = barList.begin(); iter != barList.end(); ++iter)
  {
    if ((*iter).get() == pAppBar)
    {
      barList.erase(iter);
      return true;
    }
  }

  return false;
}

bool Applet::RemoveTrayIconListItem(TrayIcon* pTrayIcon)
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator iter;
  bool ret = false;

  EnterCriticalSection(&trayVectorCS);
  for (iter = trayIconList.begin(); iter != trayIconList.end(); ++iter)
  {
    if ((*iter).get() == pTrayIcon)
    {
      trayIconList.erase(iter);
      ret = true;
      break;
    }
  }
  LeaveCriticalSection(&trayVectorCS);

  return ret;
}

AppBar* Applet::FindAppBar(APPBARDATA* pAppBarData)
{
  std::vector< std::tr1::shared_ptr<AppBar> >::iterator iter;

  for (iter = barList.begin(); iter != barList.end(); ++iter)
  {
    if ((*iter)->IsEqual(pAppBarData))
    {
      return (*iter).get();
    }
  }

  return NULL;
}

LRESULT Applet::IconIdentifierEvent(COPYDATASTRUCT* cpData)
{
  POINT cursorPos;
  ICONIDENTIFIERDATA* iid;

  GetCursorPos(&cursorPos);

  iid = (PICONIDENTIFIERDATA)cpData->lpData;
  switch (iid->dwMessage)
  {
  case 2:
    return MAKELONG(16, 16);
  default:
    return MAKELONG(cursorPos.x, cursorPos.y);
  }

  return 0;
}

bool Applet::SetAutoHideEdge(UINT edge)
{
  if ((edge == ABE_TOP) && !autoHideTop)
  {
    autoHideTop = true;
    return true;
  }
  else if ((edge == ABE_BOTTOM) && !autoHideBottom)
  {
    autoHideBottom = true;
    return true;
  }
  else if ((edge == ABE_LEFT) && !autoHideLeft)
  {
    autoHideLeft = true;
    return true;
  }
  else if ((edge == ABE_RIGHT) && !autoHideRight)
  {
    autoHideRight = true;
    return true;
  }

  return false;
}

bool Applet::ClearAutoHideEdge(UINT edge)
{
  if (edge == ABE_TOP)
  {
    autoHideTop = false;
  }
  else if (edge == ABE_BOTTOM)
  {
    autoHideBottom = false;
  }
  else if (edge == ABE_LEFT)
  {
    autoHideLeft = false;
  }
  else if (edge == ABE_RIGHT)
  {
    autoHideRight = false;
  }

  return true;
}

LRESULT Applet::AppBarEvent(COPYDATASTRUCT* cpData)
{
  DWORD message = 0, processID = 0;
  HANDLE sharedMem = NULL;
  APPBARDATA appBarData, *pAppBarData;
  AppBar* pAppBar;
  RECT trayRect;
  bool is64Bit = true;
  std::vector< std::tr1::shared_ptr<AppBar> >::iterator iter;
  UINT offset = 0;

  GetWindowRect(trayWnd, &trayRect);

  pAppBarData = (APPBARDATA*)cpData->lpData;
  if (!IsWindow(pAppBarData->hWnd))
  {
    APPBARDATA_WOW32* pAppBarData32 = (APPBARDATA_WOW32*)cpData->lpData;
    if (!IsWindow(MAKEHWND(pAppBarData32->hWnd)))
    {
      return 0;
    }

    appBarData.cbSize = pAppBarData32->cbSize;
    appBarData.hWnd = MAKEHWND(pAppBarData32->hWnd);
    appBarData.lParam = pAppBarData32->lParam;
    CopyRect(&appBarData.rc, &pAppBarData32->rc);
    appBarData.uCallbackMessage = pAppBarData32->uCallbackMessage;
    appBarData.uEdge = pAppBarData32->uEdge;

    pAppBarData = &appBarData;
    is64Bit = false;
  }

  message = *(DWORD*) (((BYTE*)cpData->lpData) + pAppBarData->cbSize);
  offset = sizeof(DWORD);
  if (ELOSVersionInfo() > 6.0)
  {
    offset += sizeof(DWORD);
  }
  sharedMem = *(HANDLE*) (((BYTE*)cpData->lpData) + pAppBarData->cbSize + offset);
  processID = *(DWORD*) (((BYTE*)cpData->lpData) + pAppBarData->cbSize + offset + sizeof(HANDLE));

  switch (message)
  {
  case ABM_ACTIVATE:
    return 1;

  case ABM_GETSTATE:
    {
      LRESULT result = 0;

      if (!IsWindowVisible(mainWnd))
      {
        result = ABS_AUTOHIDE;
      }

      //if (ELOSVersionInfo() > 6.0)
      if (IsWindows7OrGreater()) //according to MSDN, as of Windows 7 the taskbar is always in a ABS_ALWAYSONTOP state
      {
        result |= ABS_ALWAYSONTOP;
      }
      else
      {
        if (ELToLower(pSettings->GetZPosition()) == TEXT("top"))
        {
          result |= ABS_ALWAYSONTOP;
        }
      }

      return result;
    }

  case ABM_SETSTATE:
    return 1;

  case ABM_WINDOWPOSCHANGED:
    return 1;

  case ABM_GETAUTOHIDEBAR:
    for (iter = barList.begin(); iter != barList.end(); ++iter)
    {
      if ((*iter)->IsAutoHide() && ((*iter)->GetEdge() == pAppBarData->uEdge))
      {
        return (LRESULT)(*iter)->GetWnd();
      }
    }
    break;

  case ABM_SETAUTOHIDEBAR:
    pAppBar = FindAppBar(pAppBarData);
    if (pAppBar)
    {
      if (pAppBarData->lParam)
      {
        if (SetAutoHideEdge(pAppBarData->uEdge))
        {
          return 1;
        }
      }
      else
      {
        ClearAutoHideEdge(pAppBar->GetEdge());
        return 1;
      }
    }
    break;

  case ABM_REMOVE:
    pAppBar = FindAppBar(pAppBarData);
    if (pAppBar)
    {
      if (pAppBar->IsAutoHide())
      {
        ClearAutoHideEdge(pAppBar->GetEdge());
      }
      RemoveAppBar(pAppBar);
      return 1;
    }
    break;

  case ABM_NEW:
    pAppBar = FindAppBar(pAppBarData);
    if (!pAppBar)
    {
      barList.push_back( std::tr1::shared_ptr<AppBar>(new AppBar(pAppBarData)) );
      return 1;
    }
    break;

  case ABM_QUERYPOS:
    pAppBar = FindAppBar(pAppBarData);
    if (pAppBar)
    {
      pAppBar->SetRect(pAppBarData->rc);
      pAppBar->SetEdge(pAppBarData->uEdge);

      if (is64Bit)
      {
        APPBARDATA* pSharedAppBarData = (APPBARDATA*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &pAppBarData->rc);
          ELUnlockShared(pSharedAppBarData);
        }
      }
      else
      {
        APPBARDATA_WOW32* pSharedAppBarData = (APPBARDATA_WOW32*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &pAppBarData->rc);
          ELUnlockShared(pSharedAppBarData);
        }
      }

      return 1;
    }
    break;

  case ABM_SETPOS:
    pAppBar = FindAppBar(pAppBarData);
    if (pAppBar)
    {
      pAppBar->SetRect(pAppBarData->rc);
      pAppBar->SetEdge(pAppBarData->uEdge);

      if (is64Bit)
      {
        APPBARDATA* pSharedAppBarData = (APPBARDATA*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &pAppBarData->rc);
          ELUnlockShared(pSharedAppBarData);
        }
      }
      else
      {
        APPBARDATA_WOW32* pSharedAppBarData = (APPBARDATA_WOW32*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &pAppBarData->rc);
          ELUnlockShared(pSharedAppBarData);
        }
      }

      return 1;
    }
    break;

  case ABM_GETTASKBARPOS:
    if (sharedMem)
    {
      MONITORINFO appletMonitorInfo;
      appletMonitorInfo.cbSize = sizeof(MONITORINFO);
      HMONITOR appletMonitor = MonitorFromWindow(trayWnd, MONITOR_DEFAULTTONEAREST);
      UINT uEdge = ABE_BOTTOM;

      if (GetMonitorInfo(appletMonitor, &appletMonitorInfo))
      {
        int topDelta = abs(trayRect.top - appletMonitorInfo.rcMonitor.top);
        int bottomDelta = abs(appletMonitorInfo.rcMonitor.bottom - trayRect.bottom);
        int leftDelta = abs(trayRect.left - appletMonitorInfo.rcMonitor.left);
        int rightDelta = abs(appletMonitorInfo.rcMonitor.right - trayRect.right);

        if ((leftDelta < topDelta) && (leftDelta < bottomDelta) && (leftDelta < rightDelta))
        {
          uEdge = ABE_LEFT;
        }
        else if ((rightDelta < topDelta) && (rightDelta < bottomDelta) && (rightDelta < leftDelta))
        {
          uEdge = ABE_RIGHT;
        }
        else if (topDelta < bottomDelta)
        {
          uEdge = ABE_TOP;
        }
      }

      if (is64Bit)
      {
        APPBARDATA* pSharedAppBarData = (APPBARDATA*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &trayRect);
          pSharedAppBarData->uEdge = uEdge;
          ELUnlockShared(pSharedAppBarData);
          return 1;
        }
      }
      else
      {
        APPBARDATA_WOW32* pSharedAppBarData = (APPBARDATA_WOW32*)ELLockShared(sharedMem, processID);
        if (pSharedAppBarData)
        {
          CopyRect(&pSharedAppBarData->rc, &trayRect);
          pSharedAppBarData->uEdge = uEdge;
          ELUnlockShared(pSharedAppBarData);
          return 1;
        }
      }
    }
    break;
  }

  return 0;
}

APPBARDATA* Applet::LockAppBarMemory(HANDLE sharedMem, DWORD processID)
{
  if (sharedMem)
  {
    return (APPBARDATA*)ELLockShared(sharedMem, processID);
  }

  return NULL;
}

//----  --------------------------------------------------------------------------------------------------------
// Function:	TrayIconEvent
// Required:	LPARAM lParam - icon data
// Returns:	LRESULT
// Purpose:	Extracts the icon data and passes it to the appropriate function
//----  --------------------------------------------------------------------------------------------------------
LRESULT Applet::TrayIconEvent(COPYDATASTRUCT* cpData)
{
  bool hidden = false, shared = false;
  std::wstring wideString;
  WCHAR iconTip[TIP_SIZE], iconInfo[TIP_SIZE], infoTitle[TIP_SIZE];
  ZeroMemory(iconTip, TIP_SIZE);
  ZeroMemory(iconInfo, TIP_SIZE);
  ZeroMemory(infoTitle, TIP_SIZE);
  NID_5W* iconData5W;
  NID_5A* iconData5A;
  NID_4W* iconData4W;
  NID_4A* iconData4A;
  NID_5W_WOW32* iconData5W32;
  NID_5A_WOW32* iconData5A32;
  NID_4W_WOW32* iconData4W32;
  NID_4A_WOW32* iconData4A32;
  UINT iconVersion = 0, uID = 0, uCallbackMessage = 0, uFlags = 0;
  HICON icon = NULL;
  HWND hwnd = NULL;
  DWORD message = 0, infoFlags = 0;

  DWORD iconDataSize = 0;
  void* iconData = NULL;

  if (IsWindow(((PSHELLTRAYDATA)cpData->lpData)->iconData.hWnd))
  {
    message = ((PSHELLTRAYDATA)cpData->lpData)->dwMessage;
    iconDataSize = ((PSHELLTRAYDATA)cpData->lpData)->iconData.cbSize;
    iconData = &((PSHELLTRAYDATA)cpData->lpData)->iconData;
  }
  else if (IsWindow(MAKEHWND(((PSHELLTRAYDATAWOW32)cpData->lpData)->iconData.hWnd)))
  {
    message = ((PSHELLTRAYDATAWOW32)cpData->lpData)->dwMessage;
    iconDataSize = ((PSHELLTRAYDATAWOW32)cpData->lpData)->iconData.cbSize;
    iconData = &((PSHELLTRAYDATAWOW32)cpData->lpData)->iconData;
  }
  else
  {
    return 0;
  }

  switch (iconDataSize)
  {
  case sizeof(NID_5W):
  case sizeof(NID_6W):
  case sizeof(NID_7W):
    iconData5W = (NID_5W*)iconData;
    iconVersion = iconData5W->uVersion;
    hwnd = (HWND)iconData5W->hWnd;
    uID = iconData5W->uID;
    uCallbackMessage = iconData5W->uCallbackMessage;
    uFlags = iconData5W->uFlags;

    if ((uFlags & NIF_INFO) == NIF_INFO)
    {
      wcscpy(iconInfo, iconData5W->szInfo);
      iconInfo[TIP_SIZE - 1] = '0';
      wcscpy(infoTitle, iconData5W->szInfoTitle);
      infoTitle[TIP_SIZE - 1] = '0';
      infoFlags = iconData5W->dwInfoFlags;
    }

    if ((uFlags & NIF_TIP) == NIF_TIP)
    {
      wcscpy(iconTip, iconData5W->szTip);
      iconTip[TIP_SIZE - 1] = '0';
    }

    if ((uFlags & NIF_ICON) == NIF_ICON)
    {
      icon = MAKEHICON(iconData5W->hIcon);
    }

    if ((uFlags & NIF_STATE) == NIF_STATE)
    {
      if ((iconData5W->dwState & iconData5W->dwStateMask & NIS_HIDDEN) == NIS_HIDDEN)
      {
        hidden = true;
      }
      if ((iconData5W->dwState & iconData5W->dwStateMask & NIS_SHAREDICON) == NIS_SHAREDICON)
      {
        shared = true;
      }
    }
    break;

  case sizeof(NID_5A):
  case sizeof(NID_6A):
  case sizeof(NID_7A):
    iconData5A = (NID_5A*)iconData;
    iconVersion = iconData5A->uVersion;
    hwnd = (HWND)iconData5A->hWnd;
    uID = iconData5A->uID;
    uCallbackMessage = iconData5A->uCallbackMessage;
    uFlags = iconData5A->uFlags;

    if ((uFlags & NIF_INFO) == NIF_INFO)
    {
      wideString = ELstringTowstring(iconData5A->szInfo);
      wcsncpy(iconInfo, wideString.c_str(), TIP_SIZE);
      wideString = ELstringTowstring(iconData5A->szInfoTitle);
      wcsncpy(infoTitle, wideString.c_str(), TIP_SIZE);
      infoFlags = iconData5A->dwInfoFlags;
    }

    if ((uFlags & NIF_TIP) == NIF_TIP)
    {
      wideString = ELstringTowstring(iconData5A->szTip);
      wcsncpy(iconTip, wideString.c_str(), TIP_SIZE);
    }

    if ((uFlags & NIF_ICON) == NIF_ICON)
    {
      icon = MAKEHICON(iconData5A->hIcon);
    }

    if ((uFlags & NIF_STATE) == NIF_STATE)
    {
      if ((iconData5A->dwState & iconData5A->dwStateMask & NIS_HIDDEN) == NIS_HIDDEN)
      {
        hidden = true;
      }
      if ((iconData5A->dwState & iconData5A->dwStateMask & NIS_SHAREDICON) == NIS_SHAREDICON)
      {
        shared = true;
      }
    }
    break;

  case sizeof(NID_4W):
    iconData4W = (NID_4W*)iconData;
    hwnd = (HWND)iconData4W->hWnd;
    uID = iconData4W->uID;
    uCallbackMessage = iconData4W->uCallbackMessage;
    uFlags = iconData4W->uFlags;

    if ((uFlags & NIF_ICON) == NIF_ICON)
    {
      icon = MAKEHICON(iconData4W->hIcon);
    }

    if ((uFlags & NIF_TIP) == NIF_TIP)
    {
      wcsncpy(iconTip, iconData4W->szTip, TIP_SIZE);
      iconTip[TIP_SIZE - 1] = '\0';
    }
    break;

  case sizeof(NID_4A):
    iconData4A = (NID_4A*)iconData;
    hwnd = (HWND)iconData4A->hWnd;
    uID = iconData4A->uID;
    uCallbackMessage = iconData4A->uCallbackMessage;
    uFlags = iconData4A->uFlags;

    if ((uFlags & NIF_ICON) == NIF_ICON)
    {
      icon = MAKEHICON(iconData4A->hIcon);
    }

    if ((uFlags & NIF_TIP) == NIF_TIP)
    {
      wideString = ELstringTowstring(iconData4A->szTip);
      wcsncpy(iconTip, wideString.c_str(), TIP_SIZE);
    }
    break;
  }

  // If the icon notification type was not determined, attempt to check against 32-bit types.
  // This is required since x64 Windows makes use of both 32-bit and 64-bit notifications.
  if ((uID == 0) && (uCallbackMessage == 0) && (uFlags == 0) && (hwnd == NULL))
  {
    switch (iconDataSize)
    {
    case sizeof(NID_5W_WOW32):
    case sizeof(NID_6W_WOW32):
    case sizeof(NID_7W_WOW32):
      iconData5W32 = (NID_5W_WOW32*)iconData;
      iconVersion = iconData5W32->uVersion;
      hwnd = MAKEHWND(iconData5W32->hWnd);
      uID = iconData5W32->uID;
      uCallbackMessage = iconData5W32->uCallbackMessage;
      uFlags = iconData5W32->uFlags;

      if ((uFlags & NIF_INFO) == NIF_INFO)
      {
        wcscpy(iconInfo, iconData5W32->szInfo);
        iconInfo[TIP_SIZE - 1] = '0';
        wcscpy(infoTitle, iconData5W32->szInfoTitle);
        infoTitle[TIP_SIZE - 1] = '0';
        infoFlags = iconData5W32->dwInfoFlags;
      }

      if ((uFlags & NIF_TIP) == NIF_TIP)
      {
        wcscpy(iconTip, iconData5W32->szTip);
        iconTip[TIP_SIZE - 1] = '0';
      }

      if ((uFlags & NIF_ICON) == NIF_ICON)
      {
        icon = MAKEHICON(iconData5W32->hIcon);
      }

      if ((uFlags & NIF_STATE) == NIF_STATE)
      {
        if ((iconData5W32->dwState & iconData5W32->dwStateMask & NIS_HIDDEN) == NIS_HIDDEN)
        {
          hidden = true;
        }
        if ((iconData5W32->dwState & iconData5W32->dwStateMask & NIS_SHAREDICON) == NIS_SHAREDICON)
        {
          shared = true;
        }
      }
      break;

    case sizeof(NID_5A_WOW32):
    case sizeof(NID_6A_WOW32):
    case sizeof(NID_7A_WOW32):
      iconData5A32 = (NID_5A_WOW32*)iconData;
      iconVersion = iconData5A32->uVersion;
      hwnd = MAKEHWND(iconData5A32->hWnd);
      uID = iconData5A32->uID;
      uCallbackMessage = iconData5A32->uCallbackMessage;
      uFlags = iconData5A32->uFlags;

      if ((uFlags & NIF_INFO) == NIF_INFO)
      {
        wideString = ELstringTowstring(iconData5A32->szInfo);
        wcsncpy(iconInfo, wideString.c_str(), TIP_SIZE);
        wideString = ELstringTowstring(iconData5A32->szInfoTitle);
        wcsncpy(infoTitle, wideString.c_str(), TIP_SIZE);
        infoFlags = iconData5A32->dwInfoFlags;
      }

      if ((uFlags & NIF_TIP) == NIF_TIP)
      {
        wideString = ELstringTowstring(iconData5A32->szTip);
        wcsncpy(iconTip, wideString.c_str(), TIP_SIZE);
      }

      if ((uFlags & NIF_ICON) == NIF_ICON)
      {
        icon = MAKEHICON(iconData5A32->hIcon);
      }

      if ((uFlags & NIF_STATE) == NIF_STATE)
      {
        if ((iconData5A32->dwState & iconData5A32->dwStateMask & NIS_HIDDEN) == NIS_HIDDEN)
        {
          hidden = true;
        }
        if ((iconData5A32->dwState & iconData5A32->dwStateMask & NIS_SHAREDICON) == NIS_SHAREDICON)
        {
          shared = true;
        }
      }
      break;

    case sizeof(NID_4W_WOW32):
      iconData4W32 = (NID_4W_WOW32*)iconData;
      hwnd = MAKEHWND(iconData4W32->hWnd);
      uID = iconData4W32->uID;
      uCallbackMessage = iconData4W32->uCallbackMessage;
      uFlags = iconData4W32->uFlags;

      if ((uFlags & NIF_ICON) == NIF_ICON)
      {
        icon = MAKEHICON(iconData4W32->hIcon);
      }

      if ((uFlags & NIF_TIP) == NIF_TIP)
      {
        wcsncpy(iconTip, iconData4W32->szTip, TIP_SIZE);
        iconTip[TIP_SIZE - 1] = '\0';
      }
      break;

    case sizeof(NID_4A_WOW32):
      iconData4A32 = (NID_4A_WOW32*)iconData;
      hwnd = MAKEHWND(iconData4A32->hWnd);
      uID = iconData4A32->uID;
      uCallbackMessage = iconData4A32->uCallbackMessage;
      uFlags = iconData4A32->uFlags;

      if ((uFlags & NIF_ICON) == NIF_ICON)
      {
        icon = MAKEHICON(iconData4A32->hIcon);
      }

      if ((uFlags & NIF_TIP) == NIF_TIP)
      {
        wideString = ELstringTowstring(iconData4A32->szTip);
        wcsncpy(iconTip, wideString.c_str(), TIP_SIZE);
      }
      break;

    default:
      return 0;
    }
  }

  switch (message)
  {
    // Since icons don't always send the appropriate message,
    // do a little checking
  case NIM_ADD:
    return AddTrayIcon(hwnd, uID, uFlags, uCallbackMessage, icon, iconTip,
                       iconInfo, infoTitle, infoFlags, hidden, shared);
  case NIM_SETVERSION:
    return SetTrayIconVersion(hwnd, uID, iconVersion);
  case NIM_MODIFY:
    return ModifyTrayIcon(hwnd, uID, uFlags, uCallbackMessage, icon, iconTip,
                          iconInfo, infoTitle, infoFlags, hidden, shared);
  case NIM_DELETE:
    return RemoveTrayIcon(hwnd, uID);
  }

  return 0;
}

size_t Applet::GetTrayIconListSize()
{
  return trayIconList.size();
}

TrayIcon* Applet::GetTrayIconListItem(UINT index)
{
  return trayIconList[index].get();
}

// Used to sort the visible icons first, then the hidden ones
void Applet::SortIcons()
{
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator srcIter;
  std::vector< std::tr1::shared_ptr<TrayIcon> >::iterator dstIter;
  std::vector< std::tr1::shared_ptr<TrayIcon> > hiddenIconListTmp;

  EnterCriticalSection(&trayVectorCS);
  srcIter = trayIconList.begin();
  dstIter = srcIter;

  // iterate full icon list, collecting visible icons at the beginning and remembering hidden ones for later
  for(; srcIter != trayIconList.end(); ++srcIter)
  {
    // hidden icon? remember it for later and skip to next icon
    if((*srcIter)->GetHidden())
    {
      hiddenIconListTmp.push_back((*srcIter));
      continue;
    }

    // copy visible icon to correct position if needed (i.e. hidden icons were skipped)
    if(dstIter != srcIter)
    {
      (*dstIter) = (*srcIter);
    }
    ++dstIter;
  }

  // now copy back hidden icons after visible ones from tmp list
  for(srcIter = hiddenIconListTmp.begin(); srcIter != hiddenIconListTmp.end(); ++srcIter)
  {
    (*dstIter++) = (*srcIter);
  }
  LeaveCriticalSection(&trayVectorCS);
}

size_t Applet::GetVisibleIconCount()
{
  size_t visibleIconCount = 0;

  EnterCriticalSection(&trayVectorCS);
  for (size_t i = 0; i < trayIconList.size(); ++i)
  {
    if (IsIconVisible(trayIconList[i].get()))
    {
      ++visibleIconCount;
    }
  }
  LeaveCriticalSection(&trayVectorCS);

  return visibleIconCount;
}

bool Applet::IsIconVisible(TrayIcon* pTrayIcon)
{
  return (!pTrayIcon->GetHidden() &&
          ((pTrayIcon->GetFlags() & NIF_ICON) == NIF_ICON));
}

size_t Applet::GetIconCount()
{
  return trayIconList.size();
}

