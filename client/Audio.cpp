#include "Audio.hpp"

#include <trantor/utils/Logger.h>

#include <portaudio.h>

Audio::Audio() {
    Pa_Initialize();
    Init();
}

Audio::~Audio() { Pa_Terminate(); }

void Audio::Update() {
    input_devices.clear();
    output_devices.clear();

    if (input_stream) {
        Pa_StopStream(input_stream);
    }
    if (output_stream) {
        Pa_StopStream(output_stream);
    }

    input_stream = output_stream = nullptr;
    Init();
}

void Audio::Init() {
    const auto num_devices = DeviceCount();
    if (num_devices < 0) {
        LOG_ERROR << "num_devices = " << num_devices;
        return;
    }

    input_devices.reserve(num_devices);
    output_devices.reserve(num_devices);

    for (int num_device = 0; num_device < num_devices; ++num_device) {
        const auto* device_info_ptr = Pa_GetDeviceInfo(num_device);

        if (device_info_ptr && device_info_ptr->maxInputChannels > 0) {
            LOG_DEBUG << "Device #" << num_device << ": " << device_info_ptr->name
                      << " [Input channels: " << device_info_ptr->maxInputChannels << "]";

            input_devices.emplace_back(num_device);
        }

        if (device_info_ptr && device_info_ptr->maxOutputChannels > 0) {
            LOG_DEBUG << "Device #" << num_device << ": " << device_info_ptr->name
                      << " [Output channels: " << device_info_ptr->maxInputChannels << "]";

            output_devices.emplace_back(num_device);
        }
    }
}

int Audio::DeviceCount() const noexcept { return Pa_GetDeviceCount(); }

const PaDeviceInfo* Audio::GetDeviceInfo(const PaDeviceIndex index) const noexcept { return Pa_GetDeviceInfo(index); }

const PaDeviceIndex Audio::GetDefaultInputDeviceIndex() const noexcept { return Pa_GetDefaultInputDevice(); }

const PaDeviceIndex Audio::GetDefaultOutputDeviceIndex() const noexcept { return Pa_GetDefaultOutputDevice(); }

const PaDeviceInfo* Audio::GetDefaultInputDevice() const noexcept {
    const auto device_number = GetDefaultInputDeviceIndex();
    return GetDeviceInfo(device_number);
}

const PaDeviceInfo* Audio::GetDefaultOutputDevice() const noexcept {
    const auto device_number = GetDefaultOutputDeviceIndex();
    return GetDeviceInfo(device_number);
}

const std::vector<size_t>& Audio::GetInputDevices() const noexcept { return input_devices; }

const std::vector<size_t>& Audio::GetOutputDevices() const noexcept { return output_devices; }

void Audio::CreateDefaultInputStream() {
    const auto device_index = GetDefaultInputDeviceIndex();
    CreateInputStream(device_index);
}

void Audio::CreateDefaultOutputStream() {
    const auto device_index = GetDefaultOutputDeviceIndex();
    CreateOutputStream(device_index);
}

void Audio::CreateInputStream(const PaDeviceIndex device_index) {
    PaStreamParameters input_params;
    input_params.device = device_index;
    input_params.channelCount = NUM_CHANNELS;
    input_params.sampleFormat = paInt16;
    input_params.suggestedLatency = GetDeviceInfo(device_index)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = nullptr;

    auto err = Pa_OpenStream(
        &input_stream, &input_params, nullptr, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, nullptr, nullptr
    );
    if (err != paNoError) {
        LOG_ERROR << "Failed to open input stream: " << Pa_GetErrorText(err);
        return;
    }

    Pa_StartStream(input_stream);
    if (err != paNoError) {
        LOG_ERROR << "Failed to start input stream: " << Pa_GetErrorText(err);
        return;
    }
    LOG_INFO << "Input stream" << device_index << " started";
}

void Audio::CreateOutputStream(const PaDeviceIndex device_index) {
    PaStreamParameters output_params;
    output_params.device = device_index;
    output_params.channelCount = NUM_CHANNELS;
    output_params.sampleFormat = paInt16;
    output_params.suggestedLatency = GetDeviceInfo(device_index)->defaultHighOutputLatency;
    output_params.hostApiSpecificStreamInfo = nullptr;

    auto err = Pa_OpenStream(
        &output_stream, nullptr, &output_params, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, nullptr, nullptr
    );
    if (err != paNoError) {
        LOG_ERROR << "Failed to open output stream: " << Pa_GetErrorText(err);
        return;
    }

    Pa_StartStream(output_stream);
    if (err != paNoError) {
        LOG_ERROR << "Failed to start output stream: " << Pa_GetErrorText(err);
        return;
    }
    LOG_INFO << "Output stream" << device_index << " started";
}

const void Audio::GetInputStreamBuffer(SAMPLE* input_buffer) {
    const auto err = Pa_ReadStream(input_stream, input_buffer, FRAMES_PER_BUFFER);
    if (err != paNoError) {
        LOG_ERROR << "Failed to read stream: " << Pa_GetErrorText(err);
    }
}

const void Audio::SetOutputStreamBuffer(const SAMPLE* output_buffer) {
    const auto err = Pa_WriteStream(output_stream, output_buffer, FRAMES_PER_BUFFER);
    if (err != paNoError) {
        LOG_ERROR << "Failed to write stream: " << Pa_GetErrorText(err);
    }
}