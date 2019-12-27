#ifndef CPPAPE_CONSTS_H
#define CPPAPE_CONSTS_H
#include <limits>

namespace ape
{
    template<typename V>
    struct consts
    {
        // standard constants
        static constexpr V pi = static_cast<V>(3.141592653589793238462643383279502884L);
        static constexpr V e = static_cast<V>(2.71828182845904523536);
        static constexpr V tau = static_cast<V>(pi * 2);
        static constexpr V pi_half = static_cast<V>(pi / 2);
        static constexpr V pi_quarter = static_cast<V>(pi / 4);
        static constexpr V four_over_pi = static_cast<V>(4 / pi);
        static constexpr V one = static_cast<V>(1);
        static constexpr V minus_one = static_cast<V>(-1);
        static constexpr V minus_two = static_cast<V>(-2);
        static constexpr V half = static_cast<V>(0.5);
        static constexpr V quarter = static_cast<V>(0.25);
        static constexpr V two = static_cast<V>(2);
        static constexpr V four = static_cast<V>(4);
        static constexpr V sqrt_two = static_cast<V>(1.4142135623730950488016887242097L);
        static constexpr V sqrt_half_two = static_cast<V>(0.70710678118654752440084436210485L);
        static constexpr V sign_bit = static_cast<V>(-0.0);
        static constexpr V epsilon = std::numeric_limits<V>::epsilon();
        static constexpr V max = std::numeric_limits<V>::max();
        static constexpr V min = std::numeric_limits<V>::min();
        static constexpr V zero = static_cast<V>(0);

        //static constexpr V all_bits = static_cast<V>();
        //static constexpr V sign_mask = static_cast<V>();

    };
}

#endif