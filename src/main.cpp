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

template <typename T> class Marshaler final {
  CComPtr<IStream> mStream;

  void Release() {
    if (!mStream) {
      return;
    }

    // CoReleaseMarshalData also reads data from the stream.
    // Need to set the stream position.
    HRESULT hr = mStream->Seek({}, STREAM_SEEK_SET, nullptr);
    if (FAILED(hr)) {
      Log(L"IStream::Seek failed - %08lx\n", hr);
    }

    hr = ::CoReleaseMarshalData(mStream);
    if (FAILED(hr)) {
      Log(L"CoReleaseMarshalData failed - %08lx\n", hr);
    }

    mStream = nullptr;
  }

public:
  Marshaler(T *source) {
    if (!source) {
      return;
    }

    CComPtr<IStream> stream;
    HRESULT hr = ::CreateStreamOnHGlobal(
        /*hGlobal*/ nullptr,
        /*fDeleteOnRelease*/ TRUE, &stream);
    if (FAILED(hr)) {
      Log(L"CreateStreamOnHGlobal failed - %08lx\n", hr);
      return;
    }

    hr = ::CoMarshalInterface(stream, __uuidof(T), source, MSHCTX_INPROC,
                              /*pvDestContext*/ nullptr, MSHLFLAGS_NORMAL);
    if (FAILED(hr)) {
      Log(L"CoMarshalInterface failed - %08lx\n", hr);
      return;
    }

    mStream.Attach(stream.Detach());
  }

  Marshaler(const Marshaler &other) = delete;
  Marshaler &operator=(const Marshaler &) = delete;

  Marshaler(Marshaler &&other) { mStream.Attach(other.mStream.Detach()); }
  Marshaler &operator=(Marshaler &&other) {
    if (this != &other) {
      // Calling IStream::Release is not enough to release the stream
      Release();
      mStream.Attach(other.mStream.Detach());
    }
    return *this;
  }

  ~Marshaler() { Release(); }

  constexpr operator bool() const { return mStream; }

  void Examine() const {
    if (!mStream) {
      return;
    }

    HGLOBAL memBlock;
    HRESULT hr = ::GetHGlobalFromStream(mStream, &memBlock);
    if (FAILED(hr)) {
      Log(L"GetHGlobalFromStream failed - %08lx\n", hr);
      return;
    }

    SIZE_T size = ::GlobalSize(memBlock);
    uint8_t *raw = reinterpret_cast<uint8_t *>(::GlobalLock(memBlock));
    if (!raw) {
      Log(L"GlobalSize failed - %08lx\n", ::GetLastError());
      return;
    }

    Log(L"%p (%d bytes)\n", raw, size);
    for (SIZE_T i = 0; i < size; ++i) {
      if ((i + 1) % 16 == 0 || i == size - 1) {
        Log(L" %02x\n", raw[i]);
      } else {
        Log(L" %02x", raw[i]);
      }
    }

    if (::GlobalUnlock(memBlock) > 0) {
      Log(L"Failed to unlock the memory block failed\n");
      return;
    }

    DWORD gle = ::GetLastError();
    if (gle != NO_ERROR) {
      Log(L"Failed to unlock the memory block failed - %08lx\n", gle);
      return;
    }
  }

  CComPtr<T> Unmarshal() {
    if (!mStream) {
      return nullptr;
    }

    HRESULT hr = mStream->Seek({}, STREAM_SEEK_SET, nullptr);
    if (FAILED(hr)) {
      Log(L"IStream::Seek failed - %08lx\n", hr);
      return nullptr;
    }

    T *raw = nullptr;
    hr = ::CoUnmarshalInterface(mStream, __uuidof(T),
                                reinterpret_cast<void **>(&raw));
    if (FAILED(hr)) {
      Log(L"CoUnmarshalInterface failed - %08lx\n", hr);
      return nullptr;
    }

    mStream = nullptr; // one-time marshaling

    CComPtr<T> proxy;
    proxy.Attach(raw); // CoMarshalInterface already addref'ed the object
    return proxy;
  }
};

TEST(STA, StreamMarshaling) {
  std::thread t1(ComThread<COINIT_APARTMENTTHREADED>, []() {
    CComPtr<IMarshalable> comobj;
    ASSERT_EQ(comobj.CoCreateInstance(kCLSID_ExtZ_InProc_STA,
                                      /*pUnkOuter*/ nullptr,
                                      CLSCTX_INPROC_SERVER),
              S_OK);

    Marshaler<IMarshalable> marsh1(comobj), marsh2(comobj);
    Marshaler<IUnknown> marsh_unk(comobj);

    CComQIPtr<IMarshalable_NoDual> qi = comobj;
    Marshaler<IMarshalable_NoDual> unmarshalable(qi);

    comobj = nullptr;
    qi = nullptr;

    ASSERT_TRUE(marsh1);
    ASSERT_TRUE(marsh2);
    ASSERT_TRUE(marsh_unk);
    ASSERT_TRUE(unmarshalable);
    marsh1.Examine();
    marsh2.Examine();
    marsh_unk.Examine();
    unmarshalable.Examine();

    std::thread t2([marsh1 = std::move(marsh1), marsh2 = std::move(marsh2),
                    marsh_unk = std::move(marsh_unk),
                    unmarshalable = std::move(unmarshalable)]() mutable {
      ComThread<COINIT_APARTMENTTHREADED>(
          [marsh1 = std::move(marsh1), marsh2 = std::move(marsh2),
           marsh_unk = std::move(marsh_unk),
           unmarshalable = std::move(unmarshalable)]() mutable {
            CComPtr proxy1(marsh1.Unmarshal());
            CComPtr proxy2(marsh2.Unmarshal());
            CComPtr proxy3(marsh_unk.Unmarshal());
            ASSERT_TRUE(proxy1);
            EXPECT_TRUE(proxy2);
            ASSERT_TRUE(proxy3);
            TestTestNumbers(proxy1);
            TestTestNumbers(CComQIPtr<IMarshalable>(proxy3));

            proxy1 = nullptr;
            proxy3 = nullptr;

            // CoUnmarshalInterface will fail with E_FAIL
            Marshaler local_unmarshalable = std::move(unmarshalable);
            CComPtr proxy_fail(local_unmarshalable.Unmarshal());
            EXPECT_FALSE(proxy_fail);

            Marshaler<IMarshalable> marsh_proxy(proxy2);
            proxy2 = nullptr;
            std::thread t3([marsh = std::move(marsh_proxy)]() mutable {
              ComThread<COINIT_APARTMENTTHREADED>(
                  [marsh = std::move(marsh)]() mutable {
                    CComPtr proxy(marsh.Unmarshal());
                    ASSERT_TRUE(proxy);
                    TestTestNumbers(proxy);
                    proxy = nullptr;
                    // DebugBreak();
                  });
            });
            t3.join();
          });
    });
    ThreadMsgWaitForSingleObject(t2.native_handle(), INFINITE);
    t2.join();
  });
  t1.join();
}

TEST(STA, TableMarshaling) {
  std::thread t1(ComThread<COINIT_APARTMENTTHREADED>, []() {
    CComPtr<IMarshalable> comobj;
    ASSERT_EQ(comobj.CoCreateInstance(kCLSID_ExtZ_InProc_STA,
                                      /*pUnkOuter*/ nullptr,
                                      CLSCTX_INPROC_SERVER),
              S_OK);
    CComPtr<IGlobalInterfaceTable> git;
    ASSERT_EQ(git.CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                                   /*pUnkOuter*/ nullptr, CLSCTX_INPROC_SERVER),
              S_OK);
    DWORD cookie1, cookie2;
    ASSERT_EQ(git->RegisterInterfaceInGlobal(comobj, __uuidof(IMarshalable),
                                             &cookie1),
              S_OK);
    CComQIPtr<IMarshalable_OleAuto> oleauto = comobj;
    ASSERT_EQ(git->RegisterInterfaceInGlobal(
                  oleauto, __uuidof(IMarshalable_OleAuto), &cookie2),
              S_OK);
    comobj = nullptr;
    oleauto = nullptr;
    Log(L"Cookie: %08x %08x\n", cookie1, cookie2);

    std::thread t2(ComThread<COINIT_APARTMENTTHREADED>, [cookie1, cookie2]() {
      CComPtr<IGlobalInterfaceTable> git;
      ASSERT_EQ(git.CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                                     /*pUnkOuter*/ nullptr,
                                     CLSCTX_INPROC_SERVER),
                S_OK);

      CComPtr<IMarshalable> proxy;
      ASSERT_EQ(git->GetInterfaceFromGlobal(cookie1, __uuidof(IMarshalable),
                                            reinterpret_cast<void **>(&proxy)),
                S_OK);
      EXPECT_EQ(git->RevokeInterfaceFromGlobal(cookie1), S_OK);
      TestTestNumbers(proxy);

      CComPtr<IMarshalable_OleAuto> oleauto;
      ASSERT_EQ(
          git->GetInterfaceFromGlobal(cookie2, __uuidof(IMarshalable_OleAuto),
                                      reinterpret_cast<void **>(&oleauto)),
          S_OK);
      EXPECT_EQ(git->RevokeInterfaceFromGlobal(cookie2), S_OK);
      TestTestNumbers(CComQIPtr<IMarshalable>(oleauto));
      oleauto = nullptr;

      DWORD cookie_proxy;
      ASSERT_EQ(git->RegisterInterfaceInGlobal(proxy, __uuidof(IMarshalable),
                                               &cookie_proxy),
                S_OK);
      proxy = nullptr;
      Log(L"Cookie: %08x\n", cookie_proxy);

      std::thread t3(ComThread<COINIT_APARTMENTTHREADED>,
                     [git = std::move(git), cookie = cookie_proxy]() {
                       CComPtr<IMarshalable> proxy;
                       ASSERT_EQ(git->GetInterfaceFromGlobal(
                                     cookie, __uuidof(IMarshalable),
                                     reinterpret_cast<void **>(&proxy)),
                                 S_OK);
                       TestTestNumbers(proxy);
                       proxy = nullptr;

                       EXPECT_EQ(git->RevokeInterfaceFromGlobal(cookie), S_OK);

                       // RevokeInterfaceFromGlobal posts a message to revoke a
                       // table entry. Wait some cycles to give the message loop
                       // to process that message to see the instance be
                       // destroyed.
                       ::Sleep(100);
                       // DebugBreak();
                     });
      // This apartment also needs to process window messages in order
      // to revoke the |cookie_proxy|.
      ThreadMsgWaitForSingleObject(t3.native_handle(), INFINITE);
      t3.join();
    });
    ThreadMsgWaitForSingleObject(t2.native_handle(), INFINITE);
    t2.join();
  });
  t1.join();
}
