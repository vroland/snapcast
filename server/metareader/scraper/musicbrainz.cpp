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


#include "musicbrainz.hpp"
#include "common/aixlog.hpp"
#include "common/utils/string_utils.hpp"
#include "json.hpp"


namespace metareader
{
namespace scraper
{

static constexpr auto LOG_TAG = "Musicbrainz";

using namespace std;

Musicbrainz::Musicbrainz(boost::asio::io_context& ioc) : http_(ioc)
{
    LOG(DEBUG, LOG_TAG) << "Musicbrainz\n";
}

void Musicbrainz::scrape(const std::string& title, const std::string& artist, const std::string& album)
{
    std::ignore = album;
    std::stringstream query;
    using utils::string::url_encode;
    query << url_encode(title);
    if (!artist.empty())
        query << url_encode(" AND ") << "artist:" << url_encode(artist);
    if (last_query_ == query.str())
        return;
    last_query_ = query.str();
    LOG(INFO, LOG_TAG) << "Query: " << query.str() << "\n";
    const string host = "musicbrainz.org";
    const string target = "/ws/2/release-group/?query=" + query.str() + "&fmt=json&limit=1";
    LOG(INFO, LOG_TAG) << "scrape http://" << host << target << "\n";
    http_.get(host, target, 80, true, [this](beast::error_code ec, const http::response<http::vector_body<char>>& response) { onScrape(ec, response); });
}

void Musicbrainz::scrapeCover(const std::string& mbid) //, HttpClient::HttpResponseHandler handler)
{
    LOG(INFO, LOG_TAG) << "ID: " << mbid << "\n";
    std::string host = "coverartarchive.org";
    std::string target = "/release/" + mbid + "/front-500"; // 250";
    LOG(INFO, LOG_TAG) << "scrapeCover http://" << host << target << "\n";
    http_.get(host, target, 80, true, [this](beast::error_code ec, const http::response<http::vector_body<char>>& response) { onScrapeCover(ec, response); });
}

void Musicbrainz::onScrape(beast::error_code ec, const http::response<http::vector_body<char>>& response)
{
    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "Error: " << ec.message() << "\n";
    }
    else
    {
        // LOG(ERROR, LOG_TAG) << "Response: " << payload.data() << "\n";
        nlohmann::json j = nlohmann::json::parse(response.body());

        // Write the message to standard out
        // std::cout << j.dump(3) << std::endl;
        auto release_groups = j["release-groups"];
        if (release_groups.is_array() && !release_groups.empty())
        {
            auto releases = release_groups.front()["releases"];
            if (releases.is_array() && !releases.empty())
            {
                scrapeCover(releases[0]["id"].get<std::string>());
                // [this](beast::error_code ec, const http::response<http::vector_body<char>>& response) { onScrapeCover(ec, response); });
            }
        }
    }
}

void Musicbrainz::onScrapeCover(beast::error_code ec, const http::response<http::vector_body<char>>& response)
{
    if (ec)
    {
        LOG(ERROR, LOG_TAG) << "ScrapeCover Error: " << ec.message() << "\n";
    }
    else
    {
        LOG(INFO, LOG_TAG) << "ScrapeCover Result: " << response.result_int() << "\n";
        if (response.result() == beast::http::status::ok)
        {
            LOG(INFO, LOG_TAG) << "Cover: " << response.body().size() << "\n";
            std::ofstream fout("cover.jpg", ios::out | ios::binary);
            fout.write(response.body().data(), response.body().size());
        }
    }
}



} // namespace scraper
} // namespace metareader
