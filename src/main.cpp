#include "shared.h"
#include <atlbase.h>
#include <cstdarg>
#include <shlobj.h>
#include <thread>
#include <vector>

void Log(const wchar_t *format, ...) {
  va_list v;
  va_start(v, format);
  vwprintf(format, v);
  va_end(v);
}

void DoComThing() {
  HRESULT hr;
  CComPtr<IUnknown> comobj;
  hr = comobj.CoCreateInstance(kCLSID_ExtZ_InProc_STA, /*pUnkOuter*/ nullptr,
                               CLSCTX_INPROC_SERVER);
  if (FAILED(hr)) {
    Log(L"CComPtr::CreateInstance failed - %08lx\n", hr);
  }

  Log(L"IUnknown: %p\n", static_cast<IUnknown *>(comobj));
}

void Thread_STA() {
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    Log(L"CoInitializeEx failed - %08lx\n", hr);
    return;
  }
  DoComThing();
  ::CoUninitialize();
}

void Thread_MTA() {
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    Log(L"CoInitializeEx failed - %08lx\n", hr);
    return;
  }
  DoComThing();
  ::CoUninitialize();
}

int wmain(int argc, wchar_t *argv[]) {
  std::vector<std::thread> threads;
  threads.emplace_back(Thread_MTA);
  threads.emplace_back(Thread_MTA);
  threads.emplace_back(Thread_STA);
  for (auto &thread : threads) {
    thread.join();
  }
  return 0;
}
