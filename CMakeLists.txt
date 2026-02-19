cmake_minimum_required(VERSION 3.10)
project(
  EduRobot
  VERSION 1.0
  LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(SOURCES "${SRC_DIR}/main.cpp"
            "${SRC_DIR}/audio/stt/audio_capture.cpp"
            "${SRC_DIR}/websocket/websocket_client.cpp"
            "${SRC_DIR}/edurobotflow/edu_robot.cpp"
            "${SRC_DIR}/database/database_client.cpp"
            "${SRC_DIR}/chatbot/chatbot_client.cpp"
            "${SRC_DIR}/rasa/rasa_client.cpp"
            "${SRC_DIR}/gui/animator.cpp"
            "${SRC_DIR}/gui/display_thread.cpp"
            "${SRC_DIR}/gui/audio_visualizer.cpp"
            "${SRC_DIR}/audio/tts/tts_client.cpp"
            "${SRC_DIR}/helpers/translate.cpp"
            "${SRC_DIR}/helpers/env_utils.cpp"
            )
            
add_executable(EduRobot ${SOURCES})
target_compile_definitions(EduRobot PRIVATE
    GOOGLE_APPLICATION_CREDENTIALS=\"${CMAKE_SOURCE_DIR}/src/audio/tts/gen-lang-client.json\")

find_package(CURL)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

pkg_check_modules(LWS REQUIRED libwebsockets)
pkg_check_modules(MYSQL REQUIRED libmariadb) 
pkg_check_modules(SDL2       REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

target_include_directories(EduRobot PRIVATE 
                                            "${SRC_DIR}"
                                            "${SRC_DIR}/audio/stt"
                                            "${SRC_DIR}/audio/tts"
                                            "${SRC_DIR}/websocket"
                                            "${SRC_DIR}/database"
                                            "${SRC_DIR}/edurobotflow"
                                            "${SRC_DIR}/rasaclient"
                                            "${SRC_DIR}/chatbot"
                                            "${SRC_DIR}/rasa"
                                            "${SRC_DIR}/gui"
                                            "${SRC_DIR}/helpers"
                                            "${SRC_DIR}/src/Globals"
                          )

target_include_directories(
  EduRobot PRIVATE ${PORTAUDIO_INCLUDE_DIRS} 
                   ${LWS_INCLUDE_DIRS}
                   ${CMAKE_SOURCE_DIR}/thirdparty/
                   ${SDL2_INCLUDE_DIRS}
                   ${SDL2_IMAGE_INCLUDE_DIRS}
                   ${SDL2_MIXER_INCLUDE_DIRS}
                   ${SDL2_TTF_INCLUDE_DIRS}
                   ${MYSQL_INCLUDE_DIRS}
  )


target_compile_options(
  EduRobot PRIVATE ${PORTAUDIO_CFLAGS_OTHER} 
                   ${LWS_CFLAGS_OTHER}
                   ${SDL2_CFLAGS_OTHER}
                   ${SDL2_IMAGE_CFLAGS_OTHER}
                   ${SDL2_MIXER_CFLAGS_OTHER}
                   ${SDL2_TTF_CFLAGS_OTHER}
                   ${MYSQL_CFLAGS_OTHER}
                   )

target_link_directories(EduRobot PRIVATE 
                        ${PORTAUDIO_LIBRARY_DIRS}
                        ${LWS_LIBRARY_DIRS}
                        ${MYSQL_LIBRARY_DIRS}
                        ${CURL_INCLUDE_DIRS}
                        ${SDL2_LIBRARY_DIRS}
                        ${SDL2_IMAGE_LIBRARY_DIRS}
                        ${SDL2_TTF_LIBRARY_DIRS}
                        ${SDL2_MIXER_LIBRARY_DIRS}
                        )
target_link_libraries(
  EduRobot
  PRIVATE Threads::Threads
          ${PORTAUDIO_LIBRARIES} 
          ${LWS_LIBRARIES} 
          ${MYSQL_LIBRARIES}
          OpenSSL::SSL
          OpenSSL::Crypto
          CURL::libcurl   
          ${SDL2_LIBRARIES}
          ${SDL2_IMAGE_LIBRARIES}
          ${SDL2_MIXER_LIBRARIES}
          ${SDL2_TTF_LIBRARIES}
)

if(MSVC)
  target_compile_options(EduRobot PRIVATE /W4 /permissive-)
else()
  target_compile_options(EduRobot PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
