# SPDX-FileCopyrightText: 2023-2023 Istituto Italiano di Tecnologia (IIT)
# SPDX-License-Identifier: BSD-3-Clause

set(WHISPER_ROOT "" CACHE STRING "Install dir for whisper")
if(NOT WHISPER_ROOT)
  message(WARNING "WHISPER_ROOT must be provided and cannot be empty.")
else()
  set(WHISPER_INCLUDE_DIRS ${WHISPER_ROOT}/include)
  set(WHISPER_LIB_DIRS ${WHISPER_ROOT}/lib/static)
  set(YARP_HAS_Whisper true)
endif()

yarp_prepare_plugin(whisperSpeechTranscription
  CATEGORY device
  TYPE WhisperSpeechTranscription
  INCLUDE whisperSpeechTranscription.h
  DEPENDS "YARP_HAS_Whisper"
  DEFAULT ON
)

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

if(WIN32)
  target_link_libraries(yarp_whisperSpeechTranscription
     PRIVATE
     ${WHISPER_LIB_DIRS}/whisper.lib)
else()
  target_link_libraries(yarp_whisperSpeechTranscription
     PRIVATE
     ${WHISPER_LIB_DIRS}/libwhisper.a)
endif()

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
    add_subdirectory(demos)
  endif()

endif()
