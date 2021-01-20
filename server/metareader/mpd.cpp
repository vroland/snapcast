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

#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "common/aixlog.hpp"
#include "common/snap_exception.hpp"
#include "common/str_compat.hpp"
#include "common/utils/string_utils.hpp"

#include "mpd.hpp"

using namespace std;

namespace metareader
{

static constexpr auto LOG_TAG = "MPD";


Mpd::Mpd(boost::asio::io_context& ioc, const std::string& settings) : resolver_(ioc), socket_(ioc), timer_(ioc), host_("127.0.0.1"), port_(6600), scraper_(ioc)
{
    connectMeta();
    if (!settings.empty())
    {
        auto host = utils::string::split(settings, ':');
        if (host.size() >= 1)
            host_ = host[0];
        if (host.size() >= 2)
            port_ = cpt::stoi(host[1], 6600);
    }
}


void Mpd::logResponse(const Response& response, AixLog::Severity severity) const
{
    LOG(severity, LOG_TAG) << "Response received: " << response.command << "\n";
    for (const auto& line : response.message)
    {
        LOG(severity, LOG_TAG) << line << "\n";
    }
}

void Mpd::reconnectMeta()
{
    // reconnect after 1s
    LOG(INFO, LOG_TAG) << "reconnect meta\n";
    timer_.expires_after(1s);
    timer_.async_wait([this](const boost::system::error_code& ec) {
        if (ec.failed())
            LOG(INFO, LOG_TAG) << "Failed to reconnect: " << ec.message() << "\n";
        else
            connectMeta();
    });
}

void Mpd::connectMeta()
{
    LOG(INFO, LOG_TAG) << "connect meta\n";
    if (socket_.is_open())
        socket_.close();
    socket_.async_connect(tcp::endpoint(boost::asio::ip::address::from_string(host_), port_), [this](const boost::system::error_code& ec) {
        if (ec.failed())
        {
            LOG(ERROR, LOG_TAG) << "Failed to connect: " << ec.message() << "\n";
            reconnectMeta();
        }
        else
        {
            // Read connect response "OK MPD 0.21.11"
            getResponse("connect", [this](const boost::system::error_code& ec, const Response& response) {
                logResponse(response, AixLog::Severity::info);
                if (ec.failed())
                {
                    LOG(ERROR, LOG_TAG) << "Failed to get response: " << ec.message() << "\n";
                    reconnectMeta();
                }
                else
                {
                    // Start meta reading loop
                    readMeta();
                    // interrupt the first idle to query initial status
                    boost::asio::async_write(socket_, boost::asio::buffer("noidle\n", 7), [this](boost::system::error_code ec, std::size_t length) {
                        if (ec.failed())
                        {
                            LOG(ERROR, LOG_TAG) << "Failed to send \"noidle\", error: " << ec.message() << ", sent: " << length << "\n";
                            reconnectMeta();
                        }
                    });
                }
            });
        }
    });
}


void Mpd::readMeta()
{
    sendCommand("idle", [this](const boost::system::error_code& ec, const Response& response) {
        if (ec.failed())
        {
            LOG(ERROR, LOG_TAG) << "Failed to send \"idle\", error: " << ec.message() << "\n";
            reconnectMeta();
        }
        else
        {
            logResponse(response, AixLog::Severity::info);
            sendCommand("currentsong", [this](const boost::system::error_code& ec, const Response& response) {
                if (ec.failed())
                {
                    LOG(ERROR, LOG_TAG) << "Failed to send \"idle\", error: " << ec.message() << "\n";
                    reconnectMeta();
                }
                else
                {
                    auto artist = response.get("Artist");
                    auto title = response.get("Title");

                    scraper_.scrape(title, artist);

                    logResponse(response, AixLog::Severity::info);
                    sendCommand("status", [this](const boost::system::error_code& ec, const Response& response) {
                        if (ec.failed())
                        {
                            LOG(ERROR, LOG_TAG) << "Failed to send \"idle\", error: " << ec.message() << "\n";
                            reconnectMeta();
                        }
                        else
                        {
                            logResponse(response, AixLog::Severity::info);
                            readMeta();
                        }
                    });
                }
            });
        }
    });
}


void Mpd::sendCommand(const std::string& command, ReceiveHandler handler)
{
    LOG(INFO, LOG_TAG) << "Sending command \"" << command << "\"\n";
    boost::asio::async_write(socket_, boost::asio::buffer(command + "\n", command.size() + 1),
                             [this, command, handler](boost::system::error_code ec, std::size_t length) {
                                 if (ec)
                                 {
                                     LOG(ERROR, LOG_TAG) << "Failed to send message, error: " << ec.message() << "\n";
                                 }
                                 else
                                 {
                                     LOG(DEBUG, LOG_TAG) << "Wrote " << length << " bytes to socket\n";
                                     getResponse(command, handler);
                                 }
                             });
}

void Mpd::getResponse(const std::string& command, ReceiveHandler handler)
{
    static Response response;
    boost::asio::async_read_until(socket_, streambuf_, "\n", [this, command, handler](boost::system::error_code ec, std::size_t length) mutable {
        if (ec)
        {
            LOG(ERROR, LOG_TAG) << "Error reading response of length " << length << ": " << ec.message() << "\n";
            reconnectMeta();
        }
        else
        {
            std::string str(boost::asio::buffers_begin(streambuf_.data()), boost::asio::buffers_begin(streambuf_.data()) + length);
            LOG(DEBUG, LOG_TAG) << "Read: " << str << "\n";
            response.message.push_back(std::move(str));
            streambuf_.consume(length);
            if ((response.message.back().find("OK") == 0) || (response.message.back().find("ACK ") == 0))
            {
                response.command = command;
                handler(ec, response);
                response.message.clear();
            }
            else
            {
                getResponse(command, handler);
            }
        }
    });
}

} // namespace metareader
