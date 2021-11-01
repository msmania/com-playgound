#pragma once

#include <string>
#include <windows.h>

class RegUtil {
  HKEY mKey;

  bool SetStringInternal(LPCWSTR valueName, LPCWSTR valueData,
                         DWORD valueDataLength) const;

public:
  static std::wstring GuidToString(const GUID &guid);

  RegUtil();
  RegUtil(HKEY root, LPCWSTR subkey, bool createIfNotExist = false);
  ~RegUtil();

  RegUtil(const RegUtil &) = delete;
  RegUtil &operator=(const RegUtil &) = delete;
  RegUtil(RegUtil &&);
  RegUtil &operator=(RegUtil &&);

  constexpr operator bool() const { return !!mKey; }
  constexpr operator HKEY() const { return mKey; }

  std::wstring GetString(LPCWSTR valueName) const;
  bool SetString(LPCWSTR valueName, LPCWSTR valueData = nullptr) const;
  bool SetString(LPCWSTR valueName, const std::wstring &valueData) const;
};
