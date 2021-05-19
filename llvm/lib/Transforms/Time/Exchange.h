#include <utility>

#ifndef __cpp_lib_exchange_function
#define __cpp_lib_exchange_function

namespace std {

template<typename T, typename U=T>
T exchange(T& obj, U&& new_val) {
    T old_val = std::move(obj);
    obj = std::forward<U>(new_val);
    return old_val;
}

}

#endif
