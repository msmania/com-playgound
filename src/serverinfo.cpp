#include "serverinfo.h"
#include "regutils.h"
#include <atlbase.h>

const wchar_t kUserClassRoot[] = L"Software\\Classes\\";
const wchar_t kDirClsId[] = L"CLSID\\";
const wchar_t kContextMenuHandlers[] = L".zzz\\shellex\\ContextMenuHandlers\\";
IUnknown *CreateFactory();

// {16C324E8-4B82-4648-81A0-E76E3639005E}
const GUID ServerInfo::CLSID_ExtZ_InProc_STA = {
    0x16c324e8,
    0x4b82,
    0x4648,
    {0x81, 0xa0, 0xe7, 0x6e, 0x36, 0x39, 0x0, 0x5e}};

const wchar_t ServerInfo::kFriendlyName_InProc_STA[] = L"Z-InProc-STA";

ServerInfo::ServerInfo(HMODULE module)
    : mClsId(RegUtil::GuidToString(CLSID_ExtZ_InProc_STA)) {
  if (!::GetModuleFileNameW(module, mModulePath, ARRAYSIZE(mModulePath))) {
    mModulePath[0] = 0;
  }
}

bool ServerInfo::Register_STA() {
  RegUtil root(HKEY_CURRENT_USER, kUserClassRoot);
  if (!root) {
    return false;
  }

  std::wstring subkey(kDirClsId);
  subkey += mClsId;

  RegUtil server_test(root, subkey.c_str());
  if (server_test) {
    // Key already exist.  No override.
    return false;
  }

  RegUtil server(root, subkey.c_str(), /*createIfNotExist*/ true);
  if (!server || !server.SetString(nullptr, kFriendlyName_InProc_STA)) {
    Unregister();
    return false;
  }

  RegUtil inproc(server, L"InprocServer32", /*createIfNotExist*/ true);
  if (!inproc || !inproc.SetString(nullptr, mModulePath) ||
      !inproc.SetString(L"ThreadingModel", L"Apartment")) {
    Unregister();
    return false;
  }

  return true;
}

bool ServerInfo::Unregister() {
  std::wstring subkey(kUserClassRoot);
  subkey += kDirClsId;
  subkey += mClsId;

  LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
  if (ls != ERROR_SUCCESS && ls != ERROR_FILE_NOT_FOUND) {
    return false;
  }

  return true;
}

bool ServerInfo::EnableContextMenu(bool trueToDisable) {
  std::wstring subkey(kUserClassRoot);
  subkey += kContextMenuHandlers;
  subkey += mClsId;

  if (trueToDisable) {
    LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
    return ls == ERROR_SUCCESS || ls == ERROR_FILE_NOT_FOUND;
  }

  RegUtil handlers(HKEY_CURRENT_USER, subkey.c_str(),
                   /*createIfNotExist*/ true);
  if (!handlers || !handlers.SetString(nullptr, mClsId)) {
    return false;
  }

  return true;
}

HRESULT ServerInfo::GetClassObject(REFCLSID rclsid, REFIID riid,
                                   void **ppv) const {
  *ppv = nullptr;

  if (!::IsEqualCLSID(rclsid, CLSID_ExtZ_InProc_STA) ||
      !::IsEqualCLSID(riid, IID_IClassFactory)) {
    return CLASS_E_CLASSNOTAVAILABLE;
  }

  CComPtr<IUnknown> factory;
  factory.Attach(CreateFactory());
  return factory ? factory->QueryInterface(riid, ppv) : E_OUTOFMEMORY;
}
