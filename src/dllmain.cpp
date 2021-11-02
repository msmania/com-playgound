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
  return ::IsEqualCLSID(rclsid, kCLSID_ExtZ_InProc_STA) ||
                 ::IsEqualCLSID(rclsid, kCLSID_ExtZ_InProc_STA_Legacy)
             ? gSI->GetClassObject(riid, ppv)
             : CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() { return S_OK; }

static bool RegisterAllServers(bool trueToUnregister = false) {
  struct {
    GUID mGuid;
    LPCWSTR mFriendlyName;
    LPCWSTR mThreadModel;
  } static const kServers[] = {
      {kCLSID_ExtZ_InProc_STA, L"Z-InProc-STA", L"Apartment"},
      {kCLSID_ExtZ_InProc_STA_Legacy, L"Z-InProc-STA-Legacy", L"Single"},
  };

  bool ok = true;
  for (const auto &server : kServers) {
    std::wstring clsId = RegUtil::GuidToString(server.mGuid);
    if (trueToUnregister) {
      if (!gSI->UnregisterServer(clsId.c_str())) {
        // Continue unregistering all servers
        ok = false;
      }
      continue;
    }

    if (!gSI->RegisterInprocServer(clsId.c_str(), server.mFriendlyName,
                                   server.mThreadModel)) {
      // Stop registering servers
      ok = false;
      break;
    }
  }

  return ok;
}

STDAPI DllUnregisterServer() {
  return RegisterAllServers(/*trueToUnregister*/ true) ? S_OK : E_ACCESSDENIED;
}

STDAPI DllRegisterServer() {
  if (!RegisterAllServers()) {
    RegisterAllServers(/*trueToUnregister*/ true);
    return E_ACCESSDENIED;
  }
  return S_OK;
}
