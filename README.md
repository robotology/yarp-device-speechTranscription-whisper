![YARP logo](doc/images/yarp-robot-24.png?raw=true "yarp-device-speechTranscription-whisper")
Yarp device for Whisper
=====================

This repository contains the yarp-device-speechTranscription-whisper plugin.

:construction: This repository is currently work in progress. :construction:
:construction: The software contained is this repository is currently under testing. :construction:
:construction: APIs may change without any warning. :construction:

Documentation
-------------

Documentation of the nws/nwc devices is provided in the official Yarp documentation page:
[![YARP documentation](https://img.shields.io/badge/Documentation-yarp.it-19c2d8.svg)](https://yarp.it/latest/group__dev__impl.html)

Documentation of the interface API is provided in the official Yarp documentation page:
[![YARP documentation](https://img.shields.io/badge/Documentation-yarp.it-19c2d8.svg)](https://yarp.it/latest/group__dev__impl.html)

Documentation of the audio in Yarp.
https://yarp.it/latest/group__AudioDoc.html

Installation
-------------

### Step1: Build whisper.cpp library

~~~bash
# Clone whisper.cpp repository, choose an install dir, make it and install it to the chosen install dir.
# Please note that whisper.cpp has several build options. Some of them e.g. the use of the GPU may affect
# performances significantly. The commands reported below refers only to default configuration.
# Please check the documentation on the official page github page.
# ${ROBOT_CODE} is the root directory of your choice.
# ~/my_whispercpp_installation_dir is a directory of your choice (where the whispercpp library will be installed)

 cd ${ROBOT_CODE}
 git clone https://github.com/ggerganov/whisper.cpp whispercpp
 cd whispercpp
 mkdir build
 cd build
 cmake -GNinja -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=~/my_whispercpp_installation_dir ..
 cmake --build .
 cmake --install .
~~~

### Step 2: Build the yarp device
~~~bash
 cd ${ROBOT_CODE}
 git clone https://github.com/robotology/yarp-device-speechTranscription-whisper
 cd yarp-device-speechTranscription-whisper
 mkdir build
 cd build
 cmake -GNinja -DWHISPER_ROOT=~/my_whispercpp_installation_dir ..
 cmake --build .
~~~

### Step 3: Install the model(s)
~~~bash
# Here is a list (not complete) of possible whisper models: tiny.en, tiny, base.en, base, small.en, small, medium.en, medium, large-v1, large
  wget -P ${ROBOT_CODE}/yarp-device-speechTranscription-whisper/build/share/WhisperTranscribe/contexts/whisperTranscribe_demo/ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
~~~

Usage
-------------
`yarp-device-speechTranscription-whisper` is a yarp device and it cannot be executed as a standalone module.
It requires to be attached to an nws device, e.g. `SpeechTranscription_nws_yarp` and requires a
yarprobotinteface configuration file to be correctly instantiated.

Check the demo examples provided in:
https://github.com/robotology/yarp-device-speechTranscription-whisper/tree/master/src/devices/whisperSpeechTranscription/demos
demo_audio_from_file.xml demonstrate how to transcribe audio from a recorded file.
demo_audio_from_mic.xml demonstrate how to transcribe audio from a microphone. The audio is also optionally recorded to a file.
The transcribed text is provided on the port /speechTranscription_nws/text:o

CI Status
---------

:construction: This repository is currently work in progress. :construction:

[![Build Status](https://github.com/robotology/yarp-device-speechTranscription-whisper/workflows/CI%20Workflow/badge.svg)](https://github.com/robotology/yarp-device-speechTranscription-whisper/actions?query=workflow%3A%22CI+Workflow%22)

License
---------

:construction: This repository is currently work in progress. :construction:

Maintainers
--------------
This repository is maintained by:

| | |
|:---:|:---:|
| [<img src="https://github.com/randaz81.png" width="40">](https://github.com/randaz81) | [@randaz81](https://github.com/randaz81) |
