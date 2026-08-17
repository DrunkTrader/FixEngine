#include "fix_engine.hpp"

namespace fix {

EngineResult FixEngine::feed(
    std::span<const std::byte> bytes) noexcept {

    const auto ingestion = ingestor_.feed(bytes);

    if (!ingestion.ready()) {
        return {
            .ingestion = ingestion
        };
    }

    return decode(ingestion);
}

EngineResult FixEngine::process() noexcept {

    const auto ingestion = ingestor_.extract();

    if (!ingestion.ready()) {
        return {
            .ingestion = ingestion
        };
    }

    return decode(ingestion);
}

EngineResult FixEngine::decode(
    const IngestionResult& ingestion) noexcept {

    EngineResult result{
        .ingestion = ingestion
    };

    result.parsed = parser_.parse(ingestion.message);

    if (!result.parsed.ok()) {
        return result;
    }

    result.validation =
        validator_.validate(
            ingestion.message,
            result.parsed.message
        );

    current_message_size_ = ingestion.consumed;

    return result;
}

} // namespace fix