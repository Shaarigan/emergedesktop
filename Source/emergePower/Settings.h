/*!
  @file Settings.h
  @brief header for emergePower
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

#ifndef __GUARD_08f7bcb4_a329_4104_ae52_20d7bcb224c2
#define __GUARD_08f7bcb4_a329_4104_ae52_20d7bcb224c2

#define UNICODE 1

#include "../emergeBaseClasses/BaseSettings.h"

class Settings: public BaseSettings
{
public:
  Settings();
  void SetFont(LOGFONT* logFont);
  void SetUpdateInterval(int ival);
  LOGFONT* GetFont();
  int GetUpdateInterval();
  std::wstring GetHorizontalAlign();
  bool SetHorizontalAlign(std::wstring horizontalAlign);
  std::wstring GetVerticalAlign();
  bool SetVerticalAlign(std::wstring horizontalAlign);

protected:
  virtual void DoReadSettings(IOHelper& helper);
  virtual void DoWriteSettings(IOHelper& helper);
  virtual void DoInitialize();
  virtual void ResetDefaults();

private:
  int updateInterval;
  std::wstring fontString, textHorizontalAlign, textVerticalAlign;
  LOGFONT logFont;
};

#endif
