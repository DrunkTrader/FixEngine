#pragma once

#include <cstddef>
#include <span>

#include "ingestion.hpp"
#include "parser.hpp"
#include "validator.hpp"

namespace fix {

struct EngineResult {
    IngestionResult ingestion{};
    TokenizerResult parsed{};
    ValidationResult<Validator::max_errors> validation{};

    [[nodiscard]] constexpr bool ready() const noexcept {
        return ingestion.ready();
    }

    [[nodiscard]] constexpr bool parsed_ok() const noexcept {
        return ready() && parsed.ok();
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return parsed_ok() && validation.ok();
    }

    [[nodiscard]] constexpr bool need_more_data() const noexcept {
        return ingestion.need_more_data();
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return ingestion.failed();
    }
};

class FixEngine {
public:
    FixEngine() = default;

    FixEngine(const FixEngine&) = delete;
    FixEngine& operator=(const FixEngine&) = delete;

    FixEngine(FixEngine&&) = delete;
    FixEngine& operator=(FixEngine&&) = delete;

    [[nodiscard]]
    EngineResult feed(
        std::span<const std::byte> bytes) noexcept;

    [[nodiscard]]
    EngineResult process() noexcept;

    constexpr void consume() noexcept {
        if (current_message_size_ != 0) {
            ingestor_.consume(current_message_size_);
            current_message_size_ = 0;
        }
    }

    constexpr void clear() noexcept {
        ingestor_.clear();
        current_message_size_ = 0;
    }

    [[nodiscard]]
    constexpr std::size_t buffered_bytes() const noexcept {
        return ingestor_.buffered_bytes();
    }

    [[nodiscard]]
    constexpr std::size_t available_bytes() const noexcept {
        return ingestor_.available_bytes();
    }

private:
    [[nodiscard]]
    EngineResult decode(
        const IngestionResult& ingestion) noexcept;

    FixIngestor<> ingestor_{};
    Parser parser_{};
    Validator validator_{};

    std::size_t current_message_size_{0};
};

} // namespace fix