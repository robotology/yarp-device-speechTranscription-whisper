/*
 * SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <yarp/dev/ISpeechTranscription.h>
#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/WrapperSingle.h>

#include <yarp/sig/Sound.h>
#include <yarp/sig/SoundFile.h>

#include <catch2/catch_amalgamated.hpp>
#include <harness.h>

using namespace yarp::dev;
using namespace yarp::os;

TEST_CASE("dev::whisperSpeechTranscription", "[yarp::dev]")
{
    YARP_REQUIRE_PLUGIN("whisperSpeechTranscription", "device");

    Network::setLocalMode(true);

    SECTION("Checking whisperSpeechTranscription device")
    {
        yarp::dev::ISpeechTranscription* istr=nullptr;
        PolyDriver ddfake;

        //read a test sound file from disk
        yarp::sig::Sound snd;
        yarp::os::ResourceFinder rf;

        rf.setQuiet(false);
        rf.setVerbose(true);

        rf.setDefaultContext("whisperTranscribe_demo");
        std::string ss = rf.findFile("audio_in.wav");
        CHECK(!ss.empty());
        yarp::sig::file::read(snd,ss.c_str());
        CHECK(snd.getSamples()>0);

        //find the model
        std::string ss2 = rf.findFile("ggml-base.en.bin");
        CHECK(!ss2.empty());

        //open the device
        {
            Property pdev_cfg;
            pdev_cfg.put("device", "whisperSpeechTranscription");
            pdev_cfg.put("model", ss2.c_str());
            REQUIRE(ddfake.open(pdev_cfg));
            REQUIRE(ddfake.view(istr));
        }

        std::string lang = "auto";
        CHECK(istr->getLanguage(lang));
        CHECK(lang=="en");

        //not sue if ggml-base.en.bin can support other languages.
        //probably setLanguage should return an error?
        //CHECK(istr->setLanguage("en"));
        CHECK(istr->getLanguage(lang));
        CHECK(lang == "en");

        std::string transcript;
        double score;
        CHECK(istr->transcribe(snd,transcript, score));
        CHECK(transcript==" And so my fellow Americans, ask not what your country can do for you, ask what you can do for your country.");
        CHECK(score > 0.99);

        //"Close all polydrivers and check"
        {
            yarp::os::Time::delay(0.1);
            CHECK(ddfake.close());
        }
    }

    Network::setLocalMode(false);
}
