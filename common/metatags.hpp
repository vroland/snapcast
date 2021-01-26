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

#ifndef METATAGS_HPP
#define METATAGS_HPP

#include <boost/optional.hpp>
#include <string>


struct Metatags
{
    /// the artist name. Its meaning is not well-defined; see “composer” and “performer” for more specific tags.
    boost::optional<std::string> artist;
    /// same as artist, but for sorting. This usually omits prefixes such as “The”.
    boost::optional<std::string> artistsort;
    /// the album name.
    boost::optional<std::string> album;
    /// same as album, but for sorting.
    boost::optional<std::string> albumsort;
    /// on multi-artist albums, this is the artist name which shall be used for the whole album. The exact meaning of this tag is not well-defined.
    boost::optional<std::string> albumartist;
    /// same as albumartist, but for sorting.
    boost::optional<std::string> albumartistsort;
    /// the song title.
    boost::optional<std::string> title;
    /// the decimal track number within the album.
    boost::optional<uint16_t> track;
    /// a name for this song. This is not the song title. The exact meaning of this tag is not well-defined. It is often used by badly configured internet radio
    /// stations with broken tags to squeeze both the artist name and the song title in one tag.
    boost::optional<std::string> name;
    /// the music genre.
    boost::optional<std::string> genre;
    /// the song’s release date. This is usually a 4-digit year.
    boost::optional<std::string> date;
    /// the song’s original release date.
    boost::optional<std::string> originaldate;
    /// the artist who composed the song.
    boost::optional<std::string> composer;
    /// the artist who performed the song.
    boost::optional<std::string> performer;
    /// the conductor who conducted the song.
    boost::optional<std::string> conductor;
    /// “a work is a distinct intellectual or artistic creation, which can be expressed in the form of one or more audio recordings”
    boost::optional<std::string> work;
    /// “used if the sound belongs to a larger category of sounds/music” (from the IDv2.4.0 TIT1 description).
    boost::optional<std::string> grouping;
    /// a human-readable comment about this song. The exact meaning of this tag is not well-defined.
    boost::optional<std::string> comment;
    /// the decimal disc number in a multi-disc album.
    boost::optional<uint16_t> disc;
    /// the name of the label or publisher.
    boost::optional<std::string> label;
    /// the artist id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_artistid;
    /// the album id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_albumid;
    /// the album artist id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_albumartistid;
    /// the track id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_trackid;
    /// the release track id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_releasetrackid;
    /// the work id in the MusicBrainz database.
    boost::optional<std::string> musicbrainz_workid;
};


#endif
