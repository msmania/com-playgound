#include "regutils.h"
#include <memory>
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

void Log(const wchar_t *format, ...);

class ContextMenuExt : public IShellExtInit, public IContextMenu {
  ULONG mRef;

public:
  ContextMenuExt();
  virtual ~ContextMenuExt() = default;

  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IShellExtInit
  STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj,
                          HKEY hkeyProgID);

  // IContextMenu
  STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                                UINT idCmdLast, UINT uFlags);

  STDMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO *pici);

  STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved,
                                CHAR *pszName, UINT cchMax);
};

ContextMenuExt::ContextMenuExt() : mRef(1) {
  Log(L"[%04x] ContextMenuExt: %p\n", ::GetCurrentThreadId(), this);
}

STDMETHODIMP ContextMenuExt::QueryInterface(REFIID riid, void **ppv) {
  const QITAB QITable[] = {
      QITABENT(ContextMenuExt, IShellExtInit),
      QITABENT(ContextMenuExt, IContextMenu),
      {0},
  };

  HRESULT hr = ::QISearch(this, QITable, riid, ppv);
#ifdef DEBUG_QI
  if (hr == E_NOINTERFACE) {
    std::wstring guid = RegUtil::GuidToString(riid);
    Log(L"QI of ContextMenuExt: %s\n", guid.c_str());
  }
#endif
  return hr;
}

STDMETHODIMP_(ULONG) ContextMenuExt::AddRef() {
  return ::InterlockedIncrement(&mRef);
}

STDMETHODIMP_(ULONG) ContextMenuExt::Release() {
  auto cref = ::InterlockedDecrement(&mRef);
  if (cref == 0) {
    Log(L"Destroying ContextMenuExt %p\n", this);
    delete this;
  }
  return cref;
}

void PrintDataObject(IDataObject *pdtobj) {
  if (!pdtobj) {
    return;
  }

  STGMEDIUM medium;
  FORMATETC fe = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_FILE};

  HRESULT hr = pdtobj->GetData(&fe, &medium);
  if (hr == DV_E_FORMATETC) {
    medium = {TYMED_HGLOBAL};
    fe = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    hr = pdtobj->GetData(&fe, &medium);
  }

  if (SUCCEEDED(hr)) {
    switch (medium.tymed) {
    case TYMED_FILE:
      Log(L"TYMED_FILE: %s\n", medium.lpszFileName);
      break;
    case TYMED_HGLOBAL:
      if (HDROP drop = reinterpret_cast<HDROP>(GlobalLock(medium.hGlobal))) {
        WCHAR path[MAX_PATH];
        if (::DragQueryFileW(drop, 0, path, MAX_PATH)) {
          Log(L"TYMED_HGLOBAL: %s\n", path);
        }
        ::GlobalUnlock(medium.hGlobal);
      }
      break;
    }

    ::ReleaseStgMedium(&medium);
  } else {
    Log(L"IDataObject::GetData failed - %08x\n", hr);
  }
}

STDMETHODIMP ContextMenuExt::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                                        IDataObject *pdtobj, HKEY hkeyProgID) {
  PrintDataObject(pdtobj);
  return S_OK;
}

STDMETHODIMP ContextMenuExt::QueryContextMenu(HMENU hmenu, UINT indexMenu,
                                              UINT idCmdFirst, UINT idCmdLast,
                                              UINT uFlags) {
  wchar_t label[] = L":)";

  MENUITEMINFOW item = {sizeof(MENUITEMINFO)};
  item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
  item.fType = MFT_STRING;
  item.fState = MFS_ENABLED;
  item.wID = idCmdFirst;
  item.dwTypeData = label;
  item.cch = ARRAYSIZE(label) - 1;

  if (!::InsertMenuItemW(hmenu, indexMenu, /*fByPosition*/ TRUE, &item)) {
    DWORD gle = GetLastError();
    Log(L"InsertMenuItemW failed - %08lx\n", gle);
    return HRESULT_FROM_WIN32(gle);
  }

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

STDMETHODIMP ContextMenuExt::InvokeCommand(CMINVOKECOMMANDINFO *pici) {
  Log(L"ContextMenuExt::InvokeCommand\n");
  return S_OK;
}

STDMETHODIMP ContextMenuExt::GetCommandString(UINT_PTR idCmd, UINT uType,
                                              UINT *pReserved, CHAR *pszName,
                                              UINT cchMax) {
  return S_OK;
}

IContextMenu *CreateContextMenuExt() { return new ContextMenuExt; }
