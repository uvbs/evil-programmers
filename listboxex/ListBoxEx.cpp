/*
    ListBoxEx.cpp
    Copyright (C) 2004 zg

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include "ListBoxEx.hpp"

static void NotImplemented(void)
{
  Beep(100,100);
}

static long upper_key(long Key)
{
  long result=Key;
  if(FSF.LIsAlpha((unsigned long)Key)) result=FSF.LUpper((unsigned long)Key);
  return result;
}

static long get_height(HANDLE hDlg,int Index)
{
  long result=0;
#ifdef UNICODE
  FarDialogItem* DialogItem;
  DialogItem=(FarDialogItem*)(LONG_PTR)Info.SendDlgMessage(hDlg,DM_GETDLGITEM,Index,0);
#else
  FarDialogItem DialogItem_; FarDialogItem* DialogItem=&DialogItem_;
  Info.SendDlgMessage(hDlg,DM_GETDLGITEM,Index,(LONG_PTR)DialogItem);
#endif
  if(DialogItem)
  {
    result=DialogItem->Y2-DialogItem->Y1+1;
#ifdef UNICODE
    Info.SendDlgMessage(hDlg,DM_FREEDLGITEM,0,(LONG_PTR)DialogItem);
#endif
  }
  return result;
}

/*
 * ������� *current_item �� delta ����権. �᫨ ��室�� �� �࠭��� - *current_item=-1.
 */
static void get_next_item(ListBoxExData *data,long *current_item,unsigned long delta,bool direction)
{
  bool firsttime=true;
  while(*current_item<data->ItemCount&&*current_item>=0)
  {
    if(delta||!firsttime)
    {
      if(direction) (*current_item)++; else (*current_item)--;
    }
    firsttime=false;
    if(!(data->Items[*current_item].Flags&LIFEX_HIDDEN))
    {
      if(delta) delta--;
      if(!delta) break;
    }
  }
  if(*current_item<0||*current_item>=data->ItemCount) *current_item=-1;
}

static void get_nearest_item(ListBoxExData *data,long *current_item,long delta,bool direction)
{
  get_next_item(data,current_item,delta,direction);
  if(*current_item<0L&&data->ItemCount)
  {
    if(direction) *current_item=data->ItemCount-1;
    else *current_item=0;
    if(data->Items[*current_item].Flags&LIFEX_HIDDEN)
    get_next_item(data,current_item,1,!direction);
  }
}

static void normalize_curpos(ListBoxExData *data,bool direction)
{
  if(data->ItemCount)
  {
    if(data->CurPos>=data->ItemCount) data->CurPos=data->ItemCount-1;
    if(data->CurPos<0) data->CurPos=0;
    if(data->Items[data->CurPos].Flags&(LIFEX_DISABLE|LIFEX_HIDDEN))
    {
      long new_index=-1;
      for(int i=0;i<2;i++)
      {
        if(new_index!=-1) break;
        if((direction&&!i)||(!direction&&i))
        {
          for(long j=data->CurPos+1;j<data->ItemCount;j++)
          {
            if(!(data->Items[j].Flags&(LIFEX_DISABLE|LIFEX_HIDDEN)))
            {
              new_index=j;
              break;
            }
          }
        }
        else
        {
          for(int j=data->CurPos-1;j>=0;j--)
            if(!(data->Items[j].Flags&(LIFEX_DISABLE|LIFEX_HIDDEN)))
            {
              new_index=j;
              break;
            }
        }
      }
      if(new_index>-1)
      {
        data->CurPos=new_index;
      }
    }
  }
  else data->CurPos=-1;
}

static void normalize_top(ListBoxExData *data,HANDLE hDlg,int Index)
{
  if(data->ItemCount)
  {
    if(data->Top<0) data->Top=0;
    if(data->Top>=data->ItemCount) data->Top=data->ItemCount-1;
    if(data->Top>data->CurPos) data->Top=data->CurPos;
    int height=get_height(hDlg,Index)-1;
    long top_pos=data->CurPos;
    get_nearest_item(data,&top_pos,height,false);
    if(top_pos>data->CurPos) top_pos=data->CurPos;
    get_nearest_item(data,&(data->Top),0,true);
    if(data->Top<top_pos||data->Top>data->CurPos) data->Top=top_pos;
  }
  else data->Top=-1;
}

static void check_disable_or_hidden(ListBoxExData *data,HANDLE hDlg,int Index)
{
  if(data->Items[data->CurPos].Flags&(LIFEX_DISABLE|LIFEX_HIDDEN))
  {
    normalize_curpos(data,true);
    normalize_top(data,hDlg,Index);
    Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
  }
}

static void free_item(ListBoxExData *data,long Index)
{
  if(data->Items[Index].Item) HeapFree(GetProcessHeap(),0,data->Items[Index].Item);
  for(unsigned long i=0;i<sizeofa(data->Items[Index].Attribute);i++)
    if(data->Items[Index].Attribute[i]) HeapFree(GetProcessHeap(),0,data->Items[Index].Attribute[i]);
}

long WINAPI ListBoxExDialogProc(HANDLE hDlg,int Msg,int Param1,long Param2)
{
  bool return_flag=false; long return_result=FALSE,old_curpos=0L;
  const unsigned short default_colors[LISTBOXEX_COLORS_COUNT]={LISTBOXEX_COLOR_DEFAULT+LISTBOXEX_COLOR_ITEM,LISTBOXEX_COLOR_DEFAULT+LISTBOXEX_COLOR_SELECTEDITEM,LISTBOXEX_COLOR_DEFAULT+LISTBOXEX_COLOR_DISABLED};
  ListBoxExData *data=NULL;
  if(Info.SendDlgMessage(hDlg,DM_LISTBOXEX_ISLBE,Param1,0L))
  {
    data=(ListBoxExData *)Info.SendDlgMessage(hDlg,DM_GETITEMDATA,Param1,0L);
    if(data)
    {
      old_curpos=data->CurPos;
      return_flag=true;
    }
  }
  switch(Msg)
  {
    /*
     * listbox stuff
     */
    case DM_LISTBOXEX_INIT:
      if(!data)
      {
        return_flag=true;
        data=Param2?(ListBoxExData *)Param2:(ListBoxExData *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ListBoxExData));
        if(data)
        {
          if(!Param2)
          {
            data->Colors[LISTBOXEX_COLOR_BACKGROUND]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTBOX);
            data->Colors[LISTBOXEX_COLOR_ITEM]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTTEXT);
            data->Colors[LISTBOXEX_COLOR_HOTKEY]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTHIGHLIGHT);
            data->Colors[LISTBOXEX_COLOR_SELECTEDITEM]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTSELECTEDTEXT);
            data->Colors[LISTBOXEX_COLOR_SELECTEDHOTKEY]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTSELECTEDHIGHLIGHT);
            data->Colors[LISTBOXEX_COLOR_DISABLED]=Info.AdvControl(Info.ModuleNumber,ACTL_GETCOLOR,(void*)COL_DIALOGLISTDISABLED);
            data->Top=-1;
            data->CurPos=-1;
          }
          Info.SendDlgMessage(hDlg,DM_SETITEMDATA,Param1,(long)data);
          Info.SendDlgMessage(hDlg,DM_SETMOUSEEVENTNOTIFY,1,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_FREE:
      if(data)
      {
        for(long i=0;i<data->ItemCount;i++) free_item(data,i);
        if(data->Items) HeapFree(GetProcessHeap(),0,data->Items);
        HeapFree(GetProcessHeap(),0,data);
        Info.SendDlgMessage(hDlg,DM_SETITEMDATA,Param1,0L);
      }
      break;
    case DM_LISTBOXEX_INFO:
      if(data&&Param2)
      {
        NotImplemented();
      }
    case DM_LISTBOXEX_GETFLAGS:
      if(data)
      {
        return_result=data->Flags;
      }
    case DM_LISTBOXEX_SETFLAGS:
      if(data)
      {
        data->Flags=Param2;
        return_result=TRUE;
      }
      break;
    case DM_LISTBOXEX_ADD:
      if(data&&Param2)
      {
        ListBoxExAddItem *item=(ListBoxExAddItem *)Param2;
        if(item->Items)
        {
          ListBoxExItem *NewItems=(data->ItemCount)?(ListBoxExItem *)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items,(data->ItemCount+item->Count)*sizeof(ListBoxExItem)):(ListBoxExItem *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,item->Count*sizeof(ListBoxExItem));
          if(NewItems)
          {
            data->Items=NewItems;
            if(!data->ItemCount) data->Top=data->CurPos=0;
            for(long i=0;i<item->Count;i++)
            {
              ListBoxExManageItem upd_item={data->ItemCount,item->Items[i]};
              data->ItemCount++;
              Info.SendDlgMessage(hDlg,DM_LISTBOXEX_UPDATE,Param1,(long)&upd_item);
            }
            return_result=TRUE;
          }
        }
      }
      break;
    case DM_LISTBOXEX_ADDSTR:
      if(data&&Param2)
      {
        ListBoxExItem *NewItems=(data->ItemCount)?(ListBoxExItem *)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items,(data->ItemCount+1)*sizeof(ListBoxExItem)):(ListBoxExItem *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ListBoxExItem));
        if(NewItems)
        {
          data->Items=NewItems;
          data->Items[data->ItemCount].Length=_tcslen((TCHAR*)Param2);
          data->Items[data->ItemCount].Item=(TCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(data->Items[data->ItemCount].Length+1)*sizeof(TCHAR));
          if(data->Items[data->ItemCount].Item)
          {
            _tcscpy(data->Items[data->ItemCount].Item,(TCHAR*)Param2);
          }
          for(unsigned long i=0;i<sizeofa(data->Items[data->ItemCount].Attribute);i++)
          {
            data->Items[data->ItemCount].Attribute[i]=(unsigned short *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items[data->ItemCount].Length*sizeof(unsigned short));
            if(data->Items[data->ItemCount].Attribute[i])
            {
              for(unsigned long j=0;j<data->Items[data->ItemCount].Length;j++)
              {
                data->Items[data->ItemCount].Attribute[i][j]=default_colors[i];
              }
            }
          }
          if(!data->ItemCount) data->Top=data->CurPos=0;
          data->ItemCount++;
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_GETCURPOS:
      if(data)
      {
        if(Param2)
        {
          FarListPos *list=(FarListPos *)Param2;
          list->SelectPos=data->CurPos;
          list->TopPos=data->Top;
        }
        return_result=data->CurPos;
      }
      break;
    case DM_LISTBOXEX_SETCURPOS:
      if(data&&Param2)
      {
        FarListPos *list=(FarListPos *)Param2;
        data->CurPos=list->SelectPos;
        normalize_curpos(data,true);
        data->Top=list->TopPos;
        normalize_top(data,hDlg,Param1);
        Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
      }
      return_result=data->CurPos;
      break;
    case DM_LISTBOXEX_GETITEM:
      if(data&&Param2)
      {
        ListBoxExManageItem *item=(ListBoxExManageItem *)Param2;
        if(item->Index<data->ItemCount&&item->Index>=0)
        {
          item->Item=data->Items[item->Index];
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_DELETE:
      if(data&&Param2)
      {
        FarListDelete *list=(FarListDelete *)Param2;
        if(list->StartIndex<data->ItemCount&&list->StartIndex>=0)
        {
          int RealCount=list->Count; long NewPos=data->CurPos;
          if(list->StartIndex+RealCount>data->ItemCount) RealCount=data->ItemCount-list->StartIndex;
          if(NewPos>=list->StartIndex)
          {
            if(NewPos<list->StartIndex+RealCount) NewPos=list->StartIndex;
            else NewPos-=RealCount;
          }
          for(long i=0;i<RealCount;i++) free_item(data,list->StartIndex+i);
          memcpy(&(data->Items[list->StartIndex]),&(data->Items[list->StartIndex+RealCount]),(data->ItemCount-list->StartIndex-RealCount)*sizeof(data->Items[0]));
          data->ItemCount-=RealCount;
          if(data->ItemCount)
          {
            ListBoxExItem *NewItems=(ListBoxExItem *)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items,data->ItemCount*sizeof(ListBoxExItem));
            if(NewItems) data->Items=NewItems;
          }
          else
          {
            HeapFree(GetProcessHeap(),0,data->Items);
            data->Items=NULL;
          }
          data->CurPos=NewPos;
          normalize_curpos(data,true);
          normalize_top(data,hDlg,Param1);
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_UPDATE:
      if(data&&Param2)
      {
        ListBoxExManageItem *item=(ListBoxExManageItem *)Param2;
        if(item->Index<data->ItemCount)
        {
          data->Items[item->Index].Flags=item->Item.Flags;
          data->Items[item->Index].Hotkey=item->Item.Hotkey;
          data->Items[item->Index].CheckMark=item->Item.CheckMark;
          data->Items[item->Index].Length=item->Item.Length;
          if(data->Items[item->Index].Item) HeapFree(GetProcessHeap(),0,data->Items[item->Index].Item);
          data->Items[item->Index].Item=(TCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(data->Items[item->Index].Length+1)*sizeof(TCHAR));
          if(data->Items[item->Index].Item)
          {
            if(item->Item.Item)
              memcpy(data->Items[item->Index].Item,item->Item.Item,data->Items[item->Index].Length+1);
            else
            {
              memset(data->Items[item->Index].Item,' ',data->Items[item->Index].Length);
            }
          }
          for(unsigned long i=0;i<sizeofa(data->Items[item->Index].Attribute);i++)
          {
              if(data->Items[item->Index].Attribute[i]) HeapFree(GetProcessHeap(),0,data->Items[item->Index].Attribute[i]);
              data->Items[item->Index].Attribute[i]=(unsigned short *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items[item->Index].Length*sizeof(unsigned short));
              if(data->Items[item->Index].Attribute[i])
              {
                for(unsigned long j=0;j<data->Items[item->Index].Length;j++)
                {
                  if(item->Item.Attribute[i])
                    data->Items[item->Index].Attribute[i][j]=item->Item.Attribute[i][j];
                  else
                    data->Items[item->Index].Attribute[i][j]=default_colors[i];
                }
              }
          }
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_INSERT:
      if(data&&Param2)
      {
        ListBoxExManageItem *item=(ListBoxExManageItem *)Param2;
        if(item->Index<=data->ItemCount)
        {
          ListBoxExItem *NewItems=(data->ItemCount)?(ListBoxExItem *)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,data->Items,(data->ItemCount+1)*sizeof(ListBoxExItem)):(ListBoxExItem *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ListBoxExItem));
          if(NewItems)
          {
            data->Items=NewItems;
            memcpy(data->Items+item->Index+1,data->Items+item->Index,(data->ItemCount-item->Index)*sizeof(ListBoxExItem));
            memset(data->Items+item->Index,0,sizeof(ListBoxExItem));
            if(!data->ItemCount) data->Top=data->CurPos=0;
            data->ItemCount++;
            Info.SendDlgMessage(hDlg,DM_LISTBOXEX_UPDATE,Param1,Param2);
            return_result=TRUE;
          }
        }
      }
      break;
    case DM_LISTBOXEX_SETCOLORS:
      if(data&&Param2)
      {
        ListBoxExColors *colors=(ListBoxExColors *)Param2;
        long limit=(colors->Count>LISTBOXEX_COLOR_COUNT)?LISTBOXEX_COLOR_COUNT:colors->Count;
        if(colors->Colors)
        {
          for(long i=0;i<limit;i++)
            data->Colors[i]=colors->Colors[i];
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_GETDATA:
      if(data) return_result=data->UserData;
      break;
    case DM_LISTBOXEX_SETDATA:
      if(data)
      {
        return_result=data->UserData;
        data->UserData=Param2;
      }
      break;
    /*
     * items stuff
     */
    case DM_LISTBOXEX_ITEM_SETCOLOR:
      if(data&&Param2)
      {
        ListBoxExSetColor *color=(ListBoxExSetColor *)Param2;
        if(data->ItemCount>color->Index&&color->TypeIndex<LISTBOXEX_COLORS_COUNT&&data->Items[color->Index].Attribute[color->TypeIndex]&&data->Items[color->Index].Length>color->ColorIndex)
        {
          data->Items[color->Index].Attribute[color->TypeIndex][color->ColorIndex]=color->Color;
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_ITEM_SETHOTKEY:
      if(data&&Param2)
      {
        ListBoxExSetHotkey *hotkey=(ListBoxExSetHotkey *)Param2;
        if(data->ItemCount>hotkey->Index)
        {
          data->Items[hotkey->Index].Hotkey=upper_key(hotkey->Hotkey);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_ITEM_GETFLAGS:
      if(data&&Param2<data->ItemCount&&Param2>=0)
      {
        return_result=data->Items[Param2].Flags;
      }
      break;
    case DM_LISTBOXEX_ITEM_SETFLAGS:
      if(data&&Param2)
      {
        ListBoxExSetFlags *flags=(ListBoxExSetFlags *)Param2;
        if(data->ItemCount>flags->Index)
        {
          data->Items[flags->Index].Flags=flags->Flags;
          check_disable_or_hidden(data,hDlg,Param1);
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          return_result=TRUE;
        }
      }
      break;
    case DM_LISTBOXEX_ITEM_TOGGLE:
      if(data&&Param2<data->ItemCount&&Param2>=0)
      {
        if(data->Items[Param2].Flags&LIFEX_CHECKED) data->Items[Param2].Flags&=~LIFEX_CHECKED;
        else data->Items[Param2].Flags|=LIFEX_CHECKED;
        Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
        return_result=TRUE;
      }
      break;
    case DM_LISTBOXEX_ITEM_MOVE_UP:
      if(data&&Param2<data->ItemCount&&Param2>0)
      {
        ListBoxExItem temp_item=data->Items[Param2-1];
        data->Items[Param2-1]=data->Items[Param2];
        data->Items[Param2]=temp_item;
        Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
        return_result=TRUE;
      }
      break;
    case DM_LISTBOXEX_ITEM_MOVE_DOWN:
      if(data&&Param2<(data->ItemCount-1)&&Param2>=0)
      {
        ListBoxExItem temp_item=data->Items[Param2+1];
        data->Items[Param2+1]=data->Items[Param2];
        data->Items[Param2]=temp_item;
        Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
        return_result=TRUE;
      }
      break;
    case DM_LISTBOXEX_ITEM_GETDATA:
      if(data)
      {
        NotImplemented();
      }
      break;
    case DM_LISTBOXEX_ITEM_SETDATA:
      if(data)
      {
        NotImplemented();
      }
      break;
    /*
     * notification stuff
     */
    case DN_LISTBOXEX_MOUSEMOVE:
      if(data&&Param2)
      {
        COORD *coord=(COORD *)Param2;
        if(coord->Y+data->Top<data->ItemCount)
        {
          long old_CurPos=data->CurPos;
          data->CurPos=coord->Y+data->Top;
          normalize_curpos(data,old_CurPos>=data->CurPos);
          normalize_top(data,hDlg,Param1);
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
        }
      }
      break;
    /*
     * common stuff
     */
    case DN_DRAWDLGITEM:
      return_flag=false;
      if(data)
      {
#ifdef UNICODE
        FarDialogItem* DialogItem;
        DialogItem=(FarDialogItem*)(LONG_PTR)Info.SendDlgMessage(hDlg,DM_GETDLGITEM,Param1,0);
#else
        FarDialogItem DialogItem_; FarDialogItem* DialogItem=&DialogItem_;
        Info.SendDlgMessage(hDlg,DM_GETDLGITEM,Param1,(LONG_PTR)DialogItem);
#endif
        if(DialogItem)
        {
          if(DialogItem->VBuf)
          {
            int width=DialogItem->X2-DialogItem->X1+1,height=DialogItem->Y2-DialogItem->Y1+1;
            TCHAR *char_buffer=(TCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,width*sizeof(TCHAR));
            unsigned short *attr_buffer=(unsigned short *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,width*sizeof(unsigned short));
            if(char_buffer&&attr_buffer)
            {
              for(long i=0,current_item=data->Top;i<height;i++,get_next_item(data,&current_item,1,true))
              {
                for(long j=0,k=-2;j<width;j++,k++)
                {
                  if(k==data->Frozen) k+=data->CurCol;
                  if(current_item<0)
                  {
                    char_buffer[j]=' ';
                    attr_buffer[j]=data->Colors[LISTBOXEX_COLOR_BACKGROUND];
                  }
                  else
                  {
                    if(k>=0&&k<(long)data->Items[current_item].Length)
                    {
                      int color_type=LISTBOXEX_COLORS_ITEM;
                      if(current_item==data->CurPos)
                        color_type=LISTBOXEX_COLORS_SELECTED;
                      else if(data->Items[current_item].Flags&LIFEX_DISABLE)
                        color_type=LISTBOXEX_COLORS_DISABLED;
                      char_buffer[j]=data->Items[current_item].Item[k];

                      if(!data->Items[current_item].Attribute[color_type]||data->Items[current_item].Attribute[color_type][k]>=LISTBOXEX_COLOR_DEFAULT)
                      {
                        unsigned short color=data->Items[current_item].Attribute[color_type]?(data->Items[current_item].Attribute[color_type][k]-LISTBOXEX_COLOR_DEFAULT):(default_colors[color_type]-LISTBOXEX_COLOR_DEFAULT);
                        if(color>=LISTBOXEX_COLOR_COUNT) color=LISTBOXEX_COLOR_BACKGROUND;
                        attr_buffer[j]=data->Colors[color];
                      }
                      else
                        attr_buffer[j]=data->Items[current_item].Attribute[color_type][k];
                    }
                    else
                    {
                      char_buffer[j]=' ';
                      if((data->Items[current_item].Flags&LIFEX_CHECKED)&&!j)
                      {
                        char_buffer[j]=
#ifdef UNICODE
                        0x221a
#else
                        '�'
#endif
                        ;
                        if(data->Items[current_item].CheckMark) char_buffer[j]=data->Items[current_item].CheckMark;
                      }
                      if(current_item==data->CurPos)
                        attr_buffer[j]=data->Colors[LISTBOXEX_COLOR_SELECTEDITEM];
                      else if(data->Items[current_item].Flags&LIFEX_DISABLE)
                        attr_buffer[j]=data->Colors[LISTBOXEX_COLOR_DISABLED];
                      else
                        attr_buffer[j]=data->Colors[LISTBOXEX_COLOR_ITEM];
                    }
                  }
                }
                for(int j=0;j<width;j++)
                {
                  DialogItem->VBuf[i*width+j].Char.
#ifdef UNICODE
                  UnicodeChar
#else
                  AsciiChar
#endif
                  =char_buffer[j];
                  DialogItem->VBuf[i*width+j].Attributes=attr_buffer[j];
                }
              }
            }
            if(char_buffer) HeapFree(GetProcessHeap(),0,char_buffer);
            if(attr_buffer) HeapFree(GetProcessHeap(),0,attr_buffer);
          }
#ifdef UNICODE
          Info.SendDlgMessage(hDlg,DM_FREEDLGITEM,0,(LONG_PTR)DialogItem);
#endif
        }
      }
      break;
    case DN_MOUSECLICK:
      return_flag=false;
      if(data&&Param2)
      {
        MOUSE_EVENT_RECORD *mouse=(MOUSE_EVENT_RECORD *)Param2;
        if(mouse->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)
        {
          Info.SendDlgMessage(hDlg,DM_CLOSE,-1,0);
          return_flag=true;
          return_result=TRUE;
        }
      }
      break;
    case DN_KEY:
      return_flag=false;
      if(data)
      {
        bool redraw=false,hotkey=false,direction=true;
        unsigned long delta=0;
        switch(Param2)
        {
#if 1
          case KEY_LEFT:
            if(data->CurCol) data->CurCol--;
            redraw=true;
            break;
          case KEY_RIGHT:
            data->CurCol++;
            redraw=true;
            break;
#endif
          case KEY_UP:
            if((data->Flags&LBFEX_WRAPMODE)&&!data->CurPos) data->CurPos=data->ItemCount-1;
            else delta=1;
            get_nearest_item(data,&(data->CurPos),delta,false);
            direction=false;
            redraw=true;
            break;
          case KEY_DOWN:
            if((data->Flags&LBFEX_WRAPMODE)&&data->CurPos==(data->ItemCount-1)) data->CurPos=0;
            else delta=1;
            get_nearest_item(data,&(data->CurPos),delta,true);
            redraw=true;
            break;
          case KEY_HOME:
            data->CurPos=0;
            get_nearest_item(data,&(data->CurPos),delta,true);
            redraw=true;
            break;
          case KEY_END:
            data->CurPos=data->ItemCount-1;
            get_nearest_item(data,&(data->CurPos),delta,false);
            redraw=true;
            break;
          case KEY_PGUP:
            delta=get_height(hDlg,Param1);
            get_nearest_item(data,&(data->CurPos),delta,false);
            direction=false;
            redraw=true;
            break;
          case KEY_PGDN:
            delta=get_height(hDlg,Param1);
            get_nearest_item(data,&(data->CurPos),delta,true);
            redraw=true;
            break;
          default:
            {
              long FirstKey=upper_key(Param2),SecondKey=FirstKey;
              if(FSF.LIsAlpha((unsigned long)FirstKey))
              {
                unsigned char xlat_str[1]={(unsigned long)FirstKey}; //FIXME
                FSF.XLat((char*)xlat_str,0,1,NULL,0);
                SecondKey=upper_key(xlat_str[0]);
              }
              for(long i=0;i<data->ItemCount;i++)
                if((data->Items[i].Hotkey==FirstKey||data->Items[i].Hotkey==SecondKey)&&!(data->Items[i].Flags&(LIFEX_DISABLE|LIFEX_HIDDEN)))
                {
                  data->CurPos=i;
                  redraw=true;
                  hotkey=true;
                  break;
                }
            }
            break;
        }
        if(redraw)
        {
          normalize_curpos(data,direction);
          normalize_top(data,hDlg,Param1);
          Info.SendDlgMessage(hDlg,DM_REDRAW,0,0);
          if(hotkey) Info.SendDlgMessage(hDlg,DN_LISTBOXEX_HOTKEY,Param1,data->CurPos);
          return_flag=true;
          return_result=TRUE;
        }
      }
      break;
    case DN_MOUSEEVENT:
      return_flag=false;
      {
        MOUSE_EVENT_RECORD *mouse=(MOUSE_EVENT_RECORD *)Param2;
        if(mouse->dwEventFlags&MOUSE_MOVED)
        {
          SMALL_RECT rect;
          Info.SendDlgMessage(hDlg,DM_GETDLGRECT,0,(long)&rect);
          COORD relative={mouse->dwMousePosition.X-rect.Left,mouse->dwMousePosition.Y-rect.Top};
          int cur_item=0; SMALL_RECT item_rect;
          while(Info.SendDlgMessage(hDlg,DM_GETITEMPOSITION,cur_item,(long)&item_rect))
          {
            if(Info.SendDlgMessage(hDlg,DM_LISTBOXEX_ISLBE,cur_item,0L)&&relative.X>=item_rect.Left&&relative.X<=item_rect.Right&&relative.Y>=item_rect.Top&&relative.Y<=item_rect.Bottom)
            {
              COORD item_relative={relative.X-item_rect.Left,relative.Y-item_rect.Top};
              Info.SendDlgMessage(hDlg,DN_LISTBOXEX_MOUSEMOVE,cur_item,(long)&item_relative);
            }
            cur_item++;
          }
        }
      }
      break;
    default:
      return_flag=false;
      break;
  }
  if(data&&old_curpos!=data->CurPos) Info.SendDlgMessage(hDlg,DN_LISTBOXEX_POSCHANGED,Param1,data->CurPos);
  if(return_flag) return return_result;
  return Info.DefDlgProc(hDlg,Msg,Param1,Param2);
}