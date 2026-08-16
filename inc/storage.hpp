#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fix {

using Tag = std::uint16_t;
using FieldLength = std::uint16_t;

inline constexpr char soh_del = '\x01';

struct FixField {
    Tag tag{0};
    std::string_view value{};

    [[nodiscard]] constexpr bool empty() const noexcept {
        return value.empty();
    }
};

template <std::size_t MaxFields = 64>
struct FixMessage {
    std::array<FixField, MaxFields> fields{};
    std::uint16_t count{0};

    [[nodiscard]] constexpr bool empty() const noexcept {
        return count == 0;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return count;
    }

    [[nodiscard]] constexpr const FixField* begin() const noexcept {
        return fields.data();
    }

    [[nodiscard]] constexpr const FixField* end() const noexcept {
        return fields.data() + count;
    }

    [[nodiscard]] constexpr const FixField& operator[](
        std::size_t index) const noexcept {
        return fields[index];
    }

    [[nodiscard]] constexpr const FixField* find(Tag tag) const noexcept {
        for (std::size_t i = 0; i < count; ++i) {
            if (fields[i].tag == tag) {
                return &fields[i];
            }
        }

        return nullptr;
    }

    [[nodiscard]] constexpr std::string_view get(
        Tag tag,
        std::string_view fallback = {}) const noexcept {
        const auto* field = find(tag);
        return field != nullptr ? field->value : fallback;
    }

    [[nodiscard]] constexpr bool has(Tag tag) const noexcept {
        return find(tag) != nullptr;
    }

    constexpr bool push(Tag tag, std::string_view value) noexcept {
        if (count >= MaxFields) {
            return false;
        }

        fields[count++] = FixField{
            .tag = tag,
            .value = value
        };

        return true;
    }
};

struct ParseError {
    enum class Code : std::uint8_t {
        None,
        TooManyFields,
        InvalidTag,
        EmptyTag,
        MissingDelimiter
    };

    Code code{Code::None};
    std::size_t position{0};

    [[nodiscard]] constexpr bool ok() const noexcept {
        return code == Code::None;
    }
};

} // namespace fix