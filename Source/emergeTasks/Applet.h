/*!
  @file Applet.h
  @brief header for emergeTasks
  @author The Emerge Desktop Development Team

  @attention This file is part of Emerge Desktop.
  @attention Copyright (C) 2004-2013  The Emerge Desktop Development Team

  @attention Emerge Desktop is free software; you can redistribute it and/or
  modify  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  @attention Emerge Desktop is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  @attention You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

#ifndef __GUARD_83fd61a0_eea6_48dc_8548_6bb5551f0638
#define __GUARD_83fd61a0_eea6_48dc_8548_6bb5551f0638

#define UNICODE 1

// Define required for the Window Transparency
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

// Define required for menu animation
#undef WINVER
#define WINVER 0x0501

#define MODIFY_DELAY_TIME 200
#define CLEAN_WAIT_TIME   250

#ifdef __GNUC__
#include <tr1/memory>
#include <tr1/shared_ptr.h>
#else
#include <memory>
#endif

#include <windows.h>
#include <vector>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include "../emergeAppletEngine/emergeAppletEngine.h"
#include "../emergeBaseClasses/BaseApplet.h"
#include "../emergeLib/emergeWindowLib.h"
#include "../emergeStyleEngine/emergeStyleEngine.h"
#include "Config.h"
#include "Settings.h"
#include "Task.h"

//ROBLARKY - 2012-08-11 -	Added the following two lines for multimonitor support
//							This was added to support multiple instances of emergeTasks running on different monitors, so only the tasks on the
//							monitor where the applet resides displays the tasks on that monitor
#define COMPILE_MULTIMON_STUBS

typedef std::tr1::shared_ptr<Task> TaskPtr;
typedef std::vector<TaskPtr> TaskVector;

class Applet: public BaseApplet
{
private:
  std::tr1::shared_ptr<Settings> pSettings;
  TaskVector taskList;
  std::map<HWND, UINT> flashMap;
  std::map<HWND, DWORD> modifyMap;
  std::map<HWND, HANDLE> thumbnailMap;
  HWND activeWnd, oldTipWnd, movingWnd;
  void ResetTaskIcons();
  void UpdateIcons();
  static LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
  static BOOL CALLBACK EnumTasksList(HWND hwnd, LPARAM lParam);
  static VOID CALLBACK FlashTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

  //ROBLARKY - 2012-08-11 -	Added IsWindowOnSameMonitor
  //						This was added to support multiple instances of emergeTasks running on different monitors, so only the tasks on the
  //						monitor where the applet resides displays the tasks on that monitor
  bool IsWindowOnSameMonitor(HWND hwnd);

  CRITICAL_SECTION vectorLock, mapLock;
  LRESULT DoTaskIcon(HWND task, HICON icon);
  static DWORD WINAPI ModifyThreadProc(LPVOID lpParameter);
  static DWORD WINAPI UpdateThumbnailThreadProc(LPVOID lpParameter);
  TOOLINFO ti;
  HANDLE cleanTaskThread;
  static DWORD WINAPI CleanTaskThreadProc(LPVOID lpParameter);

protected:
  virtual std::tr1::shared_ptr<BaseSettings> createSettings();

public:
  Applet(HINSTANCE hInstance);
  ~Applet();
  UINT Initialize();
  LRESULT DoNotify(HWND hwnd, LPARAM lParam);
  LRESULT DoTimer(UINT_PTR timerID);
  LRESULT DoSizing(HWND hwnd, UINT edge, LPRECT rect);
  LRESULT DoDefault(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  LRESULT TaskMouseEvent(UINT message, LPARAM lParam);
  LRESULT AddTask(HWND task);
  LRESULT ModifyTask(HWND task);
  LRESULT RemoveTask(HWND task);
  LRESULT SetFlash(HWND task, bool flash);
  LRESULT DoCopyData(COPYDATASTRUCT* cds);
  void CreateBrushes();
  void DestroyBrushes();
  bool CleanTasks();
  void BuildTasksList();
  TaskVector::iterator FindTask(HWND hwnd);
  void AppletUpdate();
  void SetActive(HWND hwnd);
  void ShowConfig();
  bool PaintItem(HDC hdc, size_t index, int x, int y, RECT rect);
  size_t GetIconCount();
  size_t GetVisibleIconCount();
  void DoTaskFlash(UINT id);
  LRESULT ModifyTaskByThread(DWORD threadID);
  LRESULT DoSize(LPARAM lParam);
};

#endif
