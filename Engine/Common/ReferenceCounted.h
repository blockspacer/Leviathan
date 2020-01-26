// Leviathan Game Engine
// Copyright (c) 2012-2020 Henri Hyyryläinen
#pragma once
#include "Define.h"
// ------------------------------------ //
#include <atomic>
#include <cstdint>

#ifdef LEVIATHAN_USING_BOOST
#include <boost/intrusive_ptr.hpp>
#endif // LEVIATHAN_USING_BOOST


namespace Leviathan {

template<class T>
using CountedPtr = boost::intrusive_ptr<T>;

#ifdef LEVIATHAN_USING_BOOST
#define REFERENCE_COUNTED_PTR_TYPE(x)          \
    using pointer = CountedPtr<x>;             \
    using const_pointer = CountedPtr<const x>; \
    static constexpr auto ANGELSCRIPT_TYPE =   \
        #x "@"; // Appended @ because these are handle types
#define REFERENCE_COUNTED_PTR_TYPE_NAMED(x, y) \
    using pointer = CountedPtr<x>;             \
    using const_pointer = CountedPtr<const x>; \
    static constexpr auto ANGELSCRIPT_TYPE =   \
        #y "@"; // Appended @ because these are handle types
#else
#define REFERENCE_COUNTED_PTR_TYPE(x)
#define REFERENCE_COUNTED_PTR_TYPE_NAMED(x, y)
#endif // LEVIATHAN_USING_BOOST


//! Reference counted object which will be deleted when all references are gone
//! \note Pointers can be used with ReferenceCounted::pointer ptr = new Object();
//! \todo Make sure that all functions using intrusive pointers use the MakeIntrusive function
class ReferenceCounted {
public:
#ifdef LEVIATHAN_USING_BOOST
    // This needs to added with the REFERENCE_COUNTED_PTR_TYPE to any child class
    // using pointer = CountedPtr<use REFERENCE_COUNTED_PTR_TYPE>;
    // static constexpr auto ANGELSCRIPT_TYPE = "...";
    using basepointer = CountedPtr<ReferenceCounted>;
    using refcountedpointer = CountedPtr<ReferenceCounted>;
#endif // LEVIATHAN_USING_BOOST

    // Prevent directly using this class
protected:
    DLLEXPORT inline ReferenceCounted() : RefCount(1) {}
    DLLEXPORT virtual ~ReferenceCounted() {}

public:
    // Cannot be easily copied because of the reference count
    ReferenceCounted(const ReferenceCounted& other) = delete;
    ReferenceCounted& operator=(const ReferenceCounted& other) = delete;

    FORCE_INLINE void AddRef() const
    {
        intrusive_ptr_add_ref(this);
    }

    //! removes a reference and deletes the object if reference count reaches zero
    FORCE_INLINE void Release() const
    {
        intrusive_ptr_release(this);
    }

    FORCE_INLINE void AddRef()
    {
        intrusive_ptr_add_ref(this);
    }

    FORCE_INLINE void Release()
    {
        intrusive_ptr_release(this);
    }

#ifdef LEVIATHAN_USING_BOOST
    //! \brief Creates an intrusive_ptr from raw pointer
    //!
    //! Releases the reference of the raw pointer (so you don't need to do it manually)
    template<class ActualType>
    static inline CountedPtr<ActualType> WrapPtr(ActualType* ptr)
    {
        if(!ptr)
            return nullptr;

        CountedPtr<ActualType> newptr(ptr);
        ptr->Release();

        return newptr;
    }

    // Copy this comment to any protected constructors that are meant to be
    // accessed through this:
    // These are protected for only constructing properly reference
    // counted instances through MakeShared
    // friend ReferenceCounted;
    // Uncomment the above line after pasting

    //! \brief Constructs a new instance and wraps it
    //! \note Doesn't catch any exceptions
    template<class ActualType, class... Args>
    static CountedPtr<ActualType> MakeShared(Args&&... args)
    {
        CountedPtr<ActualType> ptr(new ActualType(std::forward<Args>(args)...));
        ptr->Release();

        return ptr;
    }

#endif // LEVIATHAN_USING_BOOST

    //! \brief Returns the reference count
    //! \todo Make sure that the right memory order is used
    int32_t GetRefCount() const
    {
        return RefCount.load(std::memory_order_acquire);
    }


protected:
    friend void intrusive_ptr_add_ref(const ReferenceCounted* obj)
    {
        obj->RefCount.fetch_add(1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release(const ReferenceCounted* obj)
    {
        if(obj->RefCount.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete obj;
        }
    }

private:
    mutable std::atomic_int_fast32_t RefCount;
};
} // namespace Leviathan
