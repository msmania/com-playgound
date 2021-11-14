#pragma once

#include <functional>
#include <windows.h>

// {16C324E8-4B82-4648-81A0-E76E3639005E}
const GUID kCLSID_ExtZ_InProc_STA = {
    0x16c324e8,
    0x4b82,
    0x4648,
    {0x81, 0xa0, 0xe7, 0x6e, 0x36, 0x39, 0x0, 0x5e}};

// {766F63F7-E338-4CC4-99C3-19428426E912}
const GUID kCLSID_ExtZ_InProc_STA_Legacy = {
    0x766f63f7,
    0xe338,
    0x4cc4,
    {0x99, 0xc3, 0x19, 0x42, 0x84, 0x26, 0xe9, 0x12}};

// {8C88319B-6BE3-4D7C-8101-93E50DAF96AE}
const GUID kCLSID_ExtZ_OutProc_STA_1 = {
    0x8c88319b, 0x6be3, 0x4d7c, {0x81, 0x1, 0x93, 0xe5, 0xd, 0xaf, 0x96, 0xae}};

// {51A5A35B-9266-4D80-9CB6-FD345FF5A0CC}
const GUID kCLSID_ExtZ_OutProc_STA_2 = {
    0x51a5a35b,
    0x9266,
    0x4d80,
    {0x9c, 0xb6, 0xfd, 0x34, 0x5f, 0xf5, 0xa0, 0xcc}};

template <DWORD CoInit, typename Runnable = std::function<void()>>
void ComThread(Runnable&& func) {
  HRESULT hr = ::CoInitializeEx(nullptr, CoInit);
  if (FAILED(hr)) {
    Log(L"CoInitializeEx failed - %08lx\n", hr);
    return;
  }
  func();
  ::CoUninitialize();
}

void ThreadMsgWaitForSingleObject(HANDLE handle, DWORD dwMilliseconds);
