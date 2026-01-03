# Kousaten Mixer

Multi-output audio mixer with XY Send Panner automation.

## Features

- **Multi-channel mixing** - Dynamic channel strips with volume, pan, mute/solo
- **Send effects** - Delay, Grain, Reverb per channel
- **Multiple audio outputs** - Route aux buses to different audio devices via RtAudio
- **XY Send Panner** - Distribute sends across aux buses using XY pad
  - Manual XY control
  - Sequencer mode (step through aux buses)
  - Random mode (random jumps)
  - Rotate mode (circular LFO)
  - Path recording (drag to record, release to playback)
  - Double-click to set home position (LFO center)

## Screenshots

*Coming soon*

## Requirements

- macOS 10.15+ (Windows/Linux untested)
- C++17 compiler
- CMake 3.15+

## Dependencies

- [JUCE 7.x](https://juce.com/) - Audio application framework
- [RtAudio 6.x](https://github.com/thestk/rtaudio) - Multi-device audio I/O

## Build

```bash
# Clone with submodules
git clone --recursive https://github.com/mmmmmmmadman/KousatenMixer.git
cd KousatenMixer

# If you forgot --recursive
git submodule update --init --recursive

# Build
cmake -B build
cmake --build build

# Run (macOS)
./build/KousatenMixer_artefacts/Debug/Kousaten\ Mixer.app/Contents/MacOS/Kousaten\ Mixer
```

## Project Structure

```
Source/
├── Main.cpp
├── MainComponent.cpp/.h
├── Core/
│   ├── AudioEngine.cpp/.h
│   └── RtAudioManager.cpp/.h
├── Effects/
│   ├── DelayProcessor.h
│   ├── GrainProcessor.h
│   └── ReverbProcessor.h
├── Mixer/
│   ├── Channel.cpp/.h
│   ├── AuxBus.cpp/.h
│   ├── MixBus.h
│   └── SendPanner.cpp/.h
└── UI/
    ├── ChannelStripComponent.cpp/.h
    ├── AuxOutputComponent.cpp/.h
    └── SendPannerComponent.cpp/.h
```

## License

MIT License - see [LICENSE](LICENSE)

## Author

MADZINE
