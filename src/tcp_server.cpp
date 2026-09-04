#include "tcp_server.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sstream>

namespace fix {

    TcpServer::TcpServer(const TcpConfig& config) : config_(config) {}

    TcpServer::~TcpServer() {
        stop();
    }

    void TcpServer::setCallbacks(const MessageCallback& callbacks) {
        callbacks_ = callbacks;
    }

    bool TcpServer::start() {
        if (running_) {
            return false;
        }

        // Create socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            if (callbacks_.on_error) {
                callbacks_.on_error("Failed to create server socket: " + std::string(strerror(errno)));
            }
            return false;
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            if (callbacks_.on_error) {
                callbacks_.on_error("Failed to set socket options: " + std::string(strerror(errno)));
            }
            close(server_socket_);
            return false;
        }

        // Bind socket
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config_.port);
        
        if (inet_pton(AF_INET, config_.host.c_str(), &server_addr.sin_addr) <= 0) {
            if (callbacks_.on_error) {
                callbacks_.on_error("Invalid host address: " + config_.host);
            }
            close(server_socket_);
            return false;
        }

        if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            if (callbacks_.on_error) {
                callbacks_.on_error("Failed to bind socket: " + std::string(strerror(errno)));
            }
            close(server_socket_);
            return false;
        }

        // Listen for connections
        if (listen(server_socket_, config_.max_connections) < 0) {
            if (callbacks_.on_error) {
                callbacks_.on_error("Failed to listen: " + std::string(strerror(errno)));
            }
            close(server_socket_);
            return false;
        }

        running_ = true;
        server_thread_ = std::thread(&TcpServer::serverLoop, this);
        
        std::cout << "FIX TCP Server started on " << config_.host << ":" << config_.port << std::endl;
        return true;
    }

    void TcpServer::stop() {
        if (!running_) {
            return;
        }

        running_ = false;
        
        if (server_socket_ >= 0) {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
            server_socket_ = -1;
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        std::cout << "FIX TCP Server stopped." << std::endl;
    }

    bool TcpServer::isRunning() const {
        return running_;
    }

    void TcpServer::serverLoop() {
        while (running_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_socket_, &read_fds);

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int activity = select(server_socket_ + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (activity < 0) {
                if (errno == EINTR) continue;
                if (callbacks_.on_error) {
                    callbacks_.on_error("Select error: " + std::string(strerror(errno)));
                }
                break;
            }

            if (activity > 0 && FD_ISSET(server_socket_, &read_fds)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
                if (client_socket < 0) {
                    if (callbacks_.on_error) {
                        callbacks_.on_error("Accept failed: " + std::string(strerror(errno)));
                    }
                    continue;
                }

                stats_.connections++;
                std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) 
                          << ":" << ntohs(client_addr.sin_port) << std::endl;

                // Handle client in a new thread
                std::thread(&TcpServer::handleClient, this, client_socket).detach();
            }
        }
    }

    void TcpServer::handleClient(int client_socket) {
        std::string buffer;
        char read_buffer[4096];

        while (running_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(client_socket, &read_fds);

            struct timeval timeout;
            timeout.tv_sec = config_.read_timeout_ms / 1000;
            timeout.tv_usec = (config_.read_timeout_ms % 1000) * 1000;

            int activity = select(client_socket + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (activity < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (activity == 0) {
                // Timeout
                continue;
            }

            ssize_t bytes_read = recv(client_socket, read_buffer, sizeof(read_buffer) - 1, 0);
            
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cout << "Client disconnected" << std::endl;
                } else if (callbacks_.on_error) {
                    callbacks_.on_error("Recv error: " + std::string(strerror(errno)));
                }
                break;
            }

            stats_.bytes_received += bytes_read;
            read_buffer[bytes_read] = '\0';
            buffer.append(read_buffer, bytes_read);

            // Process complete messages (delimited by SOH or newline)
            std::size_t pos = 0;
            while ((pos = buffer.find('\x01')) != std::string::npos || 
                   (pos = buffer.find('\n')) != std::string::npos) {
                
                std::string message = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);

                if (!message.empty()) {
                    processMessage(message);
                }
            }
        }

        close(client_socket);
    }

    void TcpServer::processMessage(const std::string& raw_message) {
        stats_.messages_received++;

        // Call raw message callback
        if (callbacks_.on_raw_message) {
            callbacks_.on_raw_message(raw_message);
        }

        // Parse and validate with duplicate detection
        ParseResult result = validator_.validateWithParse(raw_message);
        stats_.messages_parsed++;
        stats_.messages_validated++;

        if (!result.validation.ok) {
            stats_.validation_failures++;
        }

        // Call parsed message callback
        if (callbacks_.on_parsed_message) {
            callbacks_.on_parsed_message(result.message);
        }

        // Call validated result callback
        if (callbacks_.on_validated_result) {
            callbacks_.on_validated_result(result);
        }

        // Print formatted output
        std::cout << "\n=== Received FIX Message ===" << std::endl;
        auto formatted = formatter_.format(result.message);
        std::cout << formatted.summary;
        std::cout << formatter_.formatValidationResult(result.validation, raw_message);
        
        if (result.has_duplicates) {
            std::cout << "Warning: Duplicate tags detected: ";
            for (int tag : result.duplicate_tags) {
                std::cout << tag << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "============================\n" << std::endl;
    }

}
