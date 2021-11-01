#pragma once

#include <string>
#include <windows.h>

class ServerInfo {
  static const GUID CLSID_ExtZ_InProc_STA;
  static const wchar_t kFriendlyName_InProc_STA[];

  wchar_t mModulePath[MAX_PATH];
  std::wstring mClsId;

public:
  ServerInfo(HMODULE module);
  ~ServerInfo() = default;

  bool Register_STA();
  bool Unregister();

  bool EnableContextMenu(bool trueToDisable = false);

  HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) const;
};
