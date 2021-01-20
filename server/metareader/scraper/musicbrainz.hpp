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

#ifndef MUSICBRAINZ_HPP
#define MUSICBRAINZ_HPP

#include "http_client.hpp"
#include <boost/asio.hpp>


namespace metareader
{
namespace scraper
{


class Musicbrainz
{
public:
    Musicbrainz(boost::asio::io_context& ioc);
    void scrape(const std::string& title, const std::string& artist, const std::string& album = "");

private:
    void scrapeCover(const std::string& mbid); //, HttpClient::HttpResponseHandler handler);
    void onScrape(beast::error_code ec, const http::response<http::vector_body<char>>& response);
    void onScrapeCover(beast::error_code ec, const http::response<http::vector_body<char>>& response);

    HttpClient http_;
};

} // namespace scraper
} // namespace metareader

#endif
