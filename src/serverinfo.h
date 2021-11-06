#pragma once

#include <windows.h>

class ServerInfo {
  wchar_t mModulePath[MAX_PATH];

public:
  ServerInfo(HMODULE module);
  ~ServerInfo() = default;

  bool RegisterServer(LPCWSTR clsId, LPCWSTR friendlyName,
                      LPCWSTR threadModel) const;
  bool UnregisterServer(LPCWSTR clsId) const;

  bool RegisterTypelib() const;
  bool UnregisterTypelib() const;

  HRESULT GetClassObject(REFIID riid, void **ppv) const;
};

struct ServerRegistrationEntry {
  GUID mGuid;
  LPCWSTR mFriendlyName;
  LPCWSTR mThreadModel; // LocalServer if this is null
};

bool RegisterAllServers(const ServerInfo *si,
                        const ServerRegistrationEntry servers[],
                        bool trueToUnregister = false);
