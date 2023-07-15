/*
 * SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "whisperSpeechTranscription.h"

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/LogComponent.h>

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <thread>

using namespace yarp::os;
using namespace yarp::dev;

namespace {
YARP_LOG_COMPONENT(WHISPER_SPEECHTR, "yarp.device.WhisperSpeechTranscription")
}

WhisperSpeechTranscription::WhisperSpeechTranscription()
{
   m_language = "en";
   m_wparams.language=m_language.c_str();
   m_wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
   m_model = "ggml-base.en.bin";
}

WhisperSpeechTranscription::~WhisperSpeechTranscription()
{
    close();
}

bool WhisperSpeechTranscription::open(yarp::os::Searchable& config)
{
    if (config.check("threads","number of threads")){
        m_wparams.n_threads = config.find("threads").asInt16();}
    if (config.check("processors", "number of processors")){
        m_wparams.n_threads = config.find("processors").asInt16();}
    if (config.check("initial_prompt")) {
        m_wparams.initial_prompt = config.find("initial_prompt").asString().c_str();}
    if (config.check("duration","duration of audio to process in milliseconds")) {
        m_wparams.duration_ms = config.find("duration").asInt32();}
    if (config.check("offset_ms")) {
        m_wparams.offset_ms = config.find("offset_ms").asInt32();}
    if (config.check("speed_up")) {
        m_wparams.speed_up = config.find("speed_up").asBool();}
    if (config.check("thold_pt","word timestamp probability threshold")) {
        m_wparams.thold_pt = config.find("thold_pt").asFloat32();}
    if (config.check("entropy_thold","entropy threshold for decoder fail")) {
        m_wparams.entropy_thold = config.find("entropy_thold").asFloat32();}
    if (config.check("logprob_thold","log probability threshold for decoder fail")) {
        m_wparams.logprob_thold = config.find("logprob_thold").asFloat32();}
    if (config.check("print_timestamps", "print_timestamps")) {
        m_wparams.logprob_thold = config.find("print_timestamps").asFloat32();}
    if (config.check("model", "file containing the model")) {
        m_model = config.find("model").asString();}
    if (config.check("translate", "translate from source language to English")) {
        m_wparams.translate = config.find("translate").asBool();}
//    if (config.check("diarize", "stereo audio diarization")) {
//        m_wparams.diarize = config.find("diarize").asBool();}
    if(config.check("print_realtime", "print_realtime")) {
        m_wparams.print_realtime = config.find("print_realtime").asBool();}
    if(config.check("print_progress", "print_progress")) {
        m_wparams.print_progress = config.find("print_progress").asBool();}
    if (config.check("split_on_word", "split on word rather than on token")) {
        m_wparams.split_on_word = config.find("split_on_word").asBool();}
    if (config.check("best_of", "number of best candidates to keep")) {
        m_wparams.greedy.best_of = config.find("best_of").asInt32();}
    if (config.check("detect-language", "exit after automatically detecting language")) {
        m_wparams.detect_language = config.find("detect-language").asBool();}
    if (config.check("language", "spoken language ('auto' for auto-detect)")) {
        m_wparams.language = config.find("language").asString().c_str();
        m_language = m_wparams.language;//???
    }
    if (config.check("beam_size", " beam size for beam search")) {
        m_wparams.beam_search.beam_size = config.find("beam_size").asInt32();
        m_wparams.strategy = m_wparams.beam_search.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;
    }
    int32_t max_context = -1;
    int32_t max_len = 0;
    bool no_fallback = false;
    if (config.check("max-context", "maximum number of text context tokens to store")) {
        max_context = config.find("max-context").asInt32();}
    if (config.check("max-len", "maximum segment length in characters")) {
        max_len = config.find("max-len").asInt32();}
    if (config.check("no-fallback", "do not use temperature fallback while decoding")) {
        no_fallback = config.find("no-fallback").asBool();}
    m_wparams.n_max_text_ctx = max_context >= 0 ? max_context : m_wparams.n_max_text_ctx;
    m_wparams.token_timestamps = false || max_len > 0;
    m_wparams.max_len = false && max_len == 0 ? 60 : max_len;
    m_wparams.temperature_inc = no_fallback ? 0.0f : m_wparams.temperature_inc;

    if (m_language != "auto" && whisper_lang_id(m_language.c_str()) == -1)
    {
        yCError(WHISPER_SPEECHTR, "error: unknown language '%s'\n", m_language.c_str());
        return false;
    }

    // whisper init
    if (m_model=="")
    {
        yCError(WHISPER_SPEECHTR, "Please provide full path to the model file with parameter --model\n");
        return false;
    }
    m_ctx = whisper_init_from_file(m_model.c_str());
    if (m_ctx == nullptr)
    {
        yCError(WHISPER_SPEECHTR, "Failed to initialize whisper context\n");
        return false;
    }

    // print system information
    {
        yCInfo(WHISPER_SPEECHTR, "system_info: n_threads = %d / %d | %s\n",
            m_wparams.n_threads * n_processors, std::thread::hardware_concurrency(), whisper_print_system_info());
    }

    // print some info about the processing
    {
        if (!whisper_is_multilingual(m_ctx))
        {
            if (m_wparams.language != "en" || m_wparams.translate)
            {
                m_wparams.language = "en";
                m_wparams.translate = false;
                yCWarning(WHISPER_SPEECHTR,"model is not multilingual, ignoring language and translation options");
            }
        }
        if (m_wparams.detect_language)
        {
            m_wparams.language = "auto";
        }
        yCDebug(WHISPER_SPEECHTR, "%s: processing (%d samples, %.1f sec), %d threads, %d processors, lang = %s, task = %s, timestamps = %d ...\n",
            __func__, int(m_pcmf32.size()), float(m_pcmf32.size()) / WHISPER_SAMPLE_RATE,
            m_wparams.n_threads, n_processors,
            m_wparams.language,
            m_wparams.translate ? "translate" : "transcribe",
            m_wparams.print_timestamps);
    }
    return true;
}

bool WhisperSpeechTranscription::close()
{
    if (m_ctx)
    {
        whisper_free(m_ctx);
    }
    return true;
}

bool WhisperSpeechTranscription::setLanguage(const std::string& language)
{
    m_language=language;
    yCInfo(WHISPER_SPEECHTR) << "Language set to" << language;
    return true;
}

bool WhisperSpeechTranscription::getLanguage(std::string& language)
{
    language = m_language;
    return true;
}

bool WhisperSpeechTranscription::transcribe(const yarp::sig::Sound& sound, std::string& transcription, double& score)
{
    yCDebug(WHISPER_SPEECHTR)<< "received audio" << sound.getSamples();

    score=0;
    transcription="";

    if (sound.getSamples() == 0 ||
        sound.getChannels() == 0)
    {
        yCError(WHISPER_SPEECHTR) << "Invalid Sound sample received";
        return false;
    }

    //copy the audio data;
#if 0
    m_pcmf32.resize(1000);
    m_pcmf32s.clear();
#else
    m_pcmf32.resize(sound.getSamples());
    size_t channel=0;
    for (size_t i = 0; i < m_pcmf32.size(); i++)
    {
        m_pcmf32[i]=float(sound.get(i, channel))/65535.0;
    }
    m_pcmf32s.clear();
#endif

    // run the inference
    {
        if (whisper_full_parallel(m_ctx, m_wparams, m_pcmf32.data(), m_pcmf32.size(), n_processors) != 0)
        {
            yCError(WHISPER_SPEECHTR, "failed to process audio");
            return false;
        }
    }

    // output stuff
    {
        const int n_segments = whisper_full_n_segments(m_ctx);
        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(m_ctx, i);
            transcription += std::string(text);
            yCDebug(WHISPER_SPEECHTR, text);
        }
    }
    score = 1.0;
    return true;
}
