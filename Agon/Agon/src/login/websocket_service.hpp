#pragma once

#include <string>
#include <functional>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

class WebSocketService {
public:
    // Singleton pattern
    static WebSocketService& getInstance();
    
    // Connect to a WebSocket server
    bool connect(const std::string& url, const std::string& token = "", const std::string& roomId = "");
    
    // Send a message
    void send(const std::string& message);
    
    // Register message handler
    void registerMessageHandler(std::function<void(const std::string&)> handler);
    
    // Disconnect from server
    void disconnect();
    
    // Check if connected
    bool isConnected() const;
    
    
private:
    WebSocketService();
    ~WebSocketService();
    
    // Handle incoming messages
    void readLoop();
    
    // Beast objects
    std::unique_ptr<boost::asio::io_context> ioc_;
    std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
    std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws_;
    
    // Threading
    std::thread io_thread_;
    std::thread read_thread_;
    std::atomic<bool> connected_ {false};
    
    // Message handling
    std::function<void(const std::string&)> message_handler_;
    std::mutex mutex_;
};