#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.h>
#undef STB_VORBIS_HEADER_ONLY

#define DR_MP3_HEADER_ONLY
#include <dr_mp3.h>
#undef DR_MP3_HEADER_ONLY

#include <gtest/gtest.h>

TEST(VendorHeaders, StbVorbisHeaderCompiles) {
    SUCCEED() << "stb_vorbis.h included without errors";
}

TEST(VendorHeaders, DrMp3HeaderCompiles) {
    SUCCEED() << "dr_mp3.h included without errors";
}
