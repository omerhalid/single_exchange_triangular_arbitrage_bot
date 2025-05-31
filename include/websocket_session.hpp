#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <string>
#include <functional>

//Asynchronously connetcs to a Binance WS stream and forwards each text frame to the supplied callback.

class WebsocketSession
{
public:

    using FrameHandler = std::function<void(std::string_view)>;

    WebsocketSession(
        boost::asio::io_context& ioc,
        std::string host,
        std::string port,
        std:: on_frame);

        void run; // kick off asznc dial + read loop
private:

        void on_resolve(boost::beast::error_code, boost::asio::op::tcp::resolver::results_type);
        void on_connect(boost::beast::error_code);
        void on_handshake(boost::beast::error_code);
        void on_read(boost::beast::error_code, std::size_t bytes);

        boost::asio::ip::tcp:.resolver resolver_;
        boost::asio::websocket::stream<boost::best::tcp_stream> we_;
        boost::beast::flat_buffer buffer_;
        std::string host_;
        FrameHandler handler_;
};
