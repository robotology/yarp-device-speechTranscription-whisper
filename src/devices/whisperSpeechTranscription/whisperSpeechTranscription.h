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

#include "whisper.h"

using namespace yarp::os;

/**
 * @ingroup dev_impl_other
 *
 * \brief `WhisperSpeechTranscription`: A yarp device which performs audio-to-text transcription using OpenAI Whisper models.
 * This device implements the ISpeechTranscription and can be used with a speechTranscription_nws_yarp device and a AudioRecorderWrapper to transcribe audio in real time.
 *
 *  Parameters required by this device are:
 * | Parameter name | SubParameter   | Type    | Units          | Default Value    | Required     | Description                                                       | Notes |
 * |:--------------:|:--------------:|:-------:|:--------------:|:----------------:|:-----------: |:-----------------------------------------------------------------:|:-----:|
 * | model          |      -         | string  | -              | -                | Yes          | Full path tot the model file, e.g. ggml-base.en.bin               |       |
 * | language       |      -         | string  | -              | auto             | No           | Language (??? TBC)                                                |       |
 * | remove_symbols |      -         | bool    | -              | true             | No           | Removed symbols from output text, i.e. ...[bla bla]...            |       |
*/
class WhisperSpeechTranscription :
        public yarp::dev::DeviceDriver,
        public yarp::dev::ISpeechTranscription
{
private:
    bool                            m_verbose = true;
    bool                            m_no_symbols = true;
    std::string                     m_language="auto";
    std::string                     m_model;
    std::vector<float>              m_pcmf32;               // mono-channel F32 PCM
    std::vector<std::vector<float>> m_pcmf32s;              // stereo-channel F32 PCM
    struct whisper_context*         m_ctx= nullptr;
    whisper_full_params             m_wparams;

    int32_t                         n_processors = 1;

public:
    WhisperSpeechTranscription();
    virtual ~WhisperSpeechTranscription();
    WhisperSpeechTranscription(const WhisperSpeechTranscription&) = delete;
    WhisperSpeechTranscription(WhisperSpeechTranscription&&) = delete;
    WhisperSpeechTranscription& operator=(const WhisperSpeechTranscription&) = delete;
    WhisperSpeechTranscription& operator=(WhisperSpeechTranscription&&) = delete;

    //DeviceDriver interface
    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    //ISpeechTranscription interface
    virtual yarp::dev::ReturnValue setLanguage(const std::string& language) override;
    virtual yarp::dev::ReturnValue getLanguage(std::string& language) override;
    virtual yarp::dev::ReturnValue transcribe(const yarp::sig::Sound& sound, std::string& transcription, double& score) override;
};

#endif
