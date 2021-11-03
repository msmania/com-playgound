#include "regutils.h"
#include "serverinfo.h"
#include "shared.h"
#include <atlbase.h>
#include <memory>
#include <strsafe.h>
#include <thread>
#include <vector>
#include <windows.h>

static const ServerRegistrationEntry kServers[] = {
    {kCLSID_ExtZ_OutProc_STA_1, L"Z-OutProc-STA-1", nullptr},
    {kCLSID_ExtZ_OutProc_STA_2, L"Z-OutProc-STA-2", nullptr},
    {},
};

std::unique_ptr<ServerInfo> gSI;

struct HandleCloser {
  typedef HANDLE pointer;
  void operator()(HANDLE h) {
    if (h) {
      ::CloseHandle(h);
    }
  }
};

void Log(const wchar_t *format, ...) {
  wchar_t linebuf[1024];
  va_list v;
  va_start(v, format);
  ::StringCbVPrintfW(linebuf, sizeof(linebuf), format, v);
  ::OutputDebugStringW(linebuf);
  va_end(v);
}

class ComServerClass {
  CComPtr<IUnknown> mFactory;
  DWORD mCookie;

public:
  ComServerClass(GUID clsId) : mCookie(0) {
    IUnknown *raw;
    HRESULT hr =
        gSI->GetClassObject(IID_IUnknown, reinterpret_cast<void **>(&raw));
    if (FAILED(hr)) {
      Log(L"Failed to create a factory object - %08lx\n", hr);
      return;
    }
    mFactory.Attach(raw);

    hr = ::CoRegisterClassObject(clsId, mFactory, CLSCTX_LOCAL_SERVER,
                                 REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED,
                                 &mCookie);
    if (FAILED(hr)) {
      Log(L"CoRegisterClassObject failed - %08lx\n", hr);
      mFactory.Release();
      return;
    }
  }

  ~ComServerClass() {
    if (mFactory) {
      ::CoRevokeClassObject(mCookie);
    }
  }
};

void ServerMain(HANDLE event, GUID clsId) {
  ComServerClass class_sta(clsId);

  HRESULT hr = ::CoResumeClassObjects();
  if (FAILED(hr)) {
    Log(L"CoResumeClassObjects failed - %08lx\n", hr);
    return;
  }

  ThreadMsgWaitForSingleObject(event, INFINITE);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmd, int) {
  // Prevent multiple instances of this executable
  std::unique_ptr<HANDLE, HandleCloser> event(::CreateEventW(
      /*lpEventAttributes*/ nullptr,
      /*bManualReset*/ TRUE,
      /*bInitialState*/ FALSE,
      L"COMServer-a16109f3-64af-49bc-80d7-5a7c1a837cae"));
  if (!event) {
    return 1;
  }
  if (wcscmp(cmd, L"--stop") == 0) {
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
      // Signal the running server to exit
      ::SetEvent(event.get());
    }
    return 0;
  }

  gSI.reset(new ServerInfo(inst));

  if (wcscmp(cmd, L"--register") == 0) {
    if (!RegisterAllServers(gSI.get(), kServers)) {
      RegisterAllServers(gSI.get(), kServers, /*trueToUnregister*/ true);
    }
  } else if (wcscmp(cmd, L"--unregister") == 0) {
    RegisterAllServers(gSI.get(), kServers, /*trueToUnregister*/ true);
  } else {
    std::vector<std::thread> threads;
    threads.emplace_back(ComThread<COINIT_APARTMENTTHREADED>, [&event]() {
      ServerMain(event.get(), kCLSID_ExtZ_OutProc_STA_1);
    });
    threads.emplace_back(ComThread<COINIT_APARTMENTTHREADED>, [&event]() {
      ServerMain(event.get(), kCLSID_ExtZ_OutProc_STA_2);
    });
    for (auto &thread : threads) {
      thread.join();
    }
  }

  gSI.reset(nullptr);
  return 0;
}
