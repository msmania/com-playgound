#include "interfaces.h"
#include "regutils.h"
#include <atlbase.h>
#include <cassert>
#include <windows.h>

void Log(const wchar_t *format, ...);

class MainObject : public IMarshalable,
                   public IMarshalable_NoDual,
                   public IMarshalable_OleAuto {
  ULONG mRef;

public:
  MainObject();
  virtual ~MainObject() = default;

  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IDispatch
  IFACEMETHODIMP GetTypeInfoCount(UINT *pctinfo);
  IFACEMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
  IFACEMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                               LCID lcid, DISPID *rgDispId);
  IFACEMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                        WORD wFlags, DISPPARAMS *pDispParams,
                        VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                        UINT *puArgErr);

  // IMarshalable
  IFACEMETHODIMP TestNumbers(
      /* [in] */ long numberIn,
      /* [in] */ long *pnumberIn,
      /* [out] */ int *numberOut,
      /* [out][in] */ unsigned long *numberInOut,
      /* [retval][out] */ unsigned int *numberRetval);

  IFACEMETHODIMP TestWideStrings(
      /* [string][in] */ wchar_t *strIn,
      /* [string][out][in] */ wchar_t *strInOut,
      /* [string][out] */ wchar_t **strOut);

  IFACEMETHODIMP TestBStrings(
      /* [in] */ BSTR strIn,
      /* [out] */ BSTR *strOut,
      /* [out][in] */ BSTR *strInOut);

  IFACEMETHODIMP DelegateCall(
      /* [out][in] */ IMarshalable **objectInOut);

  // IMarshalable_NoDual
  IFACEMETHODIMP TestNumbers_NoDual() {
    assert(0);
    return E_NOTIMPL;
  }

  // IMarshalable_OleAuto
  IFACEMETHODIMP TestNumbers_OleAuto(
      /* [in] */ long numberIn,
      /* [in] */ long *pnumberIn,
      /* [out] */ int *numberOut,
      /* [out][in] */ unsigned long *numberInOut,
      /* [retval][out] */ unsigned int *numberRetval) {
    return TestNumbers(numberIn, pnumberIn, numberOut, numberInOut,
                       numberRetval);
  }
};

MainObject::MainObject() : mRef(1) {
  Log(L"[%04x] MainObject: %p\n", ::GetCurrentThreadId(), this);
}

STDMETHODIMP MainObject::QueryInterface(REFIID riid, void **ppv) {
  const QITAB QITable[] = {
      QITABENT(MainObject, IMarshalable),
      QITABENT(MainObject, IMarshalable_NoDual),
      QITABENT(MainObject, IMarshalable_OleAuto),
      {0},
  };

  HRESULT hr = ::QISearch(this, QITable, riid, ppv);
#ifdef DEBUG_QI
  if (hr == E_NOINTERFACE) {
    std::wstring guid = RegUtil::GuidToString(riid);
    Log(L"QI of MainObject: %s\n", guid.c_str());
  }
#endif
  return hr;
}

STDMETHODIMP_(ULONG) MainObject::AddRef() {
#ifdef DEBUG_REF
  Log(L"%p: %u -> %u\n", this, mRef, mRef + 1);
#endif
  return ::InterlockedIncrement(&mRef);
}

STDMETHODIMP_(ULONG) MainObject::Release() {
#ifdef DEBUG_REF
  Log(L"%p: %u -> %u\n", this, mRef, mRef - 1);
#endif
  auto cref = ::InterlockedDecrement(&mRef);
  if (cref == 0) {
    Log(L"Destroying MainObject %p\n", this);
    delete this;
  }
  return cref;
}

STDMETHODIMP MainObject::GetTypeInfoCount(UINT *pctinfo) {
  assert(0);
  return E_NOTIMPL;
}

STDMETHODIMP MainObject::GetTypeInfo(UINT iTInfo, LCID lcid,
                                     ITypeInfo **ppTInfo) {
  assert(0);
  return E_NOTIMPL;
}

STDMETHODIMP MainObject::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *rgDispId) {
  assert(0);
  return E_NOTIMPL;
}

STDMETHODIMP MainObject::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                WORD wFlags, DISPPARAMS *pDispParams,
                                VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                                UINT *puArgErr) {
  assert(0);
  return E_NOTIMPL;
}

STDMETHODIMP MainObject::TestNumbers(
    /* [in] */ long numberIn,
    /* [in] */ long *pnumberIn,
    /* [out] */ int *numberOut,
    /* [out][in] */ unsigned long *numberInOut,
    /* [retval][out] */ unsigned int *numberRetval) {
  Log(L"[%04x] %S: %ld %ld %d %ld %u\n", ::GetCurrentThreadId(), __FUNCTION__,
      numberIn, *pnumberIn, *numberOut, *numberInOut, *numberRetval);
  *pnumberIn = 41;
  *numberOut = 42;
  *numberInOut = 43;
  *numberRetval = 44;
  return S_OK;
}

static const wchar_t kOutput[] = L":)\u0000:)\u0000:)";
constexpr size_t kOnputCch = sizeof(kOutput) / sizeof(wchar_t) - 1;

STDMETHODIMP MainObject::TestWideStrings(
    /* [string][in] */ wchar_t *strIn,
    /* [string][out][in] */ wchar_t *strInOut,
    /* [string][out] */ wchar_t **strOut) {
  if (*strOut) {
    return E_POINTER;
  }

  Log(L"%S: %s %s\n", __FUNCTION__, strIn, strInOut);
  memcpy(strIn, kOutput, sizeof(kOutput));
  memcpy(strInOut, kOutput, sizeof(kOutput));

  wchar_t *buf = reinterpret_cast<wchar_t *>(::CoTaskMemAlloc(sizeof(kOutput)));
  Log(L"  Allocated buffer: %p\n", buf);
  memcpy(buf, kOutput, sizeof(kOutput));
  *strOut = buf;

  return S_OK;
}

STDMETHODIMP MainObject::TestBStrings(
    /* [in] */ BSTR strIn,
    /* [out] */ BSTR *strOut,
    /* [out][in] */ BSTR *strInOut) {
  if (*strOut) {
    return E_POINTER;
  }

  Log(L"%S: %s %s\n", __FUNCTION__, strIn, *strInOut);
  strIn[0] = L'@';
  ::SysReAllocStringLen(strInOut, kOutput, kOnputCch);

  CComBSTR bstr(kOnputCch, kOutput);
  *strOut = bstr.Detach();

  return S_OK;
}

STDMETHODIMP MainObject::DelegateCall(
    /* [out][in] */ IMarshalable **objectInOut) {
  if (!*objectInOut) {
    *objectInOut = new MainObject;
    return *objectInOut ? S_OK : E_OUTOFMEMORY;
  }

  CComPtr<IMarshalable> target(*objectInOut);
  long a = 101;
  int b;
  unsigned long c = 102;
  unsigned int d;
  return target->TestNumbers(100, &a, &b, &c, &d);
}

IUnknown *CreateMarshalable() {
  return static_cast<IMarshalable *>(new MainObject);
}
