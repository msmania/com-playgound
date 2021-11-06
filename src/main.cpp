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

void TestObject(const std::vector<GUID> &clsIds) {
  CComPtr<IMarshalable> comobj;
  for (const auto &clsId : clsIds) {
    ASSERT_EQ(
        comobj.CoCreateInstance(clsId,
                                /*pUnkOuter*/ nullptr,
                                CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER),
        S_OK);

    long a = 10;
    long b = 11;
    int c = 12;
    unsigned long d = 13;
    unsigned int e = 14;
    ASSERT_EQ(comobj->TestNumbers(a, &b, &c, &d, &e), S_OK);
    EXPECT_EQ(c, 42);
    EXPECT_EQ(d, 43lu);
    EXPECT_EQ(e, 44u);
    Log(L"Input params: %ld\n", b);
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
