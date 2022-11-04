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

#ifndef obj_ptr_h
#define obj_ptr_h

//! @class ObjPtrRefBase
//!
//!
class ObjPtrRefBase
{
public:
    ObjPtrRefBase() {}
    virtual ~ObjPtrRefBase() {}

private:
    friend class ObjPtrBase;
    virtual void Set(void** ppObj) = 0;
};

//! @class ObjPtrBase
//!
//!
class ObjPtrBase
{
public:
    ObjPtrBase() {}
    virtual ~ObjPtrBase() {}

protected:
    void Set(void** ppObj, ObjPtrRefBase* pObjRef) const
    {
        pObjRef->Set(ppObj);
    }
};

//! @class ObjPtrRef
//!
//!
template <typename ObjectT>
class ObjPtrRef
    : public ObjPtrRefBase
{
public:
    ObjPtrRef(ObjectT** ptrRef)
        : mPtrRef(ptrRef) {}

    ~ObjPtrRef() {}

    operator void**() const
    {
        void** addr = reinterpret_cast<void**>(mPtrRef);
        return addr;
    }

    operator ObjectT**()
    {
        return mPtrRef;
    }

    //// clang-format off
    //operator ObjectT** ()
    //{
    //    return mPtrRef;
    //}
    //// clang-format on
    //
    //// clang-format off
    //operator const ObjectT* const* ()
    //{
    //    return mPtrRef;
    //}
    //// clang-format on

private:
    virtual void Set(void** ppObj) override
    {
        *mPtrRef = reinterpret_cast<ObjectT*>(*ppObj);
    }
    // clang-format on

private:
    ObjectT** mPtrRef = nullptr;
};

//! @class ObjPtr
//!
//!
template <typename ObjectT>
class ObjPtr
    : public ObjPtrBase
{
public:
    using object_type = ObjectT;

    ObjPtr(ObjectT* ptr = nullptr)
        : mPtr(ptr)
    {
    }

    ~ObjPtr() {}

    ObjPtr& operator=(const ObjPtr& rhs)
    {
        if (&rhs != this) {
            mPtr = rhs.mPtr;
        }
        return *this;
    }

    ObjPtr& operator=(const ObjectT* rhs)
    {
        if (rhs != mPtr) {
            mPtr = const_cast<ObjectT*>(rhs);
        }
        return *this;
    }

    operator bool() const
    {
        return (mPtr != nullptr);
    }

    bool operator==(const ObjPtr<ObjectT>& rhs) const
    {
        return mPtr == rhs.mPtr;
    }

    ObjectT* Get() const
    {
        return mPtr;
    }

    void Reset()
    {
        mPtr = nullptr;
    }

    void Detach()
    {
    }

    bool IsNull() const
    {
        return mPtr == nullptr;
    }

    operator ObjectT*() const
    {
        return mPtr;
    }

    ObjectT* operator->() const
    {
        return mPtr;
    }

    ObjPtrRef<ObjectT> operator&()
    {
        return ObjPtrRef<ObjectT>(&mPtr);
    }

private:
    ObjectT* mPtr = nullptr;
};

#endif // obj_ptr_h
