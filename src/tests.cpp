#include "regutils.h"
#include "gtest/gtest.h"

void Log(const wchar_t *format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

TEST(RegUtils, ctor) {
  RegUtil reg;
  EXPECT_FALSE(reg);

  RegUtil reg_hkcu(HKEY_CURRENT_USER, nullptr);
  EXPECT_TRUE(reg_hkcu);

  RegUtil reg_ms(reg_hkcu, L"Software\\Microsoft");
  EXPECT_TRUE(reg_ms);

  RegUtil reg_nonexistent(reg_hkcu, L"-_-\\-_-");
  EXPECT_FALSE(reg_nonexistent);

  // If the process runs in WOW64, registry virtualizaion allows
  // the code below pass.  So compile it only for 64bit.
  // https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-virtualization
#if _WIN64
  RegUtil reg_hklm_access_denied(HKEY_LOCAL_MACHINE, L"Software");
  EXPECT_FALSE(reg_hklm_access_denied);
#endif
}

TEST(RegUtils, key) {
  const wchar_t kTestGuid[] = L"{709DF953-5BFF-450B-8116-85F8333F7C5E}";
  const wchar_t kJPN[] = L"\u4ECA\u65E5\u306F\u3044\u3044\u5929\u6C17";
  const wchar_t kRUS[] = L"\u041F\u0440\u0438\u0432\u0435\u0442";
  const wchar_t kChildKey[] = L"\u092E\u0941\u0930\u094D\u0917\u093E";

  std::wstring subkey(L"Software\\");
  subkey += kTestGuid;

  RegUtil reg_test_read_only(HKEY_CURRENT_USER, subkey.c_str());
  ASSERT_FALSE(reg_test_read_only);

  RegUtil reg_test(HKEY_CURRENT_USER, subkey.c_str(),
                   /*createIfNotExist*/ true);
  EXPECT_TRUE(reg_test);

  RegUtil reg_test_child(reg_test, kChildKey, /*createIfNotExist*/ true);
  EXPECT_TRUE(reg_test_child);

  // Value not set at the beginning
  std::wstring data = reg_test.GetString(nullptr);
  EXPECT_TRUE(data.empty());

  // Set an empty string and verify it
  EXPECT_TRUE(reg_test.SetString(nullptr, nullptr));
  data = reg_test.GetString(nullptr);
  EXPECT_TRUE(data.empty());

  EXPECT_TRUE(reg_test.SetString(nullptr, kJPN));
  data = reg_test.GetString(nullptr);
  EXPECT_STREQ(data.c_str(), kJPN);

  EXPECT_TRUE(reg_test.SetString(kRUS, data));
  data = reg_test.GetString(kRUS);
  EXPECT_STREQ(data.c_str(), kJPN);

  RegUtil reg_software(HKEY_CURRENT_USER, L"software");

  std::wstring childkey(kTestGuid);
  childkey += L'\\';
  childkey += kChildKey;
  EXPECT_EQ(::RegDeleteTreeW(reg_software, childkey.c_str()), ERROR_SUCCESS);

  EXPECT_EQ(::RegDeleteTreeW(reg_software, kTestGuid), ERROR_SUCCESS);
}

TEST(RegUtils, guid) {
  struct {
    GUID mGuid;
    LPCWSTR mExpected;
  } static const kTestCases[] = {
      {{}, L"{00000000-0000-0000-0000-000000000000}"},
      {{0x766f63f7,
        0xe338,
        0x4cc4,
        {0x99, 0xc3, 0x19, 0x42, 0x84, 0x26, 0xe9, 0x12}},
       L"{766f63f7-e338-4cc4-99c3-19428426e912}"},
  };

  for (const auto &testCase : kTestCases) {
    std::wstring actual = RegUtil::GuidToString(testCase.mGuid);
    EXPECT_STREQ(actual.c_str(), testCase.mExpected);
  }
}
