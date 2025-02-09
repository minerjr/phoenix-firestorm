/**
 * @file llatomic.h
 * @brief Base classes for atomic.
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLATOMIC_H
#define LL_LLATOMIC_H

#include "stdtypes.h"

#include <memory>
#include <atomic>
#include <optional>

template <typename Type, typename AtomicType = std::atomic< Type > > class LLAtomicBase
{
public:
    LLAtomicBase() {};
    LLAtomicBase(Type x) { mData.store(x); }
    ~LLAtomicBase() {};

    operator const Type() { return mData; }

    Type    CurrentValue() const { return mData; }

    Type operator =(const Type& x) { mData.store(x); return mData; }
    void operator -=(Type x) { mData -= x; }
    void operator +=(Type x) { mData += x; }
    Type operator ++(int) { return mData++; }
    Type operator --(int) { return mData--; }

    Type operator ++() { return ++mData; }
    Type operator --() { return --mData; }

private:
    AtomicType mData;
};

// Typedefs for specialized versions. Using std::atomic_(u)int32_t to get the optimzed implementation.
#ifdef LL_WINDOWS
typedef LLAtomicBase<U32, std::atomic_uint32_t> LLAtomicU32;
typedef LLAtomicBase<S32, std::atomic_int32_t> LLAtomicS32;
// <FS:minerjr>
// Added 64bit atomic values
typedef LLAtomicBase<U64, std::atomic_uint64_t> LLAtomicU64;
typedef LLAtomicBase<S64, std::atomic_int64_t> LLAtomicS64;
// <FS:minerjr>
#else
typedef LLAtomicBase<U32, std::atomic_uint> LLAtomicU32;
typedef LLAtomicBase<S32, std::atomic_int> LLAtomicS32;
// <FS:minerjr>
// Added 64bit atomic values for linux, osx
typedef LLAtomicBase<U64, std::atomic_ullong> LLAtomicU64;
typedef LLAtomicBase<S64, std::atomic_llong> LLAtomicS64;
// <FS:minerjr>
#endif

typedef LLAtomicBase<bool, std::atomic_bool> LLAtomicBool;

// <FS:minerjr>
// New Atomic Queue
template <typename Type> class FSAtomicQueue
{
public:
    struct FSAtomicQueueNode
    {
        std::atomic<std::shared_ptr<Type>> mData;
        LLAtomicU64 mState;
        std::atomic<FSAtomicQueueNode *> mNext;
        std::atomic<FSAtomicQueueNode *> mPrev;

        FSAtomicQueueNode(FSAtomicQueueNode&) = delete;
        void operator=(FSAtomicQueueNode&) = delete;
    };

    FSAtomicQueue() = default;
    ~FSAtomicQueue() = default;

    void push(Type value)
    {
        auto active = std::make_shared<FSAtomicQueueNode>(std::move(value), mHead.load());
        while (not mHead.compare_exchange_weak(active->mNext, active));
    }

    std::optional<Type> pop()
    {
        auto active = mHead.load();
        while (active != nullptr && not mTail.compare_exchange_weak(active, active->next));

        if (active != nullptr)
        {
            return {std::move(active->value)};
        }
        else
        {
            return std::nullopt;
        }
    }
protected:

private:

    std::atomic<std::shared_ptr<FSAtomicQueueNode>> mHead;
    std::atomic<std::shared_ptr<FSAtomicQueueNode>> mTail;
};
// <FS:minerjr>

#endif // LL_LLATOMIC_H
