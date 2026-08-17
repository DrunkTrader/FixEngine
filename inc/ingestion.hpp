#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace fix {

enum class IngestionStatus : std::uint8_t {
    NeedMoreData,
    MessageReady,
    BufferFull,
    InvalidMessage
};

struct IngestionResult {
    IngestionStatus status{IngestionStatus::NeedMoreData};
    std::string_view message{};
    std::size_t consumed{0};

    [[nodiscard]] constexpr bool ready() const noexcept {
        return status == IngestionStatus::MessageReady;
    }

    [[nodiscard]] constexpr bool need_more_data() const noexcept {
        return status == IngestionStatus::NeedMoreData;
    }

    [[nodiscard]] constexpr bool failed() const noexcept {
        return status == IngestionStatus::BufferFull ||
               status == IngestionStatus::InvalidMessage;
    }
};

template <std::size_t Capacity = 64 * 1024>
class IngestionBuffer {
    static_assert(Capacity > 0);

public:
    using Storage = std::array<std::byte, Capacity>;

    constexpr IngestionBuffer() noexcept = default;

    IngestionBuffer(const IngestionBuffer&) = delete;
    IngestionBuffer& operator=(const IngestionBuffer&) = delete;

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return Capacity;
    }

    [[nodiscard]] constexpr std::size_t available() const noexcept {
        return Capacity - size_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] constexpr bool full() const noexcept {
        return size_ == Capacity;
    }

    [[nodiscard]] std::span<const std::byte> data() const noexcept {
        return {
            storage_.data(),
            size_
        };
    }

    [[nodiscard]] std::span<std::byte> writable() noexcept {
        return {
            storage_.data() + size_,
            Capacity - size_
        };
    }

    constexpr bool commit(std::size_t bytes) noexcept {
        if (bytes > available()) {
            return false;
        }

        size_ += bytes;
        return true;
    }

    constexpr void consume(std::size_t bytes) noexcept {
        if (bytes >= size_) {
            size_ = 0;
            return;
        }

        const std::size_t remaining = size_ - bytes;

        for (std::size_t i = 0; i < remaining; ++i) {
            storage_[i] = storage_[bytes + i];
        }

        size_ = remaining;
    }

    constexpr void clear() noexcept {
        size_ = 0;
    }

private:
    Storage storage_{};
    std::size_t size_{0};
};

template <std::size_t Capacity = 64 * 1024>
class FixIngestor {
public:
    [[nodiscard]] IngestionResult feed(
        std::span<const std::byte> bytes) noexcept {

        if (bytes.size() > buffer_.available()) {
            return {
                IngestionStatus::BufferFull,
                {},
                0
            };
        }

        auto destination = buffer_.writable();

        for (std::size_t i = 0; i < bytes.size(); ++i) {
            destination[i] = bytes[i];
        }

        buffer_.commit(bytes.size());

        return extract();
    }

    [[nodiscard]] IngestionResult extract() noexcept;

    [[nodiscard]] constexpr std::size_t buffered_bytes() const noexcept {
        return buffer_.size();
    }

    [[nodiscard]] constexpr std::size_t available_bytes() const noexcept {
        return buffer_.available();
    }

    constexpr void consume(std::size_t bytes) noexcept {
        buffer_.consume(bytes);
    }

    constexpr void clear() noexcept {
        buffer_.clear();
    }

private:
    IngestionBuffer<Capacity> buffer_{};
};

} // namespace fix