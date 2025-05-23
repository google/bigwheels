// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ppx_ccomptr_h
#define ppx_ccomptr_h

// Borrowed from DXC

#if defined(PPX_D3D12)

#if defined(PPX_MSW)
#if !defined(VC_EXTRALEAN)
#define VC_EXTRALEAN
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#endif
#include <windows.h>
#include <unknwn.h>

template <class T>
class CComPtrBase
{
protected:
    CComPtrBase() throw() { p = nullptr; }
    CComPtrBase(T* lp) throw()
    {
        p = lp;
        if (p != nullptr)
            p->AddRef();
    }
    void Swap(CComPtrBase& other)
    {
        T* pTemp = p;
        p        = other.p;
        other.p  = pTemp;
    }

public:
    ~CComPtrBase() throw()
    {
        if (p) {
            p->Release();
            p = nullptr;
        }
    }
        operator T*() const throw() { return p; }
    T&  operator*() const { return *p; }
    T*  operator->() const { return p; }
    T** operator&() throw()
    {
        assert(p == nullptr);
        return &p;
    }
    bool operator!() const throw() { return (p == nullptr); }
    bool operator<(T* pT) const throw() { return p < pT; }
    bool operator!=(T* pT) const { return !operator==(pT); }
    bool operator==(T* pT) const throw() { return p == pT; }

    // Release the interface and set to nullptr
    void Release() throw()
    {
        T* pTemp = p;
        if (pTemp) {
            p = nullptr;
            pTemp->Release();
        }
    }

    // Attach to an existing interface (does not AddRef)
    void Attach(T* p2) throw()
    {
        if (p) {
            ULONG ref = p->Release();
            (void)(ref);
            // Attaching to the same object only works if duplicate references are
            // being coalesced.  Otherwise re-attaching will cause the pointer to be
            // released and may cause a crash on a subsequent dereference.
            assert(ref != 0 || p2 != p);
        }
        p = p2;
    }

    // Detach the interface (does not Release)
    T* Detach() throw()
    {
        T* pt = p;
        p     = nullptr;
        return pt;
    }

    HRESULT CopyTo(T** ppT) throw()
    {
        assert(ppT != nullptr);
        if (ppT == nullptr)
            return E_POINTER;
        *ppT = p;
        if (p)
            p->AddRef();
        return S_OK;
    }

    template <class Q>
    HRESULT QueryInterface(Q** pp) const throw()
    {
        assert(pp != nullptr);
        return p->QueryInterface(__uuidof(Q), (void**)pp);
    }

    T* p;
};

template <class T>
class CComPtr : public CComPtrBase<T>
{
public:
    typedef T InterfaceType;

    CComPtr() throw() {}
    CComPtr(T* lp) throw()
        : CComPtrBase<T>(lp) {}
    CComPtr(const CComPtr<T>& lp) throw()
        : CComPtrBase<T>(lp.p) {}
    T* operator=(T* lp) throw()
    {
        if (*this != lp) {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }

    inline bool IsEqualObject(IUnknown* pOther) throw()
    {
        if (this->p == nullptr && pOther == nullptr)
            return true; // They are both NULL objects

        if (this->p == nullptr || pOther == nullptr)
            return false; // One is NULL the other is not

        CComPtr<IUnknown> punk1;
        CComPtr<IUnknown> punk2;
        this->p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
        pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
        return punk1 == punk2;
    }

    void ComPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
    {
        IUnknown* pTemp = *pp; // takes ownership
        if (lp == nullptr || FAILED(lp->QueryInterface(riid, (void**)pp)))
            *pp = nullptr;
        if (pTemp)
            pTemp->Release();
    }

    template <typename Q>
    T* operator=(const CComPtr<Q>& lp) throw()
    {
        if (!this->IsEqualObject(lp)) {
            ComPtrAssign((IUnknown**)&this->p, lp, __uuidof(T));
        }
        return *this;
    }

    T* operator=(const CComPtr<T>& lp) throw()
    {
        if (*this != lp) {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }

    CComPtr(CComPtr<T>&& lp) throw()
        : CComPtrBase<T>() { lp.Swap(*this); }

    T* operator=(CComPtr<T>&& lp) throw()
    {
        if (*this != lp) {
            CComPtr(static_cast<CComPtr&&>(lp)).Swap(*this);
        }
        return *this;
    }

    T* Get() const
    {
        return this->p;
    }

    void Reset() throw()
    {
        this->Release();
    }
};

#endif // #if defined(PPX_D3D12)

#endif // ppx_ccomptr_h