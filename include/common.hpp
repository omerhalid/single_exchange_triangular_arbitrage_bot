#pragma once
#include <cstdint>
#include <string_view>

using Price = double;
using Qty   = double;
using Ts    = std::uint64_t;

struct Quote{
    double px;  // price
    double qty; // quantity
};

constexpr auto version = "TriArbBot 0.0.1";
