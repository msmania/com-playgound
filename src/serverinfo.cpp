#include "serverinfo.h"
#include "regutils.h"
#include <atlbase.h>

const wchar_t kUserClassRoot[] = L"Software\\Classes\\";
const wchar_t kDirClsId[] = L"CLSID\\";
const wchar_t kContextMenuHandlers[] = L".zzz\\shellex\\ContextMenuHandlers\\";
IUnknown *CreateFactory();

ServerInfo::ServerInfo(HMODULE module) {
  if (!::GetModuleFileNameW(module, mModulePath, ARRAYSIZE(mModulePath))) {
    mModulePath[0] = 0;
  }
}

bool ServerInfo::RegisterInprocServer(LPCWSTR clsId, LPCWSTR friendlyName,
                                      LPCWSTR threadModel) {
  RegUtil root(HKEY_CURRENT_USER, kUserClassRoot);
  if (!root) {
    return false;
  }

  std::wstring subkey(kDirClsId);
  subkey += clsId;

  RegUtil server_test(root, subkey.c_str());
  if (server_test) {
    // Key already exist.  No override.
    return false;
  }

  RegUtil server(root, subkey.c_str(), /*createIfNotExist*/ true);
  if (!server || !server.SetString(nullptr, friendlyName)) {
    UnregisterServer(clsId);
    return false;
  }

  RegUtil inproc(server, L"InprocServer32", /*createIfNotExist*/ true);
  if (!inproc || !inproc.SetString(nullptr, mModulePath) ||
      !inproc.SetString(L"ThreadingModel", threadModel)) {
    UnregisterServer(clsId);
    return false;
  }

  return true;
}

bool ServerInfo::UnregisterServer(LPCWSTR clsId) {
  std::wstring subkey(kUserClassRoot);
  subkey += kDirClsId;
  subkey += clsId;

  LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
  if (ls != ERROR_SUCCESS && ls != ERROR_FILE_NOT_FOUND) {
    return false;
  }

  return true;
}

bool ServerInfo::EnableContextMenu(LPCWSTR clsId, bool trueToDisable) {
  std::wstring subkey(kUserClassRoot);
  subkey += kContextMenuHandlers;
  subkey += clsId;

  if (trueToDisable) {
    LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
    return ls == ERROR_SUCCESS || ls == ERROR_FILE_NOT_FOUND;
  }

  RegUtil handlers(HKEY_CURRENT_USER, subkey.c_str(),
                   /*createIfNotExist*/ true);
  if (!handlers || !handlers.SetString(nullptr, clsId)) {
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
