//---
//
//  This file is part of Emerge Desktop.
//  Copyright (C) 2004-2012  The Emerge Desktop Development Team
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
//---

#pragma once

#include "../emergeLib/emergeLib.h"
#include "../emergeGraphics/emergeGraphics.h"
#include "CustomDropTarget.h"

class MenuItem
{
public:
  MenuItem(MENUITEMDATA menuItemData, HMENU menu);
  ~MenuItem();
  UINT GetType();
  WCHAR *GetName();
  WCHAR *GetValue();
  WCHAR *GetWorkingDir();
  HMENU GetMenu();
  TiXmlElement *GetElement();
  HICON GetIcon();
  IDropTarget *GetDropTarget();
  IDropSource *GetDropSource();
  void SetIcon();
  void SetValue(WCHAR *value);
  void SetName(WCHAR *name);
  void SetElement(TiXmlElement *element);
  UINT_PTR GetID();

private:
  MENUITEMDATA menuItemData;
  HICON icon;
  IDropTarget *dropTarget;
  HMENU menu;
};
