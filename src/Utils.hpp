#pragma once

#include <cstdint>
#include <memory>

inline const char* NextUtf8Char(const char *s)
{
    for (++s; *s; ++s)
    {
        if ((static_cast<uint8_t>(*s) & 0xC0) != 0x80)
            return s;
    }
    return s;
}

inline int Utf8Len(const char *s)
{
    int count = 0;
    for (; *s; s = NextUtf8Char(s))
        ++count;
    return count;
}

template <typename T>
using PtrT = std::unique_ptr<T, void(*)(T*)>;

template <typename T>
PtrT<T> NullPtr(void(*d)(T*))
{
    return PtrT<T>(nullptr, d);
}
