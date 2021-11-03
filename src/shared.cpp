#include "shared.h"

void Log(const wchar_t *format, ...);

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
