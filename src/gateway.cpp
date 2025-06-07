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