#pragma once

#include <windows.h>

class ServerInfo {
  wchar_t mModulePath[MAX_PATH];

public:
  ServerInfo(HMODULE module);
  ~ServerInfo() = default;

  bool RegisterInprocServer(LPCWSTR clsId, LPCWSTR friendlyName,
                            LPCWSTR threadModel);
  bool UnregisterServer(LPCWSTR clsId);

  bool EnableContextMenu(LPCWSTR clsId, bool trueToDisable = false);

  HRESULT GetClassObject(REFIID riid, void **ppv) const;
};
