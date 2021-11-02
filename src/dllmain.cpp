#include "regutils.h"
#include "serverinfo.h"
#include "shared.h"
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
  return ::IsEqualCLSID(rclsid, kCLSID_ExtZ_InProc_STA)
             ? gSI->GetClassObject(riid, ppv)
             : CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() { return S_OK; }

const wchar_t kFriendlyName_InProc_STA[] = L"Z-InProc-STA";

STDAPI DllRegisterServer() {
  std::wstring clsId = RegUtil::GuidToString(kCLSID_ExtZ_InProc_STA);
  return gSI->RegisterInprocServer(clsId.c_str(), kFriendlyName_InProc_STA,
                                   L"Apartment")
             ? S_OK
             : E_ACCESSDENIED;
}

STDAPI DllUnregisterServer() {
  std::wstring clsId = RegUtil::GuidToString(kCLSID_ExtZ_InProc_STA);
  return gSI->UnregisterServer(clsId.c_str()) ? S_OK : E_ACCESSDENIED;
}
