#include "regutils.h"
#include <atlbase.h>
#include <shlobj.h>

void Log(const wchar_t *format, ...);
IContextMenu *CreateContextMenuExt();

class ClassFactory : public IClassFactory {
  ULONG mRef;

public:
  ClassFactory();
  virtual ~ClassFactory() = default;

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory
  STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
  STDMETHODIMP LockServer(BOOL fLock);
};

ClassFactory::ClassFactory() : mRef(1) { Log(L"ClassFactory: %p\n", this); }

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv) {
  const QITAB QITable[] = {
      QITABENT(ClassFactory, IClassFactory),
      {0},
  };
  return ::QISearch(this, QITable, riid, ppv);
}

STDMETHODIMP_(ULONG) ClassFactory::AddRef() {
  return ::InterlockedIncrement(&mRef);
}

STDMETHODIMP_(ULONG) ClassFactory::Release() {
  auto cref = ::InterlockedDecrement(&mRef);
  if (cref == 0) {
    Log(L"Destroying ClassFactory %p\n", this);
    delete this;
  }
  return cref;
}

STDMETHODIMP ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                                          void **ppv) {
  if (pUnkOuter) {
    return CLASS_E_NOAGGREGATION;
  }

  CComPtr<IUnknown> instance;
  if (IsEqualCLSID(riid, IID_IContextMenu)) {
    instance.Attach(CreateContextMenuExt());
  } else {
    std::wstring guidStr = RegUtil::GuidToString(riid);
    Log(L"QI: %s\n", guidStr.c_str());
    __debugbreak();
    return E_NOINTERFACE;
  }

  return instance ? instance->QueryInterface(riid, ppv) : E_OUTOFMEMORY;
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock) { return S_OK; }

IUnknown *CreateFactory() { return new ClassFactory; }
