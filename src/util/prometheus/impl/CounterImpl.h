//------------------------------------------------------------------------------
/*
    This file is part of clio: https://github.com/XRPLF/clio
    Copyright (c) 2023, the clio developers.

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL,  DIRECT,  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#pragma once

#include <atomic>
#include <cassert>
#include <concepts>

namespace util::prometheus::detail {

template <typename T>
concept SomeNumberType = std::is_arithmetic_v<T> && !std::is_same_v<T, bool> && !std::is_const_v<T>;

template <typename T>
concept SomeCounterImpl = requires(T a) {
    typename std::remove_cvref_t<T>::ValueType;
    SomeNumberType<typename std::remove_cvref_t<T>::ValueType>;
    {
        a.add(typename std::remove_cvref_t<T>::ValueType{1})
    } -> std::same_as<void>;
    {
        a.set(typename std::remove_cvref_t<T>::ValueType{1})
    } -> std::same_as<void>;
    {
        a.value()
    } -> SomeNumberType;
};

template <SomeNumberType NumberType>
class CounterImpl {
public:
    using ValueType = NumberType;

    CounterImpl() = default;

    CounterImpl(CounterImpl const&) = delete;

    // Move constructor should be used only used during initialization
    CounterImpl(CounterImpl&& other)
    {
        assert(other.value_ == 0);
        value_ = other.value_.exchange(0);
    }

    CounterImpl&
    operator=(CounterImpl const&) = delete;
    CounterImpl&
    operator=(CounterImpl&&) = delete;

    void
    add(ValueType const value)
    {
        if constexpr (std::is_integral_v<ValueType>) {
            value_.fetch_add(value);
        } else {
#if __cpp_lib_atomic_float >= 201711L
            value_.fetch_add(value);
#else
            // Workaround for atomic float not being supported by the standard library
            // cimpares_exchange_weak returns false if the value is not exchanged and updates the current value
            auto current = value_.load();
            while (!value_.compare_exchange_weak(current, current + value)) {
            }
#endif
        }
    }

    void
    set(ValueType const value)
    {
        value_ = value;
    }

    ValueType
    value() const
    {
        return value_;
    }

private:
    std::atomic<ValueType> value_{0};
};

}  // namespace util::prometheus::detail
