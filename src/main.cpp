#include "interfaces.h"
#include "shared.h"
#include "gtest/gtest.h"
#include <atlbase.h>
#include <cstdarg>
#include <thread>
#include <vector>

void Log(const wchar_t *format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

void TestTestNumbers(IMarshalable *raw) {
  CComPtr<IMarshalable> comobj(raw);
  long a = 10;
  long b = 11;
  int c = 12;
  unsigned long d = 13;
  unsigned int e = 14;
  ASSERT_EQ(comobj->TestNumbers(a, &b, &c, &d, &e), S_OK);
  EXPECT_EQ(c, 42);
  EXPECT_EQ(d, 43lu);
  EXPECT_EQ(e, 44u);
  if (b != 11) {
    Log(L"Input param was changed to %ld\n", b);
  }
}

void TestObject(const std::vector<GUID> &clsIds) {
  CComPtr<IMarshalable> comobj;
  for (const auto &clsId : clsIds) {
    ASSERT_EQ(
        comobj.CoCreateInstance(clsId,
                                /*pUnkOuter*/ nullptr,
                                CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER),
        S_OK);
    TestTestNumbers(comobj);
  }
}

TEST(STA, DefaultLegacy) {
  std::thread t(ComThread<COINIT_MULTITHREADED>, []() {
    Log(L"The next object is created in the default legacy STA.\n");
    TestObject({kCLSID_ExtZ_InProc_STA});

    Log(L"The next object is created in a new STA.\n");
    std::thread sta1(ComThread<COINIT_APARTMENTTHREADED>,
                     []() { TestObject({kCLSID_ExtZ_InProc_STA}); });
    sta1.join();

    Log(L"The first of the next two objects is created in the legacy STA.\n"
        L"The second is created in a new STA.\n");
    std::thread sta2(ComThread<COINIT_APARTMENTTHREADED>, []() {
      TestObject({kCLSID_ExtZ_InProc_STA_Legacy, kCLSID_ExtZ_InProc_STA});
    });
    sta2.join();

    Log(L"The next two objects are created in the default STA.\n");
    std::thread mta(ComThread<COINIT_MULTITHREADED>, []() {
      TestObject({kCLSID_ExtZ_InProc_STA, kCLSID_ExtZ_InProc_STA_Legacy});
    });
    mta.join();
  });
  t.join();
}

TEST(STA, DefaultNonLegacy) {
  std::thread t(ComThread<COINIT_APARTMENTTHREADED>, []() {
    TestObject({kCLSID_ExtZ_InProc_STA});
    Log(L"[%04x] The thread has entered a legacy STA.\n",
        ::GetCurrentThreadId());

    Log(L"The next object is created in the default STA.\n");
    std::thread mta(ComThread<COINIT_MULTITHREADED>,
                    []() { TestObject({kCLSID_ExtZ_InProc_STA}); });
    ThreadMsgWaitForSingleObject(mta.native_handle(), INFINITE);
    mta.join();

    Log(L"The first of the next two objects is created in the legacy STA.\n"
        L"The second is created in a new STA.\n");
    std::thread sta1(ComThread<COINIT_APARTMENTTHREADED>, []() {
      TestObject({kCLSID_ExtZ_InProc_STA_Legacy, kCLSID_ExtZ_InProc_STA});
    });
    ThreadMsgWaitForSingleObject(sta1.native_handle(), INFINITE);
    sta1.join();
  });
  t.join();
}

TEST(STA, Outproc) {
  std::thread t(ComThread<COINIT_MULTITHREADED>, []() {
    TestObject({
        kCLSID_ExtZ_OutProc_STA_1,
        kCLSID_ExtZ_OutProc_STA_2,
    });
  });
  t.join();
}

TEST(STA, Dual) {
  std::thread t(ComThread<COINIT_MULTITHREADED>, []() {
    CComPtr<IUnknown> comobj;
    ASSERT_EQ(comobj.CoCreateInstance(kCLSID_ExtZ_InProc_STA,
                                      /*pUnkOuter*/ nullptr,
                                      CLSCTX_INPROC_SERVER),
              S_OK);

    long a = 10;
    long b = 11;
    int c = 12;
    unsigned long d = 13;
    unsigned int e = 14;

    CComQIPtr<IMarshalable> dual = comobj;
    ASSERT_TRUE(dual);
    EXPECT_EQ(dual->TestNumbers(a, &b, &c, &d, &e), S_OK);

    CComPtr<IMarshalable_NoDual> nodual;
    EXPECT_EQ(comobj.QueryInterface(&nodual), E_FAIL);

    CComPtr<IMarshalable_OleAuto> oleauto;
    EXPECT_EQ(comobj.QueryInterface(&oleauto), S_OK);
    d = 13;
    EXPECT_EQ(oleauto->TestNumbers_OleAuto(a, &b, &c, &d, &e), S_OK);
  });
  t.join();
}

TEST(STA, Strings) {
  std::thread t(ComThread<COINIT_APARTMENTTHREADED>, []() {
    static const wchar_t kPart[] = L"0123456789";
    static const wchar_t kOutputPart[] = L":)";
    static const wchar_t kOutput[] = L":)\u0000:)\u0000:)";
    constexpr size_t kInputLen = sizeof(kPart) * 2;
    constexpr size_t kInputCch = kInputLen / sizeof(wchar_t) - 1;
    constexpr size_t kOutputCch = sizeof(kOutput) / sizeof(wchar_t) - 1;
    std::unique_ptr<uint8_t[]> inputStr(new uint8_t[kInputLen]);
    memset(inputStr.get(), 0, kInputLen);
    memcpy(inputStr.get(), kPart, sizeof(kPart));
    memcpy(inputStr.get() + sizeof(kPart), kPart, sizeof(kPart));

    CComPtr<IMarshalable> comobj;
    ASSERT_EQ(comobj.CoCreateInstance(kCLSID_ExtZ_OutProc_STA_1,
                                      /*pUnkOuter*/ nullptr,
                                      CLSCTX_LOCAL_SERVER),
              S_OK);

    std::unique_ptr<uint8_t[]> copied1(new uint8_t[kInputLen]);
    std::unique_ptr<uint8_t[]> copied2(new uint8_t[kInputLen]);
    memcpy(copied1.get(), inputStr.get(), kInputLen);
    memcpy(copied2.get(), inputStr.get(), kInputLen);
    wchar_t *received;
    EXPECT_EQ(comobj->TestWideStrings(
                  reinterpret_cast<wchar_t *>(copied1.get()),
                  reinterpret_cast<wchar_t *>(copied2.get()), &received),
              S_OK);
    // [in]string is not modified
    EXPECT_STREQ(reinterpret_cast<const wchar_t *>(copied1.get()), kPart);
    // [in,out]string is modified with a shorter string
    EXPECT_STREQ(reinterpret_cast<const wchar_t *>(copied2.get()), kOutputPart);
    // The remaining part of [in,out]string is not modified
    EXPECT_STREQ(
        reinterpret_cast<const wchar_t *>(copied2.get() + sizeof(kOutputPart)),
        kPart + sizeof(kOutputPart) / sizeof(wchar_t));

    // The received buffer is different from the one allocated in
    // comobj's apartment.
    Log(L"Received buffer: %p\n", received);
    EXPECT_STREQ(received, kOutputPart);
    ::CoTaskMemFree(received);

    CComBSTR bstrIn(kInputCch, reinterpret_cast<wchar_t *>(inputStr.get()));
    CComBSTR bstrInOut(bstrIn);
    CComBSTR bstrOut;
    EXPECT_EQ(comobj->TestBStrings(bstrIn, &bstrOut, &bstrInOut), S_OK);

    // [in]string is not modified
    EXPECT_STREQ(bstrIn, kPart);

    // BSTR can carry bytes after a null character unlike a C string
    EXPECT_EQ(::SysStringLen(bstrInOut), kOutputCch);
    EXPECT_EQ(::SysStringLen(bstrOut), kOutputCch);
    EXPECT_EQ(memcmp(bstrInOut, kOutput, sizeof(kOutput)), 0);
    EXPECT_EQ(memcmp(bstrOut, kOutput, sizeof(kOutput)), 0);
  });
  t.join();
}

TEST(STA, ProxyTransfer) {
  std::thread t(ComThread<COINIT_MULTITHREADED>, []() {
    CComPtr<IMarshalable> outproc1, outproc2, inproc;
    ASSERT_EQ(outproc1.CoCreateInstance(kCLSID_ExtZ_OutProc_STA_1,
                                        /*pUnkOuter*/ nullptr,
                                        CLSCTX_LOCAL_SERVER),
              S_OK);
    ASSERT_EQ(outproc2.CoCreateInstance(kCLSID_ExtZ_OutProc_STA_2,
                                        /*pUnkOuter*/ nullptr,
                                        CLSCTX_LOCAL_SERVER),
              S_OK);
    ASSERT_EQ(inproc.CoCreateInstance(kCLSID_ExtZ_InProc_STA,
                                      /*pUnkOuter*/ nullptr,
                                      CLSCTX_INPROC_SERVER),
              S_OK);

    CComPtr<IMarshalable> outproc1_new, inproc_new;
    EXPECT_EQ(outproc1->DelegateCall(&outproc1_new), S_OK);
    EXPECT_EQ(inproc->DelegateCall(&inproc_new), S_OK);
    TestTestNumbers(outproc1_new);
    EXPECT_EQ(outproc2->DelegateCall(&inproc_new), S_OK);
    EXPECT_EQ(inproc->DelegateCall(&outproc1_new), S_OK);
  });
  t.join();
}
