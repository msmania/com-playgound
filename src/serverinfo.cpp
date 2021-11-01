#include "serverinfo.h"
#include "regutils.h"

const wchar_t kUserClassRoot[] = L"Software\\Classes\\CLSID\\";

// {16C324E8-4B82-4648-81A0-E76E3639005E}
const GUID ServerInfo::CLSID_ExtZ_InProc_STA = {
    0x16c324e8,
    0x4b82,
    0x4648,
    {0x81, 0xa0, 0xe7, 0x6e, 0x36, 0x39, 0x0, 0x5e}};

const wchar_t ServerInfo::kFriendlyName_InProc_STA[] = L"Z-InProc-STA";

ServerInfo::ServerInfo(HMODULE module) {
  if (!::GetModuleFileNameW(module, mModulePath, ARRAYSIZE(mModulePath))) {
    mModulePath[0] = 0;
  }
}

bool ServerInfo::Register_STA() {
  std::wstring subkey(kUserClassRoot);
  subkey += RegUtil::GuidToString(CLSID_ExtZ_InProc_STA);

  RegUtil root_test(HKEY_CURRENT_USER, subkey.c_str());
  if (root_test) {
    // Key already exist.  No override.
    return false;
  }

  RegUtil root(HKEY_CURRENT_USER, subkey.c_str(), /*createIfNotExist*/ true);
  if (!root || !root.SetString(nullptr, kFriendlyName_InProc_STA)) {
    Unregister();
    return false;
  }

  RegUtil inproc(root, L"InprocServer32", /*createIfNotExist*/ true);
  if (!inproc || !inproc.SetString(nullptr, mModulePath) ||
      !inproc.SetString(L"ThreadingModel", L"Apartment")) {
    Unregister();
    return false;
  }

  return true;
}

bool ServerInfo::Unregister() {
  std::wstring subkey(kUserClassRoot);
  subkey += RegUtil::GuidToString(CLSID_ExtZ_InProc_STA);

  LSTATUS ls = ::RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str());
  if (ls != ERROR_SUCCESS && ls != ERROR_FILE_NOT_FOUND) {
    return false;
  }

  return true;
}
