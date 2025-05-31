#include "websocket_session.hpp"
#include <iostream>

// run() → on_resolve() → on_connect() → on_handshake() → on_read() (loop)
// This implementation uses Boost.Beast's async operations throughout, making it non-blocking and efficient for real-time market data processing.
// Raw Buffer Data -> String Conversion -> Handler
// [42 74 63 ...] -> "{"symbol":"BTCUSDT","bids":[...]})" -> Your Handler

using tcp = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

WebsocketSession::WebsocketSession(
        boost::asio::io_context& ioc,
        std::string host,
        std::string port,
        std::string target,
        FrameHandler on_frame)
    : resolver_(ioc)
    , ws_(ioc)
    , host_(std::move(host))
    , handler_(std::move(on_frame))
{
    ws_.next_later().expires_after(std::chrono::seconds(30));
    ws_.set_option(ws::stream_base::decorator(
    [](ws::request_type& req){ req.set(http::field::user_agent, "TriArbBot/0.0.1"); }));
// store target in the stream’s handshake call
    ws_.text(true);
    ws_.async_handshake(host_, target,
        boost::beast::bind_front_handler(
            &WebsocketSession::on_handshake, this));
}

void WebsocketSession::run()
{
    resolver_.async_resolve(host_, "443", boost::beast::bind_front_handler(&WebsocketSession::on_resolve, this));
}

void WebsocketSession::on_resolve(boost::beast::error_code ec, tcp::resolver::results_type results)
{
    if(ec) return std::cerr<<"resolve: "<< ec.message() << '\n';
    boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    boost::beast::get_lowest_layer(ws_).async_connect(
    results,
    boost::beast::bind_front_handler(
        &WebsocketSession::on_connect, this));
}

void WebsocketSession::on_connect(boost::beast::error_code ec)
{
    if (ec) return std::cerr << "connect: " << ec.message() << '\n';
    ws_.async_handshake(host_, "/stream?streams=btcusdt@depth5@100ms",
        boost::beast::bind_front_handler(
            &WebsocketSession::on_handshake, this));
}

void WebsocketSession::on_handshake(boost::beast::error_code ec)
{
    if (ec) return std::cerr << "handshake: " << ec.message() << '\n';
    ws_.async_read(buffer_,
        boost::beast::bind_front_handler(
            &WebsocketSession::on_read, this));
}


void WebsocketSession::on_read(boost::beast::error_code ec, std::size_t)
{
    if (ec) return std::cerr << "read: " << ec.message() << '\n';
    handler_(boost::beast::buffers_to_string(buffer_.data()));
    buffer_.clear();
    ws_.async_read(buffer_,
        boost::beast::bind_front_handler(
            &WebsocketSession::on_read, this));
}
