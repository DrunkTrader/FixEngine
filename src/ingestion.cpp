#include "ingestion.hpp"
#include "storage.hpp"

#include <limits>

namespace fix {

namespace {

[[nodiscard]] constexpr bool is_digit(char ch) noexcept {
    return ch >= '0' && ch <= '9';
}

[[nodiscard]] constexpr bool parse_number(
    std::string_view text,
    std::size_t& value) noexcept {

    if (text.empty()) {
        return false;
    }

    std::size_t result = 0;

    for (const char ch : text) {

        if (!is_digit(ch)) {
            return false;
        }

        const std::size_t digit =
            static_cast<std::size_t>(ch - '0');

        if (result >
            (std::numeric_limits<std::size_t>::max() - digit) / 10U) {
            return false;
        }

        result = result * 10U + digit;
    }

    value = result;
    return true;
}

[[nodiscard]] std::string_view as_string_view(
    std::span<const std::byte> bytes) noexcept {

    return {
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size()
    };
}

} // namespace

template <std::size_t Capacity>
IngestionResult FixIngestor<Capacity>::extract() noexcept {

    const auto bytes = buffer_.data();
    const std::string_view input = as_string_view(bytes);

    if (input.empty()) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // FIX messages must begin with BeginString (8=...)
    // ------------------------------------------------------------

    if (input.size() < 2) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    if (input[0] != '8' || input[1] != '=') {
        return {
            IngestionStatus::InvalidMessage,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // Find the end of BeginString.
    //
    // Example:
    //
    // 8=FIX.4.2<SOH>
    //              ^
    // ------------------------------------------------------------

    const std::size_t begin_end =
        input.find(soh_del, 2);

    if (begin_end == std::string_view::npos) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    const std::size_t body_length_tag_start =
        begin_end + 1;

    // We need at least "9=".
    if (input.size() < body_length_tag_start + 2) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    if (input[body_length_tag_start] != '9' ||
        input[body_length_tag_start + 1] != '=') {

        return {
            IngestionStatus::InvalidMessage,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // Find the SOH terminating BodyLength.
    //
    // 9=118<SOH>
    //       ^
    // ------------------------------------------------------------

    const std::size_t body_length_end =
        input.find(
            soh_del,
            body_length_tag_start + 2
        );

    if (body_length_end == std::string_view::npos) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    const std::string_view body_length_text =
        input.substr(
            body_length_tag_start + 2,
            body_length_end -
                (body_length_tag_start + 2)
        );

    std::size_t body_length = 0;

    if (!parse_number(body_length_text, body_length)) {
        return {
            IngestionStatus::InvalidMessage,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // BodyLength starts immediately after the SOH following tag 9.
    //
    // Example:
    //
    // 8=FIX.4.2<SOH>
    // 9=118<SOH>
    //             ^
    //             body starts here
    // ------------------------------------------------------------

    const std::size_t body_start =
        body_length_end + 1;

    // Prevent integer overflow.
    if (body_length >
        input.size() - body_start) {

        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    const std::size_t checksum_start =
        body_start + body_length;

    // We know where tag 10 must begin, but the complete
    // checksum field may not have arrived yet.
    if (input.size() < checksum_start + 3) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // BodyLength should place us exactly at:
    //
    // 10=...
    // ^^
    // ------------------------------------------------------------

    if (input[checksum_start] != '1' ||
        input[checksum_start + 1] != '0' ||
        input[checksum_start + 2] != '=') {

        return {
            IngestionStatus::InvalidMessage,
            {},
            0
        };
    }

    // ------------------------------------------------------------
    // Find checksum delimiter.
    //
    // 10=000<SOH>
    //        ^
    // ------------------------------------------------------------

    const std::size_t message_end =
        input.find(
            soh_del,
            checksum_start + 3
        );

    if (message_end == std::string_view::npos) {
        return {
            IngestionStatus::NeedMoreData,
            {},
            0
        };
    }

    const std::size_t message_size =
        message_end + 1;

    return {
        IngestionStatus::MessageReady,
        input.substr(0, message_size),
        message_size
    };
}

// Explicit template instantiation for the default buffer.
template class FixIngestor<16>;
template class FixIngestor<64 * 1024>;

} // namespace fix