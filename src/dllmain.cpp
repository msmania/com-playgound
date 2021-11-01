#include "serverinfo.h"
#include <memory>
#include <strsafe.h>

std::unique_ptr<ServerInfo> gSI;

void Log(const wchar_t *format, ...) {
  wchar_t linebuf[1024];
  va_list v;
  va_start(v, format);
  ::StringCbVPrintfW(linebuf, sizeof(linebuf), format, v);
  ::OutputDebugStringW(linebuf);
  va_end(v);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    gSI.reset(new ServerInfo(hModule));
    break;
  case DLL_PROCESS_DETACH:
    gSI.reset(nullptr);
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
  return gSI->GetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow() { return S_OK; }

STDAPI DllRegisterServer() {
  return gSI->Register_STA() && gSI->EnableContextMenu() ? S_OK
                                                         : E_ACCESSDENIED;
}

STDAPI DllUnregisterServer() {
  return gSI->EnableContextMenu(/*trueToDisable*/ true) && gSI->Unregister()
             ? S_OK
             : E_ACCESSDENIED;
}
