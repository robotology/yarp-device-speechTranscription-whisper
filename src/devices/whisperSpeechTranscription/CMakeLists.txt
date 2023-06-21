# SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
# SPDX-License-Identifier: BSD-3-Clause

yarp_prepare_plugin(whisperSpeechTranscription
  CATEGORY device
  TYPE WhisperSpeechTranscription
  INCLUDE whisperSpeechTranscription.h
)

#find_package(whisper)
set(WHISPER_ROOT "" CACHE STRING "Install dir for whisper")
if(NOT WHISPER_ROOT)
  message(FATAL_ERROR "WHISPER_ROOT must be provided and cannot be empty.")
endif()

set(WHISPER_INCLUDE_DIRS ${WHISPER_ROOT}/include)
set(WHISPER_LIB_DIRS ${WHISPER_ROOT}/lib/static)

if(ENABLE_whisperSpeechTranscription)
  yarp_add_plugin(yarp_whisperSpeechTranscription)

  target_sources(yarp_whisperSpeechTranscription
    PRIVATE
      whisperSpeechTranscription.cpp
      whisperSpeechTranscription.h
  )

 target_include_directories(yarp_whisperSpeechTranscription
    PUBLIC
    ${WHISPER_INCLUDE_DIRS}
    )

  target_link_libraries(yarp_whisperSpeechTranscription
    PRIVATE
      YARP::YARP_os
      YARP::YARP_dev
  )

  target_link_libraries(yarp_whisperSpeechTranscription
     PRIVATE
     ${WHISPER_LIB_DIRS}/whisper.lib
  )

  list(APPEND YARP_${YARP_PLUGIN_MASTER}_PRIVATE_DEPS
    YARP_os
    YARP_dev
  )

  yarp_install(
    TARGETS yarp_whisperSpeechTranscription
    EXPORT YARP_${YARP_PLUGIN_MASTER}
    COMPONENT ${YARP_PLUGIN_MASTER}
    LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
    ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR}
    YARP_INI DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR}
  )

  set(YARP_${YARP_PLUGIN_MASTER}_PRIVATE_DEPS ${YARP_${YARP_PLUGIN_MASTER}_PRIVATE_DEPS} PARENT_SCOPE)

  set_property(TARGET yarp_whisperSpeechTranscription PROPERTY FOLDER "Plugins/Device")

  if(YARP_COMPILE_TESTS)
    add_subdirectory(tests)
  endif()

endif()
