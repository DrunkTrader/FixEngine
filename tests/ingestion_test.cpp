#include "ingestion.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace {

constexpr char SOH = '\x01';

constexpr std::string_view message1 =
    "8=FIX.4.2\001"
    "9=10\001"
    "35=0\001"
    "49=A\001"
    "10=123\001";

constexpr std::string_view message2 =
    "8=FIX.4.2\001"
    "9=10\001"
    "35=0\001"
    "49=B\001"
    "10=124\001";

[[nodiscard]] std::span<const std::byte> as_bytes(
    std::string_view text) noexcept {

    return {
        reinterpret_cast<const std::byte*>(text.data()),
        text.size()
    };
}

void test_empty_buffer() {
    fix::FixIngestor<> ingestor;

    const auto result = ingestor.extract();

    assert(result.status == fix::IngestionStatus::NeedMoreData);
    assert(result.need_more_data());
    assert(!result.ready());
    assert(!result.failed());
    assert(result.message.empty());
    assert(result.consumed == 0);
    assert(ingestor.buffered_bytes() == 0);
}

void test_complete_message() {
    fix::FixIngestor<> ingestor;

    const auto result = ingestor.feed(as_bytes(message1));

    assert(result.status == fix::IngestionStatus::MessageReady);
    assert(result.ready());
    assert(!result.need_more_data());
    assert(!result.failed());

    assert(result.message == message1);
    assert(result.consumed == message1.size());

    assert(ingestor.buffered_bytes() == message1.size());
}

void test_incomplete_message() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view partial =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0\001";

    const auto result = ingestor.feed(as_bytes(partial));

    assert(result.status == fix::IngestionStatus::NeedMoreData);
    assert(result.need_more_data());
    assert(result.message.empty());
    assert(result.consumed == 0);

    assert(ingestor.buffered_bytes() == partial.size());
}

void test_fragmented_message() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view part1 =
        "8=FIX.4.2\001"
        "9=";

    constexpr std::string_view part2 =
        "10\001"
        "35=0\001"
        "49=A\001";

    constexpr std::string_view part3 =
        "10=123\001";

    auto result = ingestor.feed(as_bytes(part1));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(part2));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(part3));

    assert(result.ready());
    assert(result.message == message1);
    assert(result.consumed == message1.size());
}

void test_split_inside_begin_string() {
    fix::FixIngestor<> ingestor;

    auto result = ingestor.feed(as_bytes("8="));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(
        "FIX.4.2\001"
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=123\001"
    ));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_begin_string_delimiter() {
    fix::FixIngestor<> ingestor;

    auto result = ingestor.feed(as_bytes(
        "8=FIX.4.2"
    ));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(
        "\001"
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=123\001"
    ));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_body_length() {
    fix::FixIngestor<> ingestor;

    auto result = ingestor.feed(as_bytes(
        "8=FIX.4.2\001"
        "9="
    ));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(
        "10\001"
        "35=0\001"
        "49=A\001"
        "10=123\001"
    ));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_body_length_delimiter() {
    fix::FixIngestor<> ingestor;

    auto result = ingestor.feed(as_bytes(
        "8=FIX.4.2\001"
        "9=10"
    ));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(
        "\001"
        "35=0\001"
        "49=A\001"
        "10=123\001"
    ));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_body() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view part1 =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0";

    constexpr std::string_view part2 =
        "\001"
        "49=A\001"
        "10=123\001";

    auto result = ingestor.feed(as_bytes(part1));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes(part2));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_checksum() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view part1 =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=";

    auto result = ingestor.feed(as_bytes(part1));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes("123\001"));

    assert(result.ready());
    assert(result.message == message1);
}

void test_split_inside_checksum_value() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view part1 =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=1";

    auto result = ingestor.feed(as_bytes(part1));

    assert(result.need_more_data());

    result = ingestor.feed(as_bytes("23\001"));

    assert(result.ready());
    assert(result.message == message1);
}

void test_multiple_messages() {
    fix::FixIngestor<> ingestor;

    std::string input;
    input.reserve(message1.size() + message2.size());
    input.append(message1);
    input.append(message2);

    const auto result = ingestor.feed(as_bytes(input));

    assert(result.ready());
    assert(result.message == message1);
    assert(result.consumed == message1.size());

    assert(ingestor.buffered_bytes() == input.size());

    ingestor.consume(result.consumed);

    assert(ingestor.buffered_bytes() == message2.size());

    const auto second = ingestor.extract();

    assert(second.ready());
    assert(second.message == message2);
    assert(second.consumed == message2.size());
}

void test_consume_partial_buffer() {
    fix::FixIngestor<> ingestor;

    const auto result = ingestor.feed(as_bytes(message1));

    assert(result.ready());

    const std::size_t consume_bytes = 5;

    ingestor.consume(consume_bytes);

    assert(
        ingestor.buffered_bytes() ==
        message1.size() - consume_bytes
    );
}

void test_consume_more_than_buffered() {
    fix::FixIngestor<> ingestor;

    ingestor.feed(as_bytes(message1));

    assert(ingestor.buffered_bytes() == message1.size());

    ingestor.consume(message1.size() + 100);

    assert(ingestor.buffered_bytes() == 0);
}

void test_clear() {
    fix::FixIngestor<> ingestor;

    ingestor.feed(as_bytes(message1));

    assert(ingestor.buffered_bytes() == message1.size());

    ingestor.clear();

    assert(ingestor.buffered_bytes() == 0);
    assert(ingestor.available_bytes() == 64 * 1024);
}

void test_invalid_begin_string() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view invalid =
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=123\001";

    const auto result = ingestor.feed(as_bytes(invalid));

    assert(result.status == fix::IngestionStatus::InvalidMessage);
    assert(result.failed());
    assert(result.message.empty());
    assert(result.consumed == 0);
}

void test_invalid_body_length_tag() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view invalid =
        "8=FIX.4.2\001"
        "35=0\001"
        "49=A\001"
        "10=123\001";

    const auto result = ingestor.feed(as_bytes(invalid));

    assert(result.status == fix::IngestionStatus::InvalidMessage);
    assert(result.failed());
}

void test_invalid_body_length_value() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view invalid =
        "8=FIX.4.2\001"
        "9=abc\001"
        "35=0\001"
        "49=A\001"
        "10=123\001";

    const auto result = ingestor.feed(as_bytes(invalid));

    assert(result.status == fix::IngestionStatus::InvalidMessage);
    assert(result.failed());
}

void test_empty_body_length() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view invalid =
        "8=FIX.4.2\001"
        "9=\001"
        "35=0\001"
        "49=A\001"
        "10=123\001";

    const auto result = ingestor.feed(as_bytes(invalid));

    assert(result.status == fix::IngestionStatus::InvalidMessage);
    assert(result.failed());
}

void test_body_length_points_to_wrong_field() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view invalid =
        "8=FIX.4.2\001"
        "9=5\001"
        "35=0\001"
        "49=A\001"
        "10=123\001";

    const auto result = ingestor.feed(as_bytes(invalid));

    assert(result.status == fix::IngestionStatus::InvalidMessage);
    assert(result.failed());
}

void test_missing_checksum() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view incomplete =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0\001"
        "49=A\001";

    const auto result = ingestor.feed(as_bytes(incomplete));

    assert(result.status == fix::IngestionStatus::NeedMoreData);
    assert(result.need_more_data());
}

void test_checksum_without_delimiter() {
    fix::FixIngestor<> ingestor;

    constexpr std::string_view incomplete =
        "8=FIX.4.2\001"
        "9=10\001"
        "35=0\001"
        "49=A\001"
        "10=123";

    const auto result = ingestor.feed(as_bytes(incomplete));

    assert(result.status == fix::IngestionStatus::NeedMoreData);
    assert(result.need_more_data());
}

void test_buffer_full() {
    fix::FixIngestor<16> ingestor;

    constexpr std::array<std::byte, 17> input{};

    const auto result = ingestor.feed(input);

    assert(result.status == fix::IngestionStatus::BufferFull);
    assert(result.failed());
    assert(result.message.empty());
    assert(result.consumed == 0);

    assert(ingestor.buffered_bytes() == 0);
    assert(ingestor.available_bytes() == 16);
}

void test_exact_buffer_capacity() {
    fix::FixIngestor<16> ingestor;

    constexpr std::array<std::byte, 16> input{};

    const auto result = ingestor.feed(input);

    assert(ingestor.buffered_bytes() == 16);
    assert(ingestor.available_bytes() == 0);

    assert(
        result.status == fix::IngestionStatus::InvalidMessage ||
        result.status == fix::IngestionStatus::NeedMoreData
    );
}

void test_feed_after_consume() {
    fix::FixIngestor<> ingestor;

    const auto first = ingestor.feed(as_bytes(message1));

    assert(first.ready());

    ingestor.consume(first.consumed);

    assert(ingestor.buffered_bytes() == 0);

    const auto second = ingestor.feed(as_bytes(message2));

    assert(second.ready());
    assert(second.message == message2);
}

} // namespace

int main() {
    test_empty_buffer();

    test_complete_message();
    test_incomplete_message();
    test_fragmented_message();

    test_split_inside_begin_string();
    test_split_inside_begin_string_delimiter();

    test_split_inside_body_length();
    test_split_inside_body_length_delimiter();

    test_split_inside_body();

    test_split_inside_checksum();
    test_split_inside_checksum_value();

    test_multiple_messages();

    test_consume_partial_buffer();
    test_consume_more_than_buffered();
    test_clear();

    test_invalid_begin_string();
    test_invalid_body_length_tag();
    test_invalid_body_length_value();
    test_empty_body_length();
    test_body_length_points_to_wrong_field();

    test_missing_checksum();
    test_checksum_without_delimiter();

    test_buffer_full();
    test_exact_buffer_capacity();

    test_feed_after_consume();

    return 0;
}