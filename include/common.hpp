#pragma once
#include <cstdint>
#include <string_view>

namespace triarb {

using Price = double;
using Qty   = double;
using Ts    = std::uint64_t;

struct Quote {
    double px;  // price
    double qty; // quantity
};

constexpr auto version = "TriArbBot 0.0.2";

} // namespace triarb
