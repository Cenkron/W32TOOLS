#include "listports.h"
#include <tchar.h>
#include <stdio.h>
#include <conio.h>

static BOOL CALLBACK callback(LPVOID lpCallbackValue,LISTPORTS_PORTINFO* lpPortInfo)
{
  _tprintf(
    TEXT("\"%s\" \"%s\" \"%s\"\n"),
    lpPortInfo->lpPortName,lpPortInfo->lpTechnology,lpPortInfo->lpFriendlyName);
  return TRUE;
}

#ifdef UNICODE
int wmain(void)
#else
int main(void)
#endif
{
  ListPorts(callback,NULL);
  _tprintf(TEXT("Any key to finish\n"));
  _getch();
  return 0;
}