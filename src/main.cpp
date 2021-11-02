#include "shared.h"
#include <atlbase.h>
#include <cstdarg>
#include <functional>
#include <shlobj.h>
#include <thread>
#include <vector>

void Log(const wchar_t *format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

template <DWORD CoInit> void ComThread(const std::function<void()> &func) {
  HRESULT hr = ::CoInitializeEx(nullptr, CoInit);
  if (FAILED(hr)) {
    Log(L"CoInitializeEx failed - %08lx\n", hr);
    return;
  }
  func();
  ::CoUninitialize();
}

void ThreadMsgWaitForSingleObject(HANDLE handle, DWORD dwMilliseconds) {
  for (;;) {
    DWORD status = ::MsgWaitForMultipleObjectsEx(
        1, &handle, dwMilliseconds, QS_SENDMESSAGE | QS_POSTMESSAGE,
        MWMO_INPUTAVAILABLE);
    if (status == WAIT_OBJECT_0) {
      return;
    }

    if (status != WAIT_OBJECT_0 + 1) {
      Log(L"MsgWaitForMultipleObjectsEx returned - %08lx\n", status);
      return;
    }

    MSG msg;
    while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessageW(&msg);
    }
  }
}

void TestObject(const std::vector<GUID> &clsIds) {
  CComPtr<IUnknown> comobj;
  for (const auto &clsId : clsIds) {
    HRESULT hr =
        comobj.CoCreateInstance(clsId,
                                /*pUnkOuter*/ nullptr, CLSCTX_INPROC_SERVER);
    if (FAILED(hr)) {
      Log(L"CComPtr::CreateInstance failed - %08lx\n", hr);
    }
  }
}

int wmain(int argc, wchar_t *argv[]) {
  Log(L"\n# Default STA is a legacy STA\n#\n");
  std::thread t1(ComThread<COINIT_MULTITHREADED>, []() {
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
  t1.join();

  Log(L"\n# Default STA is a non-legacy STA\n#\n");
  std::thread t2(ComThread<COINIT_APARTMENTTHREADED>, []() {
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
  t2.join();

  return 0;
}
