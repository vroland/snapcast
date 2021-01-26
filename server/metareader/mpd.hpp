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

#ifndef MPD_HPP
#define MPD_HPP

#include "common/aixlog.hpp"
#include "common/metatags.hpp"
#include "scraper/musicbrainz.hpp"

#include <boost/asio.hpp>

namespace metareader
{


using boost::asio::ip::tcp;
using boost::asio::posix::stream_descriptor;


class Mpd
{
    struct Response
    {
        std::string command;
        std::vector<std::string> message;

        std::string get(const std::string& key) const
        {
            for (const auto& s : message)
            {
                if (s.find(key + ": ") == 0)
                    return s.substr(key.size() + 2);
            }
            return "";
        }
    };

    using ReceiveHandler = std::function<void(const boost::system::error_code& ec, const Response& response)>;

public:
    /// ctor. Encoded PCM data is passed to the PipeListener
    Mpd(boost::asio::io_context& ioc, const std::string& settings);

protected:
    void logResponse(const Response& response, AixLog::Severity severity) const;
    void connectMeta();
    void reconnectMeta();
    void getResponse(const std::string& command, ReceiveHandler handler);
    void readMeta();
    void sendCommand(const std::string& command, ReceiveHandler handler);

    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf streambuf_;
    boost::asio::steady_timer timer_;
    std::string host_;
    uint16_t port_;
    scraper::Musicbrainz scraper_;
};

} // namespace metareader

#endif
