//----  --------------------------------------------------------------------------------------------------------
//
//  This file is part of Emerge Desktop.
//  Copyright (C) 2004-2010  The Emerge Desktop Development Team
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

#ifndef __EC_THEMESELECTOR_H
#define __EC_THEMESELECTOR_H

#undef _WIN32_IE
#define _WIN32_IE	0x501

#undef _WIN32_WINNT
#define _WIN32_WINNT	0x501

#define UNICODE 1

#include "../emergeLib/emergeLib.h"
#include <commctrl.h>
#include "resource.h"
#include "ThemeSaver.h"

class ThemeSelector
{
public:
  ThemeSelector(HINSTANCE hInstance, HWND mainWnd);
  ~ThemeSelector();
  int Show();
  BOOL DoInitDialog(HWND hwndDlg);
  BOOL DoThemeCommand(HWND hwndDlg, WPARAM wParam, LPARAM lParam);

private:
  std::tr1::shared_ptr<ThemeSaver> pThemeSaver;
  HINSTANCE hInstance;
  HWND mainWnd, toolWnd;
  HICON saveasIcon, saveIcon, delIcon;
  void PopulateThemes(HWND themeWnd, WCHAR *currentTheme);
  bool SaveTheme(HWND hwndDlg);
  void DoSaveAs(HWND hwndDlg);
  void DoDelTheme(HWND hwndDlg);
  void DoSave(HWND hwndDlg);
  void DoExport(HWND hwndDlg);
  static BOOL CALLBACK ThemeSelectorDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
  BOOL DoThemeCheck(HWND hwndDlg);
};

#endif
