#pragma once

#include <memory>
#include <functional>

template <typename T, typename D = void(*)(T*)>
using PtrT = std::unique_ptr<T, D>;

template <typename T>
PtrT<T> NullPtr(void(*d)(T*))
{
    return PtrT<T>(nullptr, d);
}

template <typename T, typename D>
PtrT<T, D> MkPtr(T *t, D &&d)
{
    return PtrT<T, D>(t, std::forward<D>(d));
}

struct scope_exit
{
    using FuncT = std::function<void(void)>;
    scope_exit(FuncT f) : _f{f} {}
    ~scope_exit(void) { _f(); }

private:
    FuncT _f;
};
