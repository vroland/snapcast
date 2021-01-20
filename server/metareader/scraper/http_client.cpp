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


#include "http_client.hpp"
#include "common/str_compat.hpp"
#include "common/utils/string_utils.hpp"


namespace metareader
{
namespace scraper
{

static constexpr auto LOG_TAG = "HTTP";


HttpClient::HttpClient(net::io_context& ioc) : ioc_(ioc)
{
}


void HttpClient::get(const std::string& host, const std::string& target, size_t port, bool follow_redirects, HttpResponseHandler handler)
{
    std::make_shared<Session>(ioc_, follow_redirects, handler)->run(host.c_str(), port, target.c_str());
}


// Objects are constructed with a strand to
// ensure that handlers do not execute concurrently.
HttpClient::Session::Session(net::io_context& ioc, bool follow_redirects, HttpResponseHandler handler)
    : resolver_(net::make_strand(ioc)), stream_(net::make_strand(ioc)), handler_(handler), follow_redirects_(follow_redirects)
{
}


// Start the asynchronous operation
void HttpClient::Session::run(const std::string& host, size_t port, const std::string& target)
{
    // Set up an HTTP GET request message
    req_.version(11);
    req_.method(http::verb::get);
    req_.target(target);
    req_.set(http::field::host, host);
    res_.clear();
    res_.body().clear();

    std::string user_agent = "Snapcast/" + std::string(VERSION);
    req_.set(http::field::user_agent, user_agent);

    // Look up the domain name
    resolver_.async_resolve(host, cpt::to_string(port), beast::bind_front_handler(&Session::onResolve, shared_from_this()));
}


void HttpClient::Session::onResolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "Error resolving host: " << ec.message() << "\n";
        handler_(ec, {});
        return;
    }

    // Set a timeout on the operation
    stream_.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    stream_.async_connect(results, beast::bind_front_handler(&Session::onConnect, shared_from_this()));
}


void HttpClient::Session::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "Error connecting to host: " << ec.message() << "\n";
        handler_(ec, {});
        return;
    }

    // Set a timeout on the operation
    stream_.expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(stream_, req_, beast::bind_front_handler(&Session::onWrite, shared_from_this()));
}


void HttpClient::Session::onWrite(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "Error sending HTTP request: " << ec.message() << "\n";
        handler_(ec, {});
        return;
    }

    // Receive the HTTP response
    http::async_read(stream_, buffer_, res_, beast::bind_front_handler(&Session::onRead, shared_from_this()));
}


void HttpClient::Session::onRead(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "Error reading HTTP response: " << ec.message() << "\n";
        handler_(ec, {});
        return;
    }

    // std::cerr << "res: " << res_ << "\n";

    bool redirect = (follow_redirects_ && (res_.result_int() >= 300) && (res_.result_int() < 400));

    if (!redirect)
        handler_(ec, res_);

    // Gracefully close the socket
    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes so don't bother reporting it.
    if (ec && ec != beast::errc::not_connected)
    {
        LOG(ERROR, LOG_TAG) << "Error shutting down HTTP connection: " << ec.message() << "\n";
    }
    // If we get here then the connection is closed gracefully

    if (redirect)
    {
        std::string location = res_.at("Location").to_string();
        if (!location.empty())
        {
            LOG(INFO, LOG_TAG) << "location: \"" << location << "\"\n";
            if (location.find("http://") == 0)
                location = location.substr(7);
            LOG(INFO, LOG_TAG) << "location: \"" << location << "\"\n";
            std::string target;
            location = utils::string::split_left(location, '/', target);
            LOG(INFO, LOG_TAG) << "location: \"" << location << ", target: \"" << target << "\"\n";
            run(location.c_str(), 80, "/" + target);
        }
    }
}

} // namespace scraper
} // namespace metareader
