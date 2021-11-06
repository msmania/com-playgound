#include "serverinfo.h"
#include "interfaces.h"
#include "regutils.h"
#include <atlbase.h>

const wchar_t kUserClassRoot[] = L"Software\\Classes\\";
const wchar_t kDirClsId[] = L"CLSID\\";
const wchar_t kDirTypelib[] = L"Typelib\\";
IUnknown *CreateFactory();

void Log(const wchar_t *format, ...);

ServerInfo::ServerInfo(HMODULE module) {
  if (!::GetModuleFileNameW(module, mModulePath, ARRAYSIZE(mModulePath))) {
    mModulePath[0] = 0;
  }
}

bool ServerInfo::RegisterServer(LPCWSTR clsId, LPCWSTR friendlyName,
                                LPCWSTR threadModel) const {
  std::wstring subkey(kUserClassRoot);
  subkey += kDirClsId;
  subkey += clsId;

  RegUtil server_test(HKEY_CURRENT_USER, subkey.c_str());
  if (server_test) {
    // Key already exist.  No override.
    return false;
  }

  RegUtil server(HKEY_CURRENT_USER, subkey.c_str(), /*createIfNotExist*/ true);
  if (!server || !server.SetString(nullptr, friendlyName)) {
    UnregisterServer(clsId);
    return false;
  }

  bool ok = false;
  if (threadModel) {
    RegUtil inproc(server, L"InprocServer32", /*createIfNotExist*/ true);
    ok = inproc && inproc.SetString(nullptr, mModulePath) &&
         inproc.SetString(L"ThreadingModel", threadModel);
  } else {
    RegUtil outproc(server, L"LocalServer32", /*createIfNotExist*/ true);
    ok = outproc && outproc.SetString(nullptr, mModulePath);
  }

  if (!ok) {
    UnregisterServer(clsId);
  }
  return ok;
}

bool ServerInfo::UnregisterServer(LPCWSTR clsId) const {
  std::wstring subkey(kUserClassRoot);
  subkey += kDirClsId;
  subkey += clsId;

  LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
  if (ls != ERROR_SUCCESS && ls != ERROR_FILE_NOT_FOUND) {
    return false;
  }

  return true;
}

bool ServerInfo::RegisterTypelib() const {
  CComPtr<ITypeLib> tlb;
  HRESULT hr = ::LoadTypeLib(mModulePath, &tlb);
  if (FAILED(hr)) {
    Log(L"LoadTypeLib failed - %08lx\n", hr);
    return false;
  }

  CComBSTR copied(mModulePath);
  hr = ::RegisterTypeLibForUser(tlb, copied, nullptr);
  if (FAILED(hr)) {
    Log(L"RegisterTypeLibForUser failed - %08lx\n", hr);
    return false;
  }
  return true;
}

bool ServerInfo::UnregisterTypelib() const {
  std::wstring subkey(kUserClassRoot);
  subkey += kDirTypelib;
  subkey += RegUtil::GuidToString(LIBID_COM_Playground);
  RegUtil typelib_test(HKEY_CURRENT_USER, subkey.c_str());
  if (!typelib_test) {
    return true;
  }

#ifdef _WIN64
  constexpr SYSKIND sys = SYS_WIN64;
#else
  constexpr SYSKIND sys = SYS_WIN32;
#endif
  HRESULT hr = ::UnRegisterTypeLibForUser(LIBID_COM_Playground, 1, 0, 0, sys);
  if (FAILED(hr)) {
    Log(L"UnRegisterTypeLibForUser failed - %08lx\n", hr);
    return false;
  }
  return true;
}

HRESULT ServerInfo::GetClassObject(REFIID riid, void **ppv) const {
  *ppv = nullptr;

  CComPtr<IUnknown> factory;
  factory.Attach(CreateFactory());
  return factory ? factory->QueryInterface(riid, ppv) : E_OUTOFMEMORY;
}

bool RegisterAllServers(const ServerInfo *si,
                        const ServerRegistrationEntry servers[],
                        bool trueToUnregister) {
  constexpr GUID kEmptyGuid = {};
  bool ok = true;
  for (const ServerRegistrationEntry *server = servers;
       server->mGuid != kEmptyGuid; ++server) {
    std::wstring clsId = RegUtil::GuidToString(server->mGuid);
    if (trueToUnregister) {
      if (!si->UnregisterServer(clsId.c_str())) {
        // Continue unregistering all servers
        ok = false;
      }
      continue;
    }

    if (!si->RegisterServer(clsId.c_str(), server->mFriendlyName,
                            server->mThreadModel)) {
      // Stop registering servers
      ok = false;
      break;
    }
  }

  if (trueToUnregister) {
    if (!si->UnregisterTypelib()) {
      ok = false;
    }
  } else {
    if (!si->RegisterTypelib()) {
      ok = false;
    }
  }

  return ok;
}
