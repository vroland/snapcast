/***
    This file is part of snapcast
    Copyright (C) 2014-2021  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "common/aixlog.hpp"

namespace metareader
{
namespace scraper
{


namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>


class HttpClient
{
public:
    using HttpResponseHandler = std::function<void(beast::error_code ec, const http::response<http::vector_body<char>>& response)>;

private:
    // Performs an HTTP GET and prints the response
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        // Objects are constructed with a strand to
        // ensure that handlers do not execute concurrently.
        explicit Session(net::io_context& ioc, bool follow_redirects, HttpResponseHandler handler);

        // Start the asynchronous operation
        void run(const std::string& host, size_t port, const std::string& target);

        void onResolve(beast::error_code ec, tcp::resolver::results_type results);

        void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);

        void onWrite(beast::error_code ec, std::size_t bytes_transferred);

        void onRead(beast::error_code ec, std::size_t bytes_transferred);

    private:
        tcp::resolver resolver_;
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_; // (Must persist between reads)
        http::request<http::empty_body> req_;
        http::response<http::vector_body<char>> res_;
        HttpResponseHandler handler_;
        bool follow_redirects_;
    };

public:
    HttpClient(net::io_context& ioc);

    void get(const std::string& host, const std::string& target, size_t port, bool follow_redirects, HttpResponseHandler handler);

private:
    boost::asio::io_context& ioc_;
};

} // namespace scraper
} // namespace metareader

#endif
