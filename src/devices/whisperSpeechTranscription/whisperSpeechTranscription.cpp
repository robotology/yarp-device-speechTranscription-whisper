/*
 * SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "whisperSpeechTranscription.h"

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/LogComponent.h>

#include <algorithm>
#include <cmath>
#include <regex>
#include <thread>

using namespace yarp::os;
using namespace yarp::dev;

namespace {
YARP_LOG_COMPONENT(WHISPER_SPEECHTR, "yarp.device.WhisperSpeechTranscription")

constexpr double kPi = 3.14159265358979323846;

/**
 * Converts a (possibly multi-channel) yarp Sound into the mono, float, [-1, 1)
 * PCM stream at WHISPER_SAMPLE_RATE expected by whisper.
 * Channels are averaged; if the sound has a different sample rate it is
 * resampled with a band-limited (Blackman-windowed sinc) interpolator, which
 * also acts as the anti-aliasing low-pass filter when downsampling.
 */
void soundToWhisperPcm(const yarp::sig::Sound& sound, std::vector<float>& out)
{
    const size_t n_samples = sound.getSamples();
    const size_t n_channels = sound.getChannels();

    // 1) downmix to mono float. yarp samples are int16, so full scale is 32768.
    std::vector<float> mono(n_samples, 0.0f);
    for (size_t i = 0; i < n_samples; i++)
    {
        float acc = 0.0f;
        for (size_t c = 0; c < n_channels; c++)
        {
            acc += static_cast<float>(sound.get(i, c));
        }
        mono[i] = acc / (32768.0f * static_cast<float>(n_channels));
    }

    // 2) resample to WHISPER_SAMPLE_RATE if needed
    int in_rate = sound.getFrequency();
    if (in_rate <= 0)
    {
        yCWarning(WHISPER_SPEECHTR, "Sound has no valid sample rate, assuming %d Hz", WHISPER_SAMPLE_RATE);
        in_rate = WHISPER_SAMPLE_RATE;
    }
    if (in_rate == WHISPER_SAMPLE_RATE)
    {
        out = std::move(mono);
        return;
    }

    const double ratio = static_cast<double>(WHISPER_SAMPLE_RATE) / in_rate;
    const double cutoff = std::min(1.0, ratio);       // relative to the input Nyquist frequency
    constexpr int zero_crossings = 16;
    const double half_width = zero_crossings / cutoff; // kernel half width, in input samples
    const size_t n_out = static_cast<size_t>(std::floor(n_samples * ratio));

    out.assign(n_out, 0.0f);
    for (size_t n = 0; n < n_out; n++)
    {
        const double t = n / ratio; // position in the input stream
        const long first = std::max(0L, static_cast<long>(std::ceil(t - half_width)));
        const long last = std::min(static_cast<long>(n_samples) - 1, static_cast<long>(std::floor(t + half_width)));
        double acc = 0.0;
        for (long k = first; k <= last; k++)
        {
            const double x = k - t;
            const double arg = kPi * cutoff * x;
            const double sinc = (std::abs(arg) < 1e-12) ? 1.0 : std::sin(arg) / arg;
            const double w = 0.5 + 0.5 * x / half_width; // in [0, 1]
            const double window = 0.42 - 0.5 * std::cos(2.0 * kPi * w) + 0.08 * std::cos(4.0 * kPi * w);
            acc += mono[k] * cutoff * sinc * window;
        }
        out[n] = static_cast<float>(acc);
    }
    yCDebug(WHISPER_SPEECHTR, "Resampled audio from %d Hz to %d Hz", in_rate, WHISPER_SAMPLE_RATE);
}
} // namespace

WhisperSpeechTranscription::WhisperSpeechTranscription() :
        m_wparams(whisper_full_default_params(WHISPER_SAMPLING_GREEDY))
{
}

WhisperSpeechTranscription::~WhisperSpeechTranscription()
{
    close();
}

bool WhisperSpeechTranscription::isLanguageSupported(const std::string& language) const
{
    if (language == "auto")
    {
        return m_ctx == nullptr || whisper_is_multilingual(m_ctx);
    }
    if (whisper_lang_id(language.c_str()) == -1)
    {
        return false;
    }
    return m_ctx == nullptr || whisper_is_multilingual(m_ctx) || language == "en";
}

bool WhisperSpeechTranscription::open(yarp::os::Searchable& config)
{
    if(!parseParams(config))
    {
        yCError(WHISPER_SPEECHTR) << "Could not parse the parameters correctly";
        return false;
    }
    if (config.check("threads","number of threads")){
        m_wparams.n_threads = config.find("threads").asInt32();}
    if (config.check("processors", "number of processors")){
        m_n_processors = std::max(1, config.find("processors").asInt32());}
    if (config.check("initial_prompt")) {
        m_initial_prompt = config.find("initial_prompt").asString();
        m_wparams.initial_prompt = m_initial_prompt.c_str();}
    if (config.check("duration","duration of audio to process in milliseconds")) {
        m_wparams.duration_ms = config.find("duration").asInt32();}
    if (config.check("offset_ms")) {
        m_wparams.offset_ms = config.find("offset_ms").asInt32();}
    if (config.check("thold_pt","word timestamp probability threshold")) {
        m_wparams.thold_pt = config.find("thold_pt").asFloat32();}
    if (config.check("entropy_thold","entropy threshold for decoder fail")) {
        m_wparams.entropy_thold = config.find("entropy_thold").asFloat32();}
    if (config.check("logprob_thold","log probability threshold for decoder fail")) {
        m_wparams.logprob_thold = config.find("logprob_thold").asFloat32();}
    if (config.check("print_timestamps", "print_timestamps")) {
        m_wparams.print_timestamps = config.find("print_timestamps").asBool();}
    if (config.check("translate", "translate from source language to English")) {
        m_wparams.translate = config.find("translate").asBool();}
    if(config.check("print_realtime", "print_realtime")) {
        m_wparams.print_realtime = config.find("print_realtime").asBool();}
    if(config.check("print_progress", "print_progress")) {
        m_wparams.print_progress = config.find("print_progress").asBool();}
    if (config.check("split_on_word", "split on word rather than on token")) {
        m_wparams.split_on_word = config.find("split_on_word").asBool();}
    if (config.check("best_of", "number of best candidates to keep")) {
        m_wparams.greedy.best_of = config.find("best_of").asInt32();}
    if (config.check("beam_size", " beam size for beam search")) {
        m_wparams.beam_search.beam_size = config.find("beam_size").asInt32();
        m_wparams.strategy = m_wparams.beam_search.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;
    }
    if (config.check("max-context", "maximum number of text context tokens to store")) {
        const int32_t max_context = config.find("max-context").asInt32();
        if (max_context >= 0) {
            m_wparams.n_max_text_ctx = max_context;
        }
    }
    if (config.check("max-len", "maximum segment length in characters")) {
        m_wparams.max_len = config.find("max-len").asInt32();
        m_wparams.token_timestamps = m_wparams.max_len > 0;
    }
    if (config.check("no-fallback", "do not use temperature fallback while decoding") && config.find("no-fallback").asBool()) {
        m_wparams.temperature_inc = 0.0f;
    }
    if (config.check("detect-language")) {
        yCWarning(WHISPER_SPEECHTR) << "'detect-language' is ignored: set 'language' to 'auto' to enable automatic language detection";
    }

    if (!isLanguageSupported(m_language))
    {
        yCError(WHISPER_SPEECHTR, "unknown language '%s'", m_language.c_str());
        return false;
    }

    // whisper init
    std::string model_path;
    if (!m_model_context.empty())
    {
        yarp::os::ResourceFinder resource_finder;
        resource_finder.setDefaultContext(m_model_context);
        model_path = resource_finder.findFileByName(m_model_name);
    }
    else
    {
        model_path = m_model_name;
    }
    if (model_path.empty())
    {
        yCError(WHISPER_SPEECHTR, "Unable to find model '%s' (context: '%s')", m_model_name.c_str(), m_model_context.c_str());
        return false;
    }
    whisper_context_params cparams = whisper_context_default_params();
    m_ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (m_ctx == nullptr)
    {
        yCError(WHISPER_SPEECHTR, "Failed to initialize whisper context from '%s'", model_path.c_str());
        return false;
    }

    yCInfo(WHISPER_SPEECHTR, "system_info: n_threads = %d / %d | %s",
        m_wparams.n_threads * m_n_processors, std::thread::hardware_concurrency(), whisper_print_system_info());

    if (!whisper_is_multilingual(m_ctx))
    {
        if (m_language != "en" || m_wparams.translate)
        {
            m_language = "en";
            m_wparams.translate = false;
            yCWarning(WHISPER_SPEECHTR, "model is not multilingual, ignoring language and translation options");
        }
    }
    yCInfo(WHISPER_SPEECHTR, "Model '%s' loaded: %d threads, %d processors, lang = %s, task = %s",
        model_path.c_str(), m_wparams.n_threads, m_n_processors, m_language.c_str(),
        m_wparams.translate ? "translate" : "transcribe");
    return true;
}

bool WhisperSpeechTranscription::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ctx)
    {
        whisper_free(m_ctx);
        m_ctx=nullptr;
    }
    return true;
}

ReturnValue WhisperSpeechTranscription::setLanguage(const std::string& language)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!isLanguageSupported(language))
    {
        yCError(WHISPER_SPEECHTR) << "Language" << language << "is not supported by the loaded model";
        return ReturnValue::return_code::return_value_error_input_out_of_bounds;
    }
    m_language=language;
    yCInfo(WHISPER_SPEECHTR) << "Language set to" << language;
    return ReturnValue_ok;
}

ReturnValue WhisperSpeechTranscription::getLanguage(std::string& language)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    language = m_language;
    return ReturnValue_ok;
}

ReturnValue WhisperSpeechTranscription::transcribe(const yarp::sig::Sound& sound, std::string& transcription, double& score)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    yCDebug(WHISPER_SPEECHTR)<< "received audio" << sound.getSamples();

    score=0;
    transcription="";

    if (m_ctx == nullptr)
    {
        yCError(WHISPER_SPEECHTR) << "Device not opened";
        return ReturnValue::return_code::return_value_error_not_ready;
    }

    if (sound.getSamples() == 0 ||
        sound.getChannels() == 0)
    {
        yCError(WHISPER_SPEECHTR) << "Invalid Sound sample received";
        return ReturnValue::return_code::return_value_error_method_failed;
    }

    soundToWhisperPcm(sound, m_pcmf32);

    // m_language is only modified under m_mutex, so the pointer stays valid during inference
    m_wparams.language = m_language.c_str();

    // run the inference
    if (whisper_full_parallel(m_ctx, m_wparams, m_pcmf32.data(), static_cast<int>(m_pcmf32.size()), m_n_processors) != 0)
    {
        yCError(WHISPER_SPEECHTR, "failed to process audio");
        return ReturnValue::return_code::return_value_error_method_failed;
    }

    // output stuff
    const int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(m_ctx, i);
        transcription += std::string(text);
        yCDebug(WHISPER_SPEECHTR, "%s", text);
    }

    //remove symbols such as [bla bla]
    if (m_remove_symbols == 1)
    {
        static const std::regex pattern1("\\[[^\\]]*\\]");
        transcription = std::regex_replace(transcription, pattern1, "");
    }

    //assign the score
    score = 1.0;
    if (transcription.empty()) {score = 0.0;}
    return ReturnValue_ok;
}
