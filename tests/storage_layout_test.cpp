#include <cstddef>
#include <iostream>
#include <type_traits>

#include "storage.hpp"

int main() {
    std::cout
        << "FixField size: "
        << sizeof(fix::FixField)
        << " bytes\n";

    std::cout
        << "FixField alignment: "
        << alignof(fix::FixField)
        << " bytes\n";

    std::cout
        << "FixMessage<64> size: "
        << sizeof(fix::FixMessage<64>)
        << " bytes\n";

    std::cout
        << "FixMessage<64> alignment: "
        << alignof(fix::FixMessage<64>)
        << " bytes\n";

    static_assert(
        std::is_trivially_copyable_v<fix::FixField>
    );

    static_assert(
        std::is_trivially_destructible_v<fix::FixField>
    );

    static_assert(
        std::is_trivially_copyable_v<fix::FixMessage<64>>
    );

    static_assert(
        std::is_trivially_destructible_v<fix::FixMessage<64>>
    );

    return 0;
}