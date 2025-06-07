#include "gateway.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <openssl/hmac.h>
#include <chrono>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace ssl   = boost::asio::ssl;
using tcp       = boost::asio::ip::tcp;

/* HMAC-SHA256 Signature Generator
 * -----------------------------
 * Creates a cryptographic signature using HMAC-SHA256 algorithm
 * Required by Binance API for request authentication
 *
 * Parameters:
 * - key: API secret key used for signing
 * - msg: Message to be signed (usually request parameters)
 *
 * Returns:
 * - Hexadecimal string representation of the HMAC-SHA256 signature
 *
 * Example:
 * auto sig = hmac_sha256("my_secret", "symbol=BTCUSDT&side=BUY");
 * // Returns something like: "a1b2c3d4e5f6..."
 */
static std::string hmac_sha256(const std::string& key,
                               const std::string& msg)
{
    unsigned char mac[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;
    HMAC(EVP_sha256(),
         key.data(), key.size(),
         (unsigned char*)msg.data(), msg.size(),
         mac, &len);
    std::ostringstream oss;
    for(unsigned i=0;i<len;++i)
        oss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)mac[i];
    return oss.str();
}

Gateway::Gateway(boost::asio::io_context& ioc,
                std::string              rest_host, 
                ApiKeys                  keys,
                bool                     live)
    : host_(std::move(rest_host))                   
    , ioc_(ioc)                                      
    , live_(live)                                    
    , keys_(std::move(keys))                       
    , ctx_(boost::asio::ssl::context::tlsv12_client) 
{
    ctx_.set_default_verify_paths();
}

void Gateway::send_order(std::string_view symbol,
            std::string_view side, // BUY / SELL
            double qty,
            double price,
            std::function<void(FillReport)> cb)
{
    /* If not in live mode, simulate the order and return immediately
     * Used for testing without making real trades */
    if(!live_)
    {
        std::cout<<"[DRY-RUN] "<<side<<" "<<qty<<" "<<symbol<<" @ "<<price<<"\n";
        cb({true, qty, price}); // pretend filled
        return;
    }

    /* Get current timestamp in milliseconds - required by Binance API
     * for request signature validation */
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();

    /* Build the order parameters as a URL-encoded string
     * Format: key1=value1&key2=value2&... */
    std::ostringstream body;
    body<<"symbol="   <<symbol
        <<"&side="    <<side
        <<"&type=LIMIT_MAKER"      /* Order type that ensures we're maker not taker */
        <<"&timeInForce=IOC"       /* Immediate-or-Cancel: fill what's possible immediately */
        <<"&quantity="<<std::fixed<<std::setprecision(6)<<qty  /* 6 decimals for crypto quantity */
        <<"&price="   <<std::setprecision(2)<<price            /* 2 decimals for price */
        <<"&recvWindow=5000"       /* How long the request is valid for */
        <<"&timestamp="<<ts;       /* Timestamp for request validation */

    /* Create HMAC-SHA256 signature of the request parameters
     * Required by Binance API for authentication */
    auto sig = hmac_sha256(keys_.secret, body.str());
    body<<"&signature="<<sig;

    /* Create and configure the HTTP POST request
     * Using Boost.Beast for HTTP functionality */
    auto req = std::make_shared<http::request<http::string_body>>();
    
    req->method(http::verb::post);               /* Set HTTP method to POST */
    req->target("/api/v3/order");               /* Binance API endpoint for orders */
    req->set(http::field::host, host_);         /* Set the host header (api.binance.com) */
    req->set("X-MBX-APIKEY", keys_.key);        /* Add API key for authentication */
    req->set(http::field::content_type,         /* Set content type for form data */
             "application/x-www-form-urlencoded");
    req->body() = body.str();                   /* Set the request body with parameters */
    req->prepare_payload();                     /* Finalize the request for sending */
}
