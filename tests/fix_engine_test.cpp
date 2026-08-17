#include "fix_engine.hpp"

#include <cassert>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace {

[[nodiscard]]
std::span<const std::byte> as_bytes(
    std::string_view value) noexcept {

    return std::as_bytes(
        std::span<const char>(
            value.data(),
            value.size()
        )
    );
}

// -----------------------------------------------------------------------------
// Valid FIX messages
// -----------------------------------------------------------------------------

constexpr std::string_view message1 =
    "8=FIX.4.2\x01"
    "9=97\x01"
    "35=D\x01"
    "49=SENDER\x01"
    "56=TARGET\x01"
    "34=1\x01"
    "52=20240528-09:20:52\x01"
    "11=ORDERID\x01"
    "55=MSFT\x01"
    "54=1\x01"
    "38=1000\x01"
    "40=2\x01"
    "44=150.5\x01"
    "10=241\x01";

constexpr std::string_view message2 =
    "8=FIX.4.2\x01"
    "9=97\x01"
    "35=D\x01"
    "49=SENDER\x01"
    "56=TARGET\x01"
    "34=2\x01"
    "52=20240528-09:21:00\x01"
    "11=ORDER002\x01"
    "55=AAPL\x01"
    "54=1\x01"
    "38=500\x01"
    "40=2\x01"
    "44=200.5\x01"
    "10=165\x01";

// -----------------------------------------------------------------------------
// Complete message
// -----------------------------------------------------------------------------

void test_complete_message() {

    fix::FixEngine engine;

    const auto result =
        engine.feed(as_bytes(message1));

    assert(result.ready());
    assert(result.parsed_ok());
    assert(result.valid());

    const auto& message =
        result.parsed.message;

    assert(message.has(8));
    assert(message.has(9));
    assert(message.has(35));
    assert(message.has(49));
    assert(message.has(56));
    assert(message.has(34));
    assert(message.has(52));
    assert(message.has(11));
    assert(message.has(55));
    assert(message.has(54));
    assert(message.has(38));
    assert(message.has(40));
    assert(message.has(44));
    assert(message.has(10));

    assert(message.get(8) == "FIX.4.2");
    assert(message.get(9) == "97");
    assert(message.get(35) == "D");
    assert(message.get(49) == "SENDER");
    assert(message.get(56) == "TARGET");
    assert(message.get(34) == "1");
    assert(message.get(52) == "20240528-09:20:52");
    assert(message.get(11) == "ORDERID");
    assert(message.get(55) == "MSFT");
    assert(message.get(54) == "1");
    assert(message.get(38) == "1000");
    assert(message.get(40) == "2");
    assert(message.get(44) == "150.5");
    assert(message.get(10) == "241");

    assert(
        result.ingestion.consumed ==
        message1.size()
    );

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Partial message
// -----------------------------------------------------------------------------

void test_message_split_across_feeds() {

    fix::FixEngine engine;

    constexpr std::size_t split =
        message1.size() / 2;

    const auto first =
        message1.substr(0, split);

    const auto second =
        message1.substr(split);

    auto result =
        engine.feed(as_bytes(first));

    assert(result.need_more_data());
    assert(!result.ready());
    assert(!result.failed());

    assert(
        engine.buffered_bytes() ==
        first.size()
    );

    result =
        engine.feed(as_bytes(second));

    assert(result.ready());
    assert(result.parsed_ok());
    assert(result.valid());

    assert(result.parsed.message.get(35) == "D");
    assert(result.parsed.message.get(55) == "MSFT");

    assert(
        result.ingestion.consumed ==
        message1.size()
    );

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Every possible split position
//
// This simulates TCP delivering a FIX message in arbitrary chunks.
// -----------------------------------------------------------------------------

void test_message_split_at_every_position() {

    for (std::size_t split = 1;
         split < message1.size();
         ++split) {

        fix::FixEngine engine;

        const auto first =
            message1.substr(0, split);

        const auto second =
            message1.substr(split);

        auto result =
            engine.feed(as_bytes(first));

        assert(result.need_more_data());

        result =
            engine.feed(as_bytes(second));

        assert(result.ready());
        assert(result.parsed_ok());
        assert(result.valid());

        assert(
            result.parsed.message.get(35) ==
            "D"
        );

        assert(
            result.parsed.message.get(55) ==
            "MSFT"
        );

        engine.consume();

        assert(engine.buffered_bytes() == 0);
    }
}

// -----------------------------------------------------------------------------
// Multiple messages in one input buffer
// -----------------------------------------------------------------------------

void test_multiple_messages() {

    fix::FixEngine engine;

    std::string input;

    input.reserve(
        message1.size() +
        message2.size()
    );

    input.append(message1);
    input.append(message2);

    // First message should be extracted.
    auto result =
        engine.feed(as_bytes(input));

    assert(result.ready());
    assert(result.parsed_ok());
    assert(result.valid());

    assert(
        result.parsed.message.get(35) ==
        "D"
    );

    assert(
        result.parsed.message.get(55) ==
        "MSFT"
    );

    assert(
        result.parsed.message.get(34) ==
        "1"
    );

    assert(
        result.ingestion.consumed ==
        message1.size()
    );

    // Second message must remain buffered.
    assert(
        engine.buffered_bytes() ==
        message2.size()
    );

    engine.consume();

    assert(
        engine.buffered_bytes() ==
        message2.size()
    );

    // Process the second buffered message.
    result = engine.process();

    assert(result.ready());
    assert(result.parsed_ok());
    assert(result.valid());

    assert(
        result.parsed.message.get(35) ==
        "D"
    );

    assert(
        result.parsed.message.get(55) ==
        "AAPL"
    );

    assert(
        result.parsed.message.get(34) ==
        "2"
    );

    assert(
        result.parsed.message.get(10) ==
        "165"
    );

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Invalid BeginString
// -----------------------------------------------------------------------------

void test_invalid_begin_string() {

    fix::FixEngine engine;

    constexpr std::string_view input =
        "9=97\x01"
        "35=D\x01";

    const auto result =
        engine.feed(as_bytes(input));

    assert(result.failed());

    assert(
        result.ingestion.status ==
        fix::IngestionStatus::InvalidMessage
    );

    assert(!result.ready());
}

// -----------------------------------------------------------------------------
// Incomplete message
// -----------------------------------------------------------------------------

void test_incomplete_message() {

    fix::FixEngine engine;

    constexpr std::string_view input =
        "8=FIX.4.2\x01"
        "9=97\x01"
        "35=D\x01";

    const auto result =
        engine.feed(as_bytes(input));

    assert(result.need_more_data());
    assert(!result.ready());

    assert(
        engine.buffered_bytes() ==
        input.size()
    );
}

// -----------------------------------------------------------------------------
// Invalid checksum
// -----------------------------------------------------------------------------

void test_invalid_checksum() {

    fix::FixEngine engine;

    std::string input(message1);

    const auto checksum_position =
        input.rfind("10=");

    assert(
        checksum_position !=
        std::string::npos
    );

    // Change 241 -> 999.
    input[checksum_position + 3] = '9';
    input[checksum_position + 4] = '9';
    input[checksum_position + 5] = '9';

    const auto result =
        engine.feed(as_bytes(input));

    // Framing should still succeed.
    assert(result.ready());

    // Parsing should still succeed.
    assert(result.parsed_ok());

    // Validation must reject the checksum.
    assert(!result.valid());

    assert(result.validation.size() > 0);

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Unknown message type
// -----------------------------------------------------------------------------

void test_unknown_message_type() {

    fix::FixEngine engine;

    std::string input(message1);

    const auto message_type_position =
        input.find("35=D");

    assert(
        message_type_position !=
        std::string::npos
    );

    // Change D -> Z.
    input[message_type_position + 3] = 'Z';

    /*
     * The checksum becomes invalid because we changed the
     * message contents. That is acceptable here.
     *
     * We are specifically checking that the validator
     * rejects the unknown message type.
     */
    const auto result =
        engine.feed(as_bytes(input));

    assert(result.ready());
    assert(result.parsed_ok());
    assert(!result.valid());

    assert(result.validation.size() > 0);

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Clear buffered data
// -----------------------------------------------------------------------------

void test_clear() {

    fix::FixEngine engine;

    constexpr std::string_view input =
        "8=FIX.4.2\x01"
        "9=97\x01";

    const auto result =
        engine.feed(as_bytes(input));

    assert(result.need_more_data());

    assert(
        engine.buffered_bytes() ==
        input.size()
    );

    engine.clear();

    assert(engine.buffered_bytes() == 0);

    assert(
        engine.available_bytes() ==
        64 * 1024
    );
}

// -----------------------------------------------------------------------------
// Empty feed
// -----------------------------------------------------------------------------

void test_empty_feed() {

    fix::FixEngine engine;

    const auto result =
        engine.feed(
            std::span<const std::byte>{}
        );

    assert(result.need_more_data());
    assert(!result.ready());
    assert(!result.failed());

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// process() on buffered data
// -----------------------------------------------------------------------------

void test_process_existing_buffer() {

    fix::FixEngine engine;

    constexpr std::size_t split = 20;

    const auto first =
        message1.substr(0, split);

    const auto second =
        message1.substr(split);

    auto result =
        engine.feed(as_bytes(first));

    assert(result.need_more_data());

    assert(
        engine.buffered_bytes() ==
        first.size()
    );

    result =
        engine.feed(as_bytes(second));

    assert(result.ready());
    assert(result.parsed_ok());
    assert(result.valid());

    assert(
        result.parsed.message.get(55) ==
        "MSFT"
    );

    engine.consume();

    assert(engine.buffered_bytes() == 0);
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

} // namespace

int main() {

    test_complete_message();

    test_message_split_across_feeds();

    test_message_split_at_every_position();

    test_multiple_messages();

    test_invalid_begin_string();

    test_incomplete_message();

    test_invalid_checksum();

    test_unknown_message_type();

    test_clear();

    test_empty_feed();

    test_process_existing_buffer();

    return 0;
}