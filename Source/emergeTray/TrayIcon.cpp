// vim: tags+=../emergeLib/tags
//---
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
//---

#include "Balloon.h"
#include "TrayIcon.h"

//-----
// Function:	Constructor
// Requires:	HINSTANCE appInstance - application instance
// 		HWND wnd - window handle of icon
// 		HICON icon - icon handle of icon
// 		UINT id - icon id
// 		UINT callbackMessage - icon callback message
// 		WCHAR *tip - icon tooltip
// 		UINT flags - icon flags
// 		bool hidden - hidden state of the icon
// Returns:	Nothing
// Purpose:	Creates TrayIcon Class Object
//-----
TrayIcon::TrayIcon(HINSTANCE appInstance, HWND wnd, UINT id, HWND mainWnd, Settings* pSettings)
{
  this->appInstance = appInstance;
  this->wnd = wnd;
  this->id = id;
  iconVersion = 0;
  this->mainWnd = mainWnd;
  origIcon = NULL;
  newIcon = NULL;
  origIconSource = NULL;
  callbackMessage = 0;
  wcscpy(tip, TEXT("\0"));
  flags = 0;
  rect.left = 0;
  rect.right = 0;
  rect.top = 0;
  rect.bottom = 0;
  hidden = false;
  shared = false;

  convertIcon = true;

  pBalloon = std::tr1::shared_ptr<Balloon>(new Balloon(appInstance, this, pSettings));
  pBalloon->Initialize();

  lbuttonDown = false;
}

void TrayIcon::CreateNewIcon(BYTE foregroundAlpha, BYTE backgroundAlpha)
{
  HICON tmpIcon = NULL;

  /**< Don't bother converting NULL icons, just set newIcon and return */
  if (origIcon == NULL)
  {
    newIcon = NULL;
    return;
  }

  if (convertIcon)
  {
    convertIcon = false;

    // If the background if fully opaque, don't bother converting the icon, simply copy it
    if (backgroundAlpha == 0xff)
    {
      if (newIcon != NULL)
      {
        DestroyIcon(newIcon);
      }
      newIcon = CopyIcon(origIcon);
    }
    else
    {
      /**< Don't bail if EGConvertIcon returns a NULL icon, since in this case it may be valid (icon flashing) */
      tmpIcon = EGConvertIcon(origIcon, foregroundAlpha);
      if (newIcon != NULL)
      {
        DestroyIcon(newIcon);
      }
      newIcon = CopyIcon(tmpIcon);
      DestroyIcon(tmpIcon);
    }
  }
}

//-----
// Function:	Deconstructor
// Requires:	Nothing
// Returns:	Nothing
// Purpose:	Deletes the icon
//-----
TrayIcon::~TrayIcon()
{
  DestroyIcon(origIcon);
  DestroyIcon(newIcon);
}

//-----
// Function:	GetWnd
// Requires:	Nothing
// Returns:	HWND
// Purpose:	Retrieves the icon window
//-----
HWND TrayIcon::GetWnd()
{
  return wnd;
}

//-----
// Function:	GetIcon
// Requires:	Nothing
// Returns:	HICON
// Purpose:	Retrieves the icon
//-----
HICON TrayIcon::GetIcon()
{
  return newIcon;
}

//-----
// Function:	GetID
// Requires:	Nothing
// Returns:	UINT
// Purpose:	Retrieves icon ID
//-----
UINT TrayIcon::GetID()
{
  return id;
}

UINT TrayIcon::GetIconVersion()
{
  return iconVersion;
}

//-----
// Function:	GetCallback
// Requires:	Nothing
// Returns:	UINT
// Purpose:	Retrieves the icon callback message
//-----
UINT TrayIcon::GetCallback()
{
  return callbackMessage;
}

//-----
// Function:	GetTip
// Requires:	Nothing
// Returns:	WCHAR*
// Purpose:	Retrieves the icon's tooltip
//-----
WCHAR* TrayIcon::GetTip()
{
  return tip;
}

//-----
// Function:	GetFlags
// Requires:	Nothing
// Returns:	UINT
// Purpose:	Retrieves the icon flags
//-----
UINT TrayIcon::GetFlags()
{
  return flags;
}

//-----
// Function:	GetRect
// Requires:	Nothing
// Returns:	RECT*
// Purpose:	Retrieves the bounding rectangle of the icon
//-----
RECT* TrayIcon::GetRect()
{
  return &rect;
}

//-----
// Function:	GetHidden
// Requires:	Nothing
// Returns:	bool
// Purpose:	Retrieves hidden state of the icon
//-----
bool TrayIcon::GetHidden()
{
  return hidden;
}

//-----
// Function:	GetShared
// Requires:	Nothing
// Returns:	bool
// Purpose:	Retrieves shared state of the icon
//-----
bool TrayIcon::GetShared()
{
  return shared;
}

void TrayIcon::SetIconVersion(UINT iconVersion)
{
  (*this).iconVersion = iconVersion;
  pBalloon->SetIconVersion(iconVersion);
}

//-----
// Function:	SetIcon
// Requires:	HICON icon - new icon
// Returns:	true if icon was updated, false otherwise
// Purpose:	Replaces existing icon with new icon
//-----
bool TrayIcon::SetIcon(HICON icon)
{
  if (origIconSource != icon)
  {
    if (origIcon)
    {
      DestroyIcon(origIcon);
    }

    origIconSource = icon;
    origIcon = CopyIcon(icon);
    convertIcon = true;

    // changed
    return true;
  }

  return false;
}

//-----
// Function:	SetCallback
// Requires:	UINT callbackMessage
// Returns:	true if callback was updated, false otherwise
// Purpose:	Replaces the existing callback message with the new one
//-----
bool TrayIcon::SetCallback(UINT callbackMessage)
{
  if ((*this).callbackMessage != callbackMessage)
  {
    (*this).callbackMessage = callbackMessage;
    pBalloon->SetCallbackMessage(callbackMessage);

    return true;
  }

  return false;
}

//-----
// Function:	SetTip
// Requires:	WCHAR *tip - new tooltip text
// Returns:	true if tooltip was updated, false otherwise
// Purpose:	Replaces the existing tooltip text
//-----
bool TrayIcon::SetTip(WCHAR* tip)
{
  if (wcscmp((*this).tip, tip) != 0)
  {
    wcscpy((*this).tip, tip);

    // changed
    return true;
  }

  return false;
}

// Function:	SetInfo
// Requires:	WCHAR *info - new info text
// Returns:	true if info was updated, false otherwise
// Purpose:	Replaces the existing info text
//-----
void TrayIcon::ShowBalloon(WCHAR* infoTitle, WCHAR* info, DWORD infoFlags, HICON icon)
{
  POINT balloonPt;
  balloonPt.x = rect.left;
  balloonPt.y = rect.top;

  if (ClientToScreen(mainWnd, &balloonPt))
  {
    if (pBalloon->SetInfoFlags(infoFlags, icon))
    {
      pBalloon->SetInfoTitle(infoTitle);
      pBalloon->SetInfo(info);
      if (wcslen(info))
      {
        pBalloon->Show(balloonPt);
      }
      else
      {
        pBalloon->Hide();
      }
    }
  }
}

void TrayIcon::HideBalloon()
{
  pBalloon->Hide();
}

BOOL TrayIcon::SendMessage(UINT message)
{
  WPARAM wParam = WPARAM(id);
  LPARAM lParam = LPARAM(message);

  // Only send an WM_LBUTTONUP message if there has been a previous
  // WM_LBUTTONDOWN or WM_LBUTTONDBLCLK message sent
  if (lParam == WM_LBUTTONUP)
  {
    if (!lbuttonDown)
    {
      return TRUE;
    }
    else
    {
      lbuttonDown = false;
    }
  }

  // Set lbuttonDown based on WM_LBUTTONDOWN or WM_LBUTTONDBLCLK
  if ((lParam == WM_LBUTTONDOWN) || (lParam == WM_LBUTTONDBLCLK))
  {
    lbuttonDown = true;
  }

  if (iconVersion == NOTIFYICON_VERSION_4)
  {
    POINT messagePt;

    messagePt.x = rect.left;
    messagePt.y = rect.top;
    ClientToScreen(mainWnd, &messagePt);

    wParam = MAKEWPARAM(messagePt.x, messagePt.y);
    lParam = MAKELPARAM(lParam, id);
  }

  return SendNotifyMessage(wnd, callbackMessage, wParam, lParam);
}

//-----
// Function:	SetFlags
// Requires:	UINT flags - new icon flags
// Returns:	Nothing
// Purpose:	Replaces existing icon flags
//-----
void TrayIcon::SetFlags(UINT flags)
{
  (*this).flags = flags;
}

//-----
// Function:	SetRect
// Requires:	RECT rect - new bounding rectangle
// Returns:	Nothing
// Purpose:	Replaces the bounding rectangle of the icon
//-----
void TrayIcon::SetRect(RECT rect)
{
  if (!EqualRect(&(*this).rect, &rect))
  {
    (*this).rect = rect;
    pBalloon->SetIconRect(rect);
    convertIcon = true;
  }
}

//-----
// Function:	SetHidden
// Requires:	bool hidden - the hidden state of the icon
// Returns:	Nothing
// Purpose:	Replaces the hidden state of the icon
//-----
void TrayIcon::SetHidden(bool hidden)
{
  (*this).hidden = hidden;
}

//-----
// Function:	SetShared
// Requires:	bool shared - the shared state of the icon
// Returns:	Nothing
// Purpose:	Replaces the shared state of the icon
//-----
void TrayIcon::SetShared(bool shared)
{
  (*this).shared = shared;
}

void TrayIcon::UpdateIcon()
{
  convertIcon = true;
}

void TrayIcon::DeleteBalloon()
{
  pBalloon.reset();
}

