#pragma once

#include <windows.h>

class ServerInfo {
  static const GUID CLSID_ExtZ_InProc_STA;
  static const wchar_t kFriendlyName_InProc_STA[];

  wchar_t mModulePath[MAX_PATH];

public:
  ServerInfo(HMODULE module);
  ~ServerInfo() = default;

  bool Register_STA();
  bool Unregister();
};
