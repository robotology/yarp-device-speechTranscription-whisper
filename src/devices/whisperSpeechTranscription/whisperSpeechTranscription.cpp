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

using namespace yarp::os;
using namespace yarp::dev;

namespace {
YARP_LOG_COMPONENT(WHISPER_SPEECHTR, "yarp.device.WhisperSpeechTranscription")
}
/*
struct whisper_params {
    int32_t n_threads = std::min(4, (int32_t)std::thread::hardware_concurrency());
    int32_t max_context = -1;
    int32_t max_len = 0;
    int32_t best_of = 2;
    int32_t beam_size = -1;

    bool translate = false;
    bool detect_language = false;
    bool split_on_word = false;
    bool no_fallback = false;
    bool output_txt = false;
    bool print_progress = false;

    std::string language = "en";
};
*/

/*
bool whisper_params_parse(int argc, char** argv, whisper_params& params)
{
        else if (arg == "-mc" || arg == "--max-context") { params.max_context = std::stoi(argv[++i]); }
        else if (arg == "-ml" || arg == "--max-len") { params.max_len = std::stoi(argv[++i]); }
        else if (arg == "-bo" || arg == "--best-of") { params.best_of = std::stoi(argv[++i]); }
        else if (arg == "-bs" || arg == "--beam-size") { params.beam_size = std::stoi(argv[++i]); }
        else if (arg == "-tr" || arg == "--translate") { params.translate = true; }
        else if (arg == "-sow" || arg == "--split-on-word") { params.split_on_word = true; }
        else if (arg == "-nf" || arg == "--no-fallback") { params.no_fallback = true; }
        else if (arg == "-pp" || arg == "--print-progress") { params.print_progress = true; }
        else if (arg == "-l" || arg == "--language") { params.language = argv[++i]; }
        else if (arg == "-dl" || arg == "--detect-language") { params.detect_language = true; }
    return true;
}
*/

/*
void whisper_print_usage(int argc, char** argv, const whisper_params& params)
{
    fprintf(stderr, "  -mc N,     --max-context N     [%-7d] maximum number of text context tokens to store\n", params.max_context);
    fprintf(stderr, "  -ml N,     --max-len N         [%-7d] maximum segment length in characters\n", params.max_len);
    fprintf(stderr, "  -sow,      --split-on-word     [%-7s] split on word rather than on token\n", params.split_on_word ? "true" : "false");
    fprintf(stderr, "  -bo N,     --best-of N         [%-7d] number of best candidates to keep\n", params.best_of);
    fprintf(stderr, "  -bs N,     --beam-size N       [%-7d] beam size for beam search\n", params.beam_size);
    fprintf(stderr, "  -nf,       --no-fallback       [%-7s] do not use temperature fallback while decoding\n", params.no_fallback ? "true" : "false");
    fprintf(stderr, "  -pp,       --print-progress    [%-7s] print progress\n", params.print_progress ? "true" : "false");
    fprintf(stderr, "  -l LANG,   --language LANG     [%-7s] spoken language ('auto' for auto-detect)\n", params.language.c_str());
    fprintf(stderr, "  -dl,       --detect-language   [%-7s] exit after automatically detecting language\n", params.detect_language ? "true" : "false");
    fprintf(stderr, "\n");
}
*/

WhisperSpeechTranscription::WhisperSpeechTranscription()
{
   m_wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
   m_model = "C:\\Software\\yarp_software\\whisper_cpp_sw\\models\\ggml-base.en.bin";
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
        m_model = config.find("model").asFloat32();}
    if (config.check("translate", "translate from source language to English")) {
        m_wparams.translate = config.find("translate").asBool();}
//    if (config.check("diarize", "stereo audio diarization")) {
//        m_wparams.diarize = config.find("diarize").asBool();
//    }

    if (m_language != "auto" && whisper_lang_id(m_language.c_str()) == -1)
    {
        yCError(WHISPER_SPEECHTR, "error: unknown language '%s'\n", m_language.c_str());
        return false;
    }

    // whisper init
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
        fprintf(stderr, "%s: processing (%d samples, %.1f sec), %d threads, %d processors, lang = %s, task = %s, timestamps = %d ...\n",
            __func__, int(m_pcmf32.size()), float(m_pcmf32.size()) / WHISPER_SAMPLE_RATE,
            m_wparams.n_threads, n_processors,
            m_wparams.language,
            m_wparams.translate ? "translate" : "transcribe",
            m_wparams.print_timestamps);
    }

   /*
    m_wparams.strategy = params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;
    m_wparams.print_realtime = false;
    m_wparams.print_progress = params.print_progress;
    m_wparams.translate = params.translate;
    m_wparams.language = params.language.c_str();
    m_wparams.detect_language = params.detect_language;
    m_wparams.n_max_text_ctx = params.max_context >= 0 ? params.max_context : m_wparams.n_max_text_ctx;
    m_wparams.token_timestamps = false || params.max_len > 0;
    m_wparams.max_len = false && params.max_len == 0 ? 60 : params.max_len;
    m_wparams.split_on_word = params.split_on_word;
    m_wparams.greedy.best_of = params.best_of;
    m_wparams.beam_search.beam_size = params.beam_size;
    m_wparams.temperature_inc = params.no_fallback ? 0.0f : m_wparams.temperature_inc;
*/

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

bool WhisperSpeechTranscription::setLanguage(const std::string language)
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
/*
        whisper_print_user_data user_data = { &params, &m_pcmf32s };

        // example for abort mechanism
        // in this example, we do not abort the processing, but we could if the flag is set to true
        // the callback is called before every encoder run - if it returns false, the processing is aborted
        {
            static bool is_aborted = false; // NOTE: this should be atomic to avoid data race

            wparams.encoder_begin_callback = [](struct whisper_context*, struct whisper_state*, void* user_data)
            {
                bool is_aborted = *(bool*)user_data;
                return !is_aborted;
            };
            wparams.encoder_begin_callback_user_data = &is_aborted;
        }
*/

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
