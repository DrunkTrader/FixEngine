#include <iostream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include "inc/parser.hpp"
#include "inc/validator.hpp"
#include "inc/formatter.hpp"
#include "inc/tcp_server.hpp"

void demonstrateEnhancedFeatures() {
    std::cout << "=== Enhanced FIX Parser Demo ===\n\n";

    fix::Formatter formatter;
    fix::Validator validator;

    // Test 1: Valid message with correct body length and checksum
    std::cout << "Test 1: Valid FIX Message\n";
    std::cout << "--------------------------\n";
    // Body: from tag 35 to before tag 10 (SOH delimited, excluding final delimiter)
    // 35=D\x0149=SENDER\x0156=TARGET\x0134=2\x0152=20240528-09:20:52\x0111=ORDERID\x0155=MSFT\x0154=1\x0138=1000\x0140=2\x0144=150.5
    // Full msg up to 10=: 8=FIX.4.2\x019=96\x0135=D\x0149=SENDER\x0156=TARGET\x0134=2\x0152=20240528-09:20:52\x0111=ORDERID\x0155=MSFT\x0154=1\x0138=1000\x0140=2\x0144=150.5\x0110=
    // Length = 96 bytes, Checksum = 143
    std::string valid_msg = "8=FIX.4.2|9=96|35=D|49=SENDER|56=TARGET|34=2|52=20240528-09:20:52|11=ORDERID|55=MSFT|54=1|38=1000|40=2|44=150.5|10=143|";
    
    auto parse_result = validator.validateWithParse(valid_msg);
    
    auto formatted = formatter.format(parse_result.message);
    std::cout << formatted.summary;
    std::cout << formatter.formatDetailed(parse_result.message);
    std::cout << formatter.formatValidationResult(parse_result.validation, valid_msg);
    std::cout << "\n";

    // Test 2: Message with duplicate tags
    std::cout << "Test 2: Message with Duplicate Tags\n";
    std::cout << "------------------------------------\n";
    std::string duplicate_msg = "8=FIX.4.2|9=95|35=D|49=SENDER|56=TARGET|34=2|52=20240528-09:20:52|11=ORDERID|55=MSFT|55=AAPL|54=1|38=1000|40=2|10=058|";
    
    auto dup_result = validator.validateWithParse(duplicate_msg);
    std::cout << formatter.formatValidationResult(dup_result.validation, duplicate_msg);
    if (dup_result.has_duplicates) {
        std::cout << "Duplicate tags found: ";
        for (int tag : dup_result.duplicate_tags) {
            std::cout << tag << " ";
        }
        std::cout << "\n\n";
    }

    // Test 3: Message with incorrect body length
    std::cout << "Test 3: Incorrect Body Length\n";
    std::cout << "------------------------------\n";
    std::string bad_body_msg = "8=FIX.4.2|9=999|35=D|49=SENDER|56=TARGET|34=2|52=20240528-09:20:52|11=ORDERID|55=MSFT|54=1|38=1000|40=2|44=150.5|10=045|";
    
    auto body_result = validator.validateWithParse(bad_body_msg);
    std::cout << formatter.formatValidationResult(body_result.validation, bad_body_msg);
    std::cout << "\n";
}

void runTcpServer() {
    std::cout << "=== Starting FIX TCP Server ===\n";
    std::cout << "Server will listen on port 9876\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    fix::TcpConfig config;
    config.host = "0.0.0.0";
    config.port = 9876;
    config.max_connections = 10;

    fix::TcpServer server(config);

    fix::MessageCallback callbacks;
    callbacks.on_error = [](const std::string& error) {
        std::cerr << "[ERROR] " << error << std::endl;
    };

    callbacks.on_validated_result = [](const fix::ParseResult& result) {
        if (!result.validation.ok) {
            std::cout << "[VALIDATION FAILED] " << result.validation.errors.size() << " error(s)\n";
        }
    };

    server.setCallbacks(callbacks);

    if (!server.start()) {
        std::cerr << "Failed to start server\n";
        return;
    }

    // Run server until interrupted
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Print stats every 10 seconds
        static int counter = 0;
        if (++counter % 10 == 0) {
            const auto& stats = server.getStats();
            std::cout << "\n[Stats] Messages: " << stats.messages_received 
                      << " | Validated: " << stats.messages_validated
                      << " | Failures: " << stats.validation_failures
                      << " | Connections: " << stats.connections << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--tcp") {
        runTcpServer();
    } else {
        demonstrateEnhancedFeatures();
        
        std::cout << "\n=== Usage ===\n";
        std::cout << "Run without arguments: Demo mode (shows enhanced features)\n";
        std::cout << "Run with --tcp argument: Start TCP server on port 9876\n";
        std::cout << "\nExample TCP client test:\n";
        std::cout << "  echo '8=FIX.4.2|9=118|35=D|49=SENDER|56=TARGET|34=2|52=20240528-09:20:52|11=ORDERID|55=MSFT|54=1|38=1000|40=2|44=150.5|10=000|' | nc localhost 9876\n";
    }

    return 0;
}