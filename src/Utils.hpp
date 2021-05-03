#pragma once

#include <memory>

template <typename T, typename D = void(*)(T*)>
using PtrT = std::unique_ptr<T, D>;

template <typename T>
PtrT<T> NullPtr(void(*d)(T*))
{
    return PtrT<T>(nullptr, d);
}

template <typename T, typename D>
PtrT<T, D> MkPtrT(T *t, D &&d)
{
    return PtrT<T, D>(t, std::forward<D>(d));
}
