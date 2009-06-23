/*
    Servelad plugin for FAR Manager
    Copyright (C) 2009 Alex Yaroslavsky

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "plugin.hpp"
#include <CRT/crt.hpp>
#include "ServiceManager.hpp"

struct PluginStartupInfo Info;
FARSTANDARDFUNCTIONS FSF;

const TCHAR *GetMsg(int MsgId)
{
  return(Info.GetMsg(Info.ModuleNumber,MsgId));
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *psi)
{
  Info = *psi;
  FSF = *psi->FSF;
  Info.FSF = &FSF;
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
  ServiceManager *sm = new ServiceManager();

  return (HANDLE)sm;
}

void WINAPI EXP_NAME(ClosePlugin)(HANDLE hPlugin)
{
  ServiceManager *sm = (ServiceManager *)hPlugin;

  delete sm;
}

void WINAPI GetPluginInfoW(struct PluginInfo *pi)
{
  static const wchar_t *MenuStrings[1];

  pi->StructSize = sizeof(struct PluginInfo);
  pi->Flags = 0;
  MenuStrings[0] = L"Servelad";
  pi->PluginMenuStrings = MenuStrings;
  pi->PluginMenuStringsNumber = 1;
}

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *Info)
{
  ServiceManager *sm = (ServiceManager *)hPlugin;

  Info->StructSize = sizeof(*Info);

  Info->Flags = OPIF_ADDDOTS|OPIF_USEHIGHLIGHTING|OPIF_USEFILTER;

  Info->CurDir = NULL;
  if (sm->GetType() == SERVICE_WIN32)
    Info->CurDir = L"Services";
  else if (sm->GetType() == SERVICE_DRIVER)
    Info->CurDir = L"Drivers";
}

int WINAPI GetFindDataW(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
  ServiceManager *sm = (ServiceManager *)hPlugin;

  *pPanelItem = NULL;
  *pItemsNumber = 0;

  if (sm->GetType() == 0)
  {
    *pPanelItem = (PluginPanelItem *)malloc(sizeof(PluginPanelItem)*2);
    memset(*pPanelItem, 0, sizeof(PluginPanelItem)*2);
    *pItemsNumber = 2;

    (*pPanelItem)[0].FindData.lpwszFileName = (wchar_t *)L"Drivers";
    (*pPanelItem)[0].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    (*pPanelItem)[1].FindData.lpwszFileName = (wchar_t *)L"Services";
    (*pPanelItem)[1].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
  }
  else
  {
    sm->RefreshList();

    *pPanelItem = (PluginPanelItem *)malloc(sizeof(PluginPanelItem)*sm->GetCount());
    memset(*pPanelItem, 0, sizeof(PluginPanelItem)*sm->GetCount());
    *pItemsNumber = sm->GetCount();

    for (int i = 0; i < *pItemsNumber; i++)
    {
      (*pPanelItem)[i].FindData.lpwszFileName = (wchar_t *)(*sm)[i].lpDisplayName;
      (*pPanelItem)[i].UserData = i;
    }
  }

  return TRUE;
}

void WINAPI FreeFindDataW(HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber)
{
  free(PanelItem);
}

int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
  if (OpMode&OPM_FIND)
    return FALSE;

  ServiceManager *sm = (ServiceManager *)hPlugin;

  if (!wcscmp(Dir,L"\\") || !wcscmp(Dir,L".."))
  {
    sm->Reset();
  }
  else if (!wcscmp(Dir,L"Services"))
  {
    sm->Reset(SERVICE_WIN32);
  }
  else if (!wcscmp(Dir,L"Drivers"))
  {
    sm->Reset(SERVICE_DRIVER);
  }
  else
  {
    return FALSE;
  }

  return TRUE;
}

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  (void) hDll;
  (void) dwReason;
  (void) lpReserved;
  return TRUE;
}
#endif