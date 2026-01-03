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

## Installation

### macOS

1. Download `KousatenMixer-macOS.zip` from [Releases](https://github.com/mmmmmmmadman/KousatenMixer/releases)
2. Unzip the file
3. Move `Kousaten Mixer.app` to `/Applications`
4. First launch: Right-click > Open (to bypass Gatekeeper)
5. Grant microphone permission when prompted

### Windows

1. Download `KousatenMixer-Windows.zip` from [Releases](https://github.com/mmmmmmmadman/KousatenMixer/releases)
2. Unzip the file
3. Run `Kousaten Mixer.exe`

## Usage

### Basic Operation

1. **Add channels** - Click "Add Channel" button at bottom
2. **Select audio input** - Use dropdown in each channel strip
3. **Adjust levels** - Volume fader, Pan knob, Mute/Solo buttons
4. **Add effects** - Adjust Delay/Grain/Reverb send levels per channel

### Aux Outputs

1. **Add aux output** - Click "Add Aux" in the Aux Output section
2. **Select device** - Choose output device from dropdown (e.g., headphones, speakers)
3. **Rename** - Double-click aux name to rename

### Send Panner (per channel)

The XY Pad distributes audio across aux buses based on position.

| Control | Action |
|---------|--------|
| Drag | Move panner position / Record path |
| Double-click | Set home position (LFO center) |
| Release after drag | Start path playback |

**Modes:**
- **Manual** - Direct XY control
- **Sequence** - Step through recorded path or aux positions
- **Random** - Random jumps between aux buses
- **Rotate** - Circular movement around aux positions

**Parameters:**
- **Speed** - Automation speed (0.1-10 Hz)
- **Smooth** - Transition smoothness (instant to 2 seconds)
- **Amount** - Panning depth (0% = uniform, 100% = full panning)

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| M | Mute selected channel |
| S | Solo selected channel |

## Requirements (for building)

- macOS 10.15+ / Windows 10+
- C++17 compiler
- CMake 3.15+

## Dependencies

- [JUCE 7.x](https://juce.com/) - Audio application framework
- [RtAudio 6.x](https://github.com/thestk/rtaudio) - Multi-device audio I/O

## Build from Source

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
