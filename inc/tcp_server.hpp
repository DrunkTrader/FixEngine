#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include "storage.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include "validator.hpp"
#include "formatter.hpp"

namespace fix {

    struct TcpConfig {
        std::string host = "0.0.0.0";
        int port = 9876;
        int max_connections = 10;
        int buffer_size = 4096;
        int read_timeout_ms = 5000;
    };

    struct MessageCallback {
        std::function<void(const std::string& raw_message)> on_raw_message;
        std::function<void(const FixMessage& parsed_message)> on_parsed_message;
        std::function<void(const ParseResult& validated_result)> on_validated_result;
        std::function<void(const std::string& error)> on_error;
    };

    class TcpServer {
    public:
        explicit TcpServer(const TcpConfig& config = TcpConfig());
        ~TcpServer();

        // Set callbacks for message processing
        void setCallbacks(const MessageCallback& callbacks);

        // Start/stop server
        bool start();
        void stop();
        bool isRunning() const;

        // Get statistics
        struct Stats {
            std::atomic<uint64_t> messages_received{0};
            std::atomic<uint64_t> messages_parsed{0};
            std::atomic<uint64_t> messages_validated{0};
            std::atomic<uint64_t> validation_failures{0};
            std::atomic<uint64_t> bytes_received{0};
            std::atomic<uint64_t> connections{0};
        };
        
        const Stats& getStats() const { return stats_; }

    private:
        void serverLoop();
        void handleClient(int client_socket);
        std::string readMessage(int client_socket);
        void processMessage(const std::string& raw_message);

        TcpConfig config_;
        MessageCallback callbacks_;
        std::atomic<bool> running_{false};
        std::thread server_thread_;
        int server_socket_ = -1;
        Stats stats_;
        fix::Parser parser_;
        fix::Validator validator_;
        fix::Formatter formatter_;
    };

}
