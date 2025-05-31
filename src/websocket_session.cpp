#include "websocket_session.hpp"
#include <iostream>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
namespace ssl   = net::ssl;
using tcp      = net::ip::tcp;

WebsocketSession::WebsocketSession(
    net::io_context& ioc,
    std::string host,
    std::string port,
    std::string target,
    FrameHandler on_frame)
    : resolver_(ioc)
    , ctx_(ssl::context::tlsv12_client)
    , ws_(ioc, ctx_)
    , host_(std::move(host))
    , port_(std::move(port))
    , target_(std::move(target))
    , handler_(std::move(on_frame))
{
    // These are required for Binance's SSL certificate validation
    ctx_.set_verify_mode(ssl::verify_peer);
    ctx_.set_default_verify_paths();
}

void WebsocketSession::run()
{
    // Start the asynchronous DNS resolution for host_:port_
    resolver_.async_resolve(
        host_,
        port_,
        beast::bind_front_handler(
            &WebsocketSession::on_resolve,
            this));
}

void WebsocketSession::on_resolve(
    beast::error_code ec,
    tcp::resolver::results_type results)
{
    if (ec) {
        std::cerr << "Resolve error: " << ec.message() << "\n";
        return;
    }

    // Set a 30‐second timeout on the underlying TCP layer
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Attempt to connect the TCP socket to one of the resolved endpoints
    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &WebsocketSession::on_connect,
            this));
}

void WebsocketSession::on_connect(
    beast::error_code ec,
    tcp::endpoint endpoint)
{
    if (ec) {
        std::cerr << "Connect error: " << ec.message() << "\n";
        return;
    }

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(
        ws_.next_layer().native_handle(), host_.c_str()))
    {
        ec = beast::error_code(
            static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category());
        std::cerr << "SSL SNI error: " << ec.message() << "\n";
        return;
    }

    // Update the hostname in handshake (SSL)
    ws_.next_layer().set_verify_callback(
        [this](bool preverified, ssl::verify_context& ctx) {
            return this->verify_certificate(preverified, ctx);
        });

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &WebsocketSession::on_ssl_handshake,
            this));

    // just set a User-Agent
    ws_.set_option(ws::stream_base::decorator(
        [](ws::request_type& r){
            r.set(http::field::user_agent, "TriArbBot/0.0.1");
        }));

}

bool WebsocketSession::verify_certificate(
    bool preverified,
    ssl::verify_context& ctx)
{
    // Just use the pre-verification
    return preverified;
}

void WebsocketSession::on_ssl_handshake(beast::error_code ec)
{
    if (ec) {
        std::cerr << "SSL handshake error: " << ec.message() << "\n";
        return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Enable binary frames
    ws_.binary(false);

    // Set suggested timeout settings for the websocket
    ws_.set_option(
        ws::stream_base::timeout::suggested(
            beast::role_type::client));

    // Specify permessage-deflate settings
    ws_.set_option(ws::permessage_deflate{});

    // Set a decorator to change the User-Agent and other headers
    ws_.set_option(ws::stream_base::decorator(
        [](ws::request_type& req) {
            req.set(http::field::user_agent, "TriArbBot/0.0.1");
            req.set(http::field::host, "testnet.binance.vision");
            req.set(http::field::upgrade, "websocket");
            req.set(http::field::connection, "upgrade");
            req.set(http::field::sec_websocket_version, "13");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(host_, target_,
        beast::bind_front_handler(
            &WebsocketSession::on_handshake,
            this));
}

void WebsocketSession::on_handshake(beast::error_code ec)
{
    if (ec) {
        std::cerr << "Handshake error: " << ec.message() << "\n";
        return;
    }

    // We’re now connected and handshaken. Start reading messages into our buffer:
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebsocketSession::on_read,
            this));
}

void WebsocketSession::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "Read error: " << ec.message() << "\n";
        return;
    }

    // Convert the buffer to a string and call the user callback
    handler_(beast::buffers_to_string(buffer_.data()));

    // Clear the buffer for the next frame
    buffer_.consume(buffer_.size());

    // Read again—loop forever until error or program exit
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WebsocketSession::on_read,
            this));
}
