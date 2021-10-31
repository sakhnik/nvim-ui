#pragma once

#include <functional>

// Create a callback function matching a given member function
// to be used as a GCallback.
template <typename T>
class GCallbackAdaptor
{
    // This function is compatible with GSignal/Gtk
    template <auto Func, typename R, typename ...Args>
        static R _CallbackFunc(Args... args, void *data)
        {
            // Call the original member function by the pointer.
            T *self = reinterpret_cast<T *>(data);
            return (self->*Func)(args...);
        }

    // Return a pointer to the static function with the desired signature (gpointer at the end)
    template <auto Func, typename R, typename ...Args>
        static constexpr R (*_Callback(R (T::*)(Args...)))(Args..., void *)
        {
            return _CallbackFunc<Func, R, Args...>;
        }

public:
    template <auto func>
        static constexpr auto MakeCallback()
        {
            // First capture both the function pointer and the signature
            return _Callback<func>(func);
        }
};
