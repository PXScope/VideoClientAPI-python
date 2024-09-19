cmake_minimum_required(VERSION 3.4...3.18)
project(video_client)

add_subdirectory(pybind11)
add_subdirectory(VideoClientAPI)

pybind11_add_module(video_client src/plugins.cpp)

# C++11 표준을 사용합니다
target_compile_features(video_client PRIVATE cxx_std_11)

# 헤더 파일 경로를 추가합니다
target_include_directories(video_client
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoClientAPI/VideoClientAPI/include
)

target_link_libraries(video_client PRIVATE VideoClientAPI)
