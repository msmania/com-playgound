#include "regutils.h"
#include <memory>
#include <strsafe.h>

void Log(const wchar_t *format, ...);

std::wstring RegUtil::GuidToString(const GUID &guid) {
  wchar_t guidStr[64];
  HRESULT hr = ::StringCbPrintfW(
      guidStr, sizeof(guidStr),
      L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", guid.Data1,
      guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
      guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
      guid.Data4[7]);
  if (FAILED(hr)) {
    Log(L"StringCbPrintfW failed - %08lx\n", hr);
    return L"";
  }
  return guidStr;
}

bool RegUtil::SetStringInternal(LPCWSTR valueName, LPCWSTR valueData,
                                DWORD valueDataLength) const {
  if (!mKey) {
    return false;
  }

  LSTATUS ls = ::RegSetValueExW(mKey, valueName,
                                /*Reserved*/ 0, REG_SZ,
                                reinterpret_cast<const BYTE *>(valueData),
                                valueDataLength);
  if (ls != ERROR_SUCCESS) {
    Log(L"RegSetValueExW failed - %08x\n", ls);
    return false;
  }
  return true;
}

RegUtil::RegUtil() : mKey(nullptr) {}

RegUtil::RegUtil(HKEY root, LPCWSTR subkey, bool createIfNotExist)
    : mKey(nullptr) {
  if (createIfNotExist) {
    DWORD dispo;
    LSTATUS ls =
        ::RegCreateKeyExW(root, subkey ? subkey : L"",
                          /*Reserved*/ 0,
                          /*lpClass*/ nullptr,
                          /*dwOptions*/ 0, KEY_ALL_ACCESS,
                          /*lpSecurityAttributes*/ nullptr, &mKey, &dispo);
    if (ls != ERROR_SUCCESS) {
      Log(L"RegCreateKeyExW failed - %08lx\n", ls);
      return;
    }
  } else {
    LSTATUS ls = ::RegOpenKeyExW(root, subkey,
                                 /*ulOptions*/ 0, KEY_ALL_ACCESS, &mKey);
    if (ls != ERROR_FILE_NOT_FOUND && ls != ERROR_SUCCESS) {
      Log(L"RegOpenKeyExW failed - %08lx\n", ls);
      return;
    }
  }
}

RegUtil::RegUtil(RegUtil &&other) : mKey(other.mKey) { other.mKey = nullptr; }

RegUtil &RegUtil::operator=(RegUtil &&other) {
  if (this != &other) {
    mKey = other.mKey;
    other.mKey = nullptr;
  }
  return *this;
}

RegUtil::~RegUtil() {
  if (!mKey) {
    return;
  }

  LSTATUS ls = ::RegCloseKey(mKey);
  if (ls != ERROR_SUCCESS) {
    Log(L"RegCloseKey failed - %08lx\n", ls);
  }
}

std::wstring RegUtil::GetString(LPCWSTR valueName) const {
  DWORD type;
  for (DWORD len = 1;; len *= 2) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[len]);
    LSTATUS status = ::RegGetValueW(mKey, nullptr, valueName, RRF_RT_REG_SZ,
                                    &type, buf.get(), &len);
    if (status == ERROR_SUCCESS) {
      return std::wstring(reinterpret_cast<wchar_t *>(buf.get()));
    } else if (status == ERROR_FILE_NOT_FOUND) {
      return L"";
    } else if (status != ERROR_MORE_DATA) {
      Log(L"RegGetValueW failed - %08x\n", status);
      return L"";
    }
  }
}

bool RegUtil::SetString(LPCWSTR valueName, LPCWSTR valueData) const {
  return SetStringInternal(
      valueName, valueData,
      valueData ? static_cast<DWORD>((wcslen(valueData) + 1) * sizeof(wchar_t))
                : 0);
}

bool RegUtil::SetString(LPCWSTR valueName,
                        const std::wstring &valueData) const {
  return SetStringInternal(
      valueName, valueData.c_str(),
      static_cast<DWORD>((valueData.size() + 1) * sizeof(wchar_t)));
}
