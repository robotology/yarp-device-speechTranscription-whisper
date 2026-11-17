/*
 * SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WHISPERSPEECHTRANSCRIPTION_H
#define WHISPERSPEECHTRANSCRIPTION_H

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/ISpeechTranscription.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/ResourceFinder.h>

#include <mutex>
#include <string>
#include <vector>

#include "whisper.h"
#include "WhisperSpeechTranscription_ParamsParser.h"

/**
 * @ingroup dev_impl_other
 *
 * \brief `WhisperSpeechTranscription`: A yarp device which performs audio-to-text transcription using OpenAI Whisper models.
 * This device implements the ISpeechTranscription and can be used with a speechTranscription_nws_yarp device and a AudioRecorderWrapper to transcribe audio in real time.
 *
 *  Parameters required by this device are described in the class WhisperSpeechTranscription_ParamsParser
*/
class WhisperSpeechTranscription :
        public yarp::dev::DeviceDriver,
        public yarp::dev::ISpeechTranscription,
        public WhisperSpeechTranscription_ParamsParser
{
private:
    // whisper_full_params only stores raw `const char*` pointers: the strings
    // they point to must outlive every whisper_full() call, so they are kept here.
    std::string                     m_initial_prompt;
    std::vector<float>              m_pcmf32;               // mono-channel F32 PCM at WHISPER_SAMPLE_RATE
    struct whisper_context*         m_ctx = nullptr;
    whisper_full_params             m_wparams;
    int32_t                         m_n_processors = 1;
    std::mutex                      m_mutex;

    bool isLanguageSupported(const std::string& language) const;

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
