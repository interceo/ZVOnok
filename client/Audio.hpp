#pragma once
#include <array>
#include <vector>

#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_SECONDS 5
#define NUM_CHANNELS 1

using SAMPLE = short;

#define BUF_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS)

class Audio {
    std::vector<size_t> input_devices;
    std::vector<size_t> output_devices;

    PaStream* input_stream{NULL};
    PaStream* output_stream{NULL};

public:
    Audio();
    ~Audio();

    void Update();

    int DeviceCount() const noexcept;
    const PaDeviceInfo* GetDeviceInfo(const PaDeviceIndex index) const noexcept;
    const PaDeviceIndex GetDefaultInputDeviceIndex() const noexcept;
    const PaDeviceIndex GetDefaultOutputDeviceIndex() const noexcept;
    const PaDeviceInfo* GetDefaultInputDevice() const noexcept;
    const PaDeviceInfo* GetDefaultOutputDevice() const noexcept;

    const std::vector<size_t>& GetInputDevices() const noexcept;
    const std::vector<size_t>& GetOutputDevices() const noexcept;

    void CreateDefaultInputStream();
    void CreateDefaultOutputStream();
    void CreateInputStream(const PaDeviceIndex device_index);
    void CreateOutputStream(const PaDeviceIndex device_index);
    const void GetInputStreamBuffer(SAMPLE* input_buffer);
    const void SetOutputStreamBuffer(const SAMPLE* output_buffer);

private:
    void Init();
    void CreateStream(const PaDeviceIndex device_index, PaStream* stream);
};
