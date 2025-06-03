#pragma once

// Beast core, HTTP, WebSocket and SSL
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/http.hpp>

// Asio I/O and TCP
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

// Standard headers
#include <string>
#include <string_view>
#include <functional>
#include <chrono>

/// Asynchronously connects to a Binance WS stream and
/// forwards each text frame to the supplied callback.
class WebsocketSession {
public:
    using FrameHandler = std::function<void(std::string_view)>;

    /// Construct with:
    ///   - ioc: the Asio I/O context
    ///   - host: e.g. "testnet.binance.vision" or "stream.binance.com"
    ///   - port: "9443"
    ///   - target: e.g. "/stream?streams=btcusdt@depth5@100ms"
    ///   - on_frame: callback invoked on each full JSON frame
    WebsocketSession(
        boost::asio::io_context& ioc,
        std::string             host,
        std::string             port,
        std::string             target,
        FrameHandler            on_frame);

    /// Starts the resolve→connect→handshake chain
    void run();

private:
    // Step 1: DNS resolution callback
    void on_resolve(
        boost::beast::error_code ec,
        boost::asio::ip::tcp::resolver::results_type results);

    // Step 2: TCP connect callback
    void on_connect(
        boost::beast::error_code ec,
        boost::asio::ip::tcp::endpoint endpoint); 

    // Step 3: SSL handshake callback
    void on_ssl_handshake(boost::beast::error_code ec);

    // Step 4: WebSocket handshake callback  
    void on_handshake(boost::beast::error_code ec);

    // Step 5: Read loop callback
    void on_read(
        boost::beast::error_code ec,
        std::size_t bytes_transferred);

    // Certificate verification callback
    bool verify_certificate(
        bool preverified,
        boost::asio::ssl::verify_context& ctx);

    // Member variables
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>> ws_;
    boost::beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::string target_;
    FrameHandler handler_;
};
