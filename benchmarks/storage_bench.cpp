#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "storage.hpp"

namespace {

using Clock = std::chrono::steady_clock;

volatile std::size_t benchmark_sink = 0;

struct BenchmarkResult {
    std::string_view name;
    std::size_t iterations;
    std::chrono::nanoseconds elapsed;
};

template <typename Fn>
BenchmarkResult benchmark(
    std::string_view name,
    std::size_t iterations,
    Fn&& fn) {

    const auto start = Clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        fn();
    }

    const auto end = Clock::now();

    return {
        .name = name,
        .iterations = iterations,
        .elapsed =
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(end - start)
    };
}

void print_result(const BenchmarkResult& result) {

    const double total_ns =
        static_cast<double>(result.elapsed.count());

    const double ns_per_op =
        total_ns /
        static_cast<double>(result.iterations);

    const double ops_per_sec =
        1'000'000'000.0 /
        ns_per_op;

    std::cout
        << std::left
        << std::setw(32)
        << result.name
        << " | "
        << std::right
        << std::setw(12)
        << std::fixed
        << std::setprecision(2)
        << ns_per_op
        << " ns/op"
        << " | "
        << std::setw(12)
        << std::setprecision(0)
        << ops_per_sec
        << " ops/sec\n";
}


template <std::size_t N>
fix::FixMessage<N> make_message() {

    fix::FixMessage<N> message;

    constexpr std::array<std::pair<fix::Tag, std::string_view>, 12>
        fields{{
            {8,   "FIX.4.4"},
            {9,   "100"},
            {35,  "D"},
            {34,  "42"},
            {49,  "SENDER"},
            {52,  "20260816-10:00:00.000"},
            {56,  "TARGET"},
            {11,  "ORDER-123"},
            {21,  "1"},
            {55,  "AAPL"},
            {54,  "1"},
            {38,  "100"}
        }};

    for (const auto& [tag, value] : fields) {

        if (!message.push(tag, value)) {
            break;
        }
    }

    return message;
}


void benchmark_construction() {

    constexpr std::size_t iterations =
        5'000'000;

    const auto result =
        benchmark(
            "FixMessage construction",
            iterations,
            [] {
                fix::FixMessage<64> message;

                benchmark_sink += message.size();
            }
        );

    print_result(result);
}


void benchmark_push() {

    constexpr std::size_t iterations =
        5'000'000;

    const auto result =
        benchmark(
            "push 12 FIX fields",
            iterations,
            [] {
                auto message =
                    make_message<64>();

                benchmark_sink += message.size();
            }
        );

    print_result(result);
}


void benchmark_iteration() {

    constexpr std::size_t iterations =
        5'000'000;

    const auto message =
        make_message<64>();

    const auto result =
        benchmark(
            "iterate 12 FIX fields",
            iterations,
            [&message] {

                std::size_t total = 0;

                for (const auto& field : message) {
                    total += field.tag;
                    total += field.value.size();
                }

                benchmark_sink += total;
            }
        );

    print_result(result);
}


void benchmark_find() {

    constexpr std::size_t iterations =
        5'000'000;

    const auto message =
        make_message<64>();

    const auto result =
        benchmark(
            "find tag 55",
            iterations,
            [&message] {

                const auto* field =
                    message.find(55);

                if (field != nullptr) {
                    benchmark_sink +=
                        field->value.size();
                }
            }
        );

    print_result(result);
}


void benchmark_get() {

    constexpr std::size_t iterations =
        5'000'000;

    const auto message =
        make_message<64>();

    const auto result =
        benchmark(
            "get tag 55",
            iterations,
            [&message] {

                const auto value =
                    message.get(55);

                benchmark_sink +=
                    value.size();
            }
        );

    print_result(result);
}


template <std::size_t FieldCount>
void benchmark_message_size(
    std::string_view name) {

    constexpr std::size_t iterations =
        5'000'000;

    const auto result =
        benchmark(
            name,
            iterations,
            [] {

                fix::FixMessage<64> message;

                constexpr std::array<
                    std::pair<fix::Tag, std::string_view>,
                    12
                > fields{{
                    {8,  "FIX.4.4"},
                    {35, "D"},
                    {49, "SENDER"},
                    {56, "TARGET"},
                    {11, "ORDER-123"},
                    {21, "1"},
                    {55, "AAPL"},
                    {54, "1"},
                    {38, "100"},
                    {40, "2"},
                    {44, "100.50"},
                    {60, "20260816"}
                }};

                for (std::size_t i = 0;
                     i < FieldCount;
                     ++i) {

                    message.push(
                        fields[i].first,
                        fields[i].second
                    );
                }

                benchmark_sink +=
                    message.size();
            }
        );

    print_result(result);
}


void print_layout() {

    std::cout
        << "\nMemory layout\n"
        << "-------------\n";

    std::cout
        << "sizeof(FixField):        "
        << sizeof(fix::FixField)
        << " bytes\n";

    std::cout
        << "alignof(FixField):       "
        << alignof(fix::FixField)
        << " bytes\n";

    std::cout
        << "sizeof(FixMessage<8>):   "
        << sizeof(fix::FixMessage<8>)
        << " bytes\n";

    std::cout
        << "sizeof(FixMessage<64>):  "
        << sizeof(fix::FixMessage<64>)
        << " bytes\n";

    std::cout
        << "alignof(FixMessage<64>): "
        << alignof(fix::FixMessage<64>)
        << " bytes\n";
}

} // namespace


int main() {

    std::cout
        << "FlooFIX Storage Benchmarks\n"
        << "==========================\n\n";

    print_layout();

    std::cout
        << "\nBenchmarks\n"
        << "----------\n";

    benchmark_construction();
    benchmark_push();
    benchmark_iteration();
    benchmark_find();
    benchmark_get();

    benchmark_message_size<1>(
        "construct 1 field");

    benchmark_message_size<4>(
        "construct 4 fields");

    benchmark_message_size<8>(
        "construct 8 fields");

    benchmark_message_size<12>(
        "construct 12 fields");

    std::cout
        << "\nSink: "
        << benchmark_sink
        << '\n';

    return 0;
}