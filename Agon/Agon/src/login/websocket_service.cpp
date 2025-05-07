#include "websocket_service.hpp"
#include <iostream>
#include <regex>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

WebSocketService& WebSocketService::getInstance() {
    static WebSocketService instance;
    return instance;
}

WebSocketService::WebSocketService() {
    // Initialize IO context
    ioc_ = std::make_unique<net::io_context>();
}

WebSocketService::~WebSocketService() {
    disconnect();
    
    // Join threads if still running
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
}

bool WebSocketService::connect(const std::string& url, const std::string& token, const std::string& roomId) {
    if (connected_) {
        std::cout << "Already connected to WebSocket server\n";
        return true;
    }
    
    try {
        // Parse URL
        std::string host;
        std::string port;
        std::string target;
        
        // Simple URL parsing
        std::string uri = url;
        
        // Remove ws:// prefix if present
        if (uri.substr(0, 5) == "ws://") {
            uri = uri.substr(5);
        }
        
        // Find host/port/path
        size_t path_pos = uri.find('/');
        if (path_pos != std::string::npos) {
            std::string host_port = uri.substr(0, path_pos);
            target = uri.substr(path_pos);
            
            // Split host:port
            size_t colon_pos = host_port.find(':');
            if (colon_pos != std::string::npos) {
                host = host_port.substr(0, colon_pos);
                port = host_port.substr(colon_pos + 1);
            } else {
                host = host_port;
                port = "80"; // Default WebSocket port
            }
        } else {
            host = uri;
            target = "/";
            port = "80";
        }
        
        // Add query parameters
        target += (target.find('?') == std::string::npos) ? "?" : "&";
        target += "token=" + token;
        
        if (!roomId.empty()) {
            target += "&roomId=" + roomId;
        }
        
        std::cout << "Connecting to " << host << ":" << port << target << std::endl;
        
        // Create fresh objects for the connection
        ioc_ = std::make_unique<net::io_context>();
        resolver_ = std::make_unique<tcp::resolver>(*ioc_);
        ws_ = std::make_unique<websocket::stream<tcp::socket>>(*ioc_);
        
        // Look up the domain name
        auto const results = resolver_->resolve(host, port);
        
        // Make the connection on the IP address we get from a lookup
        auto ep = net::connect(ws_->next_layer(), results);
        
        // Set a timeout for the handshake
        ws_->set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        
        // Set a decorator to change the User-Agent of the handshake
        ws_->set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
            }));
            
        // Perform the websocket handshake
        ws_->handshake(host + ":" + port, target);
        
        connected_ = true;
        
        // Start the IO service in a separate thread
        io_thread_ = std::thread([this]() {
            try {
                ioc_->run();
            } catch (const std::exception& e) {
                std::cerr << "IO service exception: " << e.what() << std::endl;
                connected_ = false;
            }
        });
        
        // Start reading messages
        read_thread_ = std::thread([this]() {
            readLoop();
        });
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error connecting to WebSocket server: " << e.what() << std::endl;
        return false;
    }
}

void WebSocketService::disconnect() {
    if (!connected_) return;
    
    try {
        // Close the WebSocket connection
        if (ws_) {
            ws_->close(websocket::close_code::normal);
        }
        
        // Stop the IO context
        if (ioc_) {
            ioc_->stop();
        }
        
        connected_ = false;
        
        // Wait for threads to finish
        if (read_thread_.joinable()) {
            read_thread_.join();
        }
        
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        
        std::cout << "Disconnected from WebSocket server" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during disconnect: " << e.what() << std::endl;
    }
}

bool WebSocketService::isConnected() const {
    return connected_;
}

void WebSocketService::send(const std::string& message) {
    if (!connected_ || !ws_) {
        std::cerr << "Cannot send message: not connected" << std::endl;
        return;
    }
    
    try {
        // Send the message synchronously
        ws_->write(net::buffer(message));
        std::cout << "Message sent: " << message << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
        connected_ = false;
    }
}

void WebSocketService::registerMessageHandler(std::function<void(const std::string&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_handler_ = handler;
}

void WebSocketService::readLoop() {
    try {
        beast::flat_buffer buffer;
        
        while (connected_) {
            // Read a message
            ws_->read(buffer);
            
            // Extract message to string
            std::string message = beast::buffers_to_string(buffer.data());
            std::cout << "Message received: " << message << std::endl;
            
            // Clear the buffer for the next read
            buffer.consume(buffer.size());
            
            // Call handler if registered
            std::lock_guard<std::mutex> lock(mutex_);
            if (message_handler_) {
                message_handler_(message);
            }
        }
    } catch (const beast::system_error& se) {
        // Don't log error if it's just because the connection was closed normally
        if (se.code() != websocket::error::closed) {
            std::cerr << "WebSocket read error: " << se.code().message() << std::endl;
        }
        connected_ = false;
    } catch (const std::exception& e) {
        std::cerr << "Error in read loop: " << e.what() << std::endl;
        connected_ = false;
    }
}
