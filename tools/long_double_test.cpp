#include "math/Real.h"

#include <iostream>
#include <limits>
#include <type_traits>

template <typename T> void printTypeInfo(const char* name)
{
    std::cout << name << '\n'
              << "  sizeof: " << sizeof(T) << " bytes\n"
              << "  digits: " << std::numeric_limits<T>::digits << " binary digits\n"
              << "  digits10: " << std::numeric_limits<T>::digits10 << " decimal digits\n"
              << "  max_digits10: " << std::numeric_limits<T>::max_digits10 << '\n'
              << "  is_iec559: " << std::numeric_limits<T>::is_iec559 << '\n';
}

int main()
{
    printTypeInfo<double>("double");
    printTypeInfo<long double>("long double");
    printTypeInfo<Real>("Real");

    std::cout << "Real is double: " << std::is_same_v<Real, double> << '\n'
              << "Real is long double: " << std::is_same_v<Real, long double> << '\n';

#if defined(_MSC_VER)
    std::cout << "compiler: MSVC\n";
#elif defined(__clang__)
    std::cout << "compiler: Clang\n";
#elif defined(__GNUC__)
    std::cout << "compiler: GCC\n";
#else
    std::cout << "compiler: unknown\n";
#endif

#if defined(__x86_64__) || defined(_M_X64)
    std::cout << "arch: x86_64\n";
#elif defined(__i386__) || defined(_M_IX86)
    std::cout << "arch: x86\n";
#else
    std::cout << "arch: other\n";
#endif
}
