#ifndef INSTRUCTIONTRAITS_H
#define INSTRUCTIONTRAITS_H

#include "instructions/allinstructions.h"
#include <type_traits>

namespace x64 {

    template<typename, typename = void>
    struct is_call : std::false_type { };

    template <typename INS>
    struct is_call<INS, std::void_t<decltype(INS::is_call)>> : std::true_type { };

    template<typename INS>
    inline constexpr bool is_call_v = is_call<INS>::value;



    template<typename, typename = void>
    struct is_x87 : std::false_type { };

    template <typename INS>
    struct is_x87<INS, std::void_t<decltype(INS::is_x87)>> : std::true_type { };

    template<typename INS>
    inline constexpr bool is_x87_v = is_x87<INS>::value;



    template<typename, typename = void>
    struct is_sse : std::false_type { };

    template <typename INS>
    struct is_sse<INS, std::void_t<decltype(INS::is_sse)>> : std::true_type { };

    template<typename INS>
    inline constexpr bool is_sse_v = is_sse<INS>::value;

}

#endif