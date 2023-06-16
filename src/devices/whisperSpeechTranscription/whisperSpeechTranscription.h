/*
 * SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WHISPERSPEECHTRANSCRIPTION_H
#define WHISPERSPEECHTRANSCRIPTION_H

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/ISpeechTranscription.h>
#include <yarp/os/Bottle.h>
#include <stdio.h>

using namespace yarp::os;

/**
 * @ingroup dev_impl_other
 *
 * \brief `WhisperSpeechTranscription`: A fake implementation of a speech transcriber plugin.
 */
class WhisperSpeechTranscription :
        public yarp::dev::DeviceDriver,
        public yarp::dev::ISpeechTranscription
{
private:
    bool m_verbose = true;
    std::string m_language="auto";

public:
    WhisperSpeechTranscription();
    virtual ~WhisperSpeechTranscription();
    WhisperSpeechTranscription(const WhisperSpeechTranscription&) = delete;
    WhisperSpeechTranscription(WhisperSpeechTranscription&&) = delete;
    WhisperSpeechTranscription& operator=(const WhisperSpeechTranscription&) = delete;
    WhisperSpeechTranscription& operator=(WhisperSpeechTranscription&&) = delete;

    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    virtual bool setLanguage(const std::string language) override;
    virtual bool getLanguage(std::string& language) override;
    virtual bool transcribe(const yarp::sig::Sound& sound, std::string& transcription, double& score) override;
};

#endif
