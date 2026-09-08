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

    // Convert string_view to string for parser
    std::string message_str{ingestion.message};
    result.message = parser_.parse(message_str);

    if (result.message.empty()) {
        return result;
    }

    result.validation =
        validator_.validate(
            message_str,
            result.message
        );

    current_message_size_ = ingestion.consumed;

    return result;
}

} // namespace fix