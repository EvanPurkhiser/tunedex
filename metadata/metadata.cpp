#include "metadata.hpp"

#include <taglib/fileref.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/aifffile.h>
#include <taglib/attachedpictureframe.h>
#include <iostream>

typedef std::pair<TagLib::File*, TagLib::ID3v2::Tag*> fileTags;

fileTags get_tags(const char* path)
{
    const auto path_str = std::string(path);
    const int ext_pos = path_str.rfind(".");

    // Bad extension, can't determine what file loader to use
    if (ext_pos == -1) return {0, 0};

    auto ext = path_str.substr(ext_pos + 1);

    // AIFF file, access the ID3 tags directly using AIFF::File::tag
    if (ext == "aif" || ext == "aiff")
    {
        auto file = new TagLib::RIFF::AIFF::File(path);
        return fileTags{file, file->tag()};
    }

    // MP3 file, access the ID3 tags using MPEG::File::ID3v2Tag
    if (ext == "mp3")
    {
        auto file = new TagLib::MPEG::File(path);
        return fileTags{file, file->ID3v2Tag()};
    }

    return {0, 0};
}

const char* frame_str(TagLib::ID3v2::FrameList frame)
{
    if (frame.isEmpty()) return "";

    // Get the frame value as a utf8 std::string
    auto str = frame.front()->toString().to8Bit(true);

    // Copy the value into memory
    char* value = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), value);
    value[str.size()] = '\0';

    return value;
}

/**
 * Retrieve tag information given a file path. Currently only MP3 and AIFF
 * files are supported.
 */
track* metadata(const char* path)
{
    auto file_tags = get_tags(path);

    auto file = file_tags.first;
    auto tags = file_tags.second;

    if (!file || !tags) return 0;

    auto frames = tags->frameListMap();
    auto metadata = new track();

    // Construct the rest of the track struct
    metadata->artist       = frame_str(frames["TPE1"]);
    metadata->title        = frame_str(frames["TIT2"]);
    metadata->album        = frame_str(frames["TALB"]);
    metadata->remixer      = frame_str(frames["TPE4"]);
    metadata->publisher    = frame_str(frames["TPUB"]);
    metadata->comment      = frame_str(frames["COMM"]);
    metadata->key          = frame_str(frames["TKEY"]);
    metadata->bpm          = frame_str(frames["TBPM"]);
    metadata->year         = frame_str(frames["TDRC"]);
    metadata->track_number = frame_str(frames["TRCK"]);
    metadata->disc_number  = frame_str(frames["TPOS"]);
    metadata->genre        = frame_str(frames["TCON"]);

    // Copy artwork (if available) into the metadata
    auto art_frames = frames["APIC"];

    if (!art_frames.isEmpty())
    {
        auto frame   = (TagLib::ID3v2::AttachedPictureFrame*) art_frames.front();
        auto artwork = frame->picture();

        auto art_data = new char[artwork.size()];
        std::copy(artwork.begin(), artwork.end(), art_data);

        metadata->artwork = art_data;
        metadata->art_size = artwork.size();
    }

    delete file;
    return metadata;
}
