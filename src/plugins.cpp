#include <pybind11/pybind11.h>
#include "video_client_api.h"

namespace py = pybind11;

// 불투명 핸들을 위한 커스텀 타입
struct VideoClientHandle {
    video_client ptr;
    explicit VideoClientHandle(video_client p) : ptr(p) {}
};

// 전역 Python 콜백 저장소
std::unordered_map<video_client, py::function> g_py_data_callbacks;
std::unordered_map<video_client, py::function> g_py_disconnect_callbacks;

// C++ 데이터 콜백 함수
void cpp_data_callback(video_client ctx, uint8_t* data, size_t size, void* frame_info) {
    py::gil_scoped_acquire acquire;
    auto it = g_py_data_callbacks.find(ctx);
    if (it != g_py_data_callbacks.end()) {
        try {
            py::bytes py_data(reinterpret_cast<char*>(data), size);
            it->second(VideoClientHandle(ctx), py_data, size, reinterpret_cast<std::uintptr_t>(frame_info));
        } catch (py::error_already_set &e) {
            std::cerr << "Python data callback error: " << e.what() << std::endl;
        }
    }
}

// C++ 연결 해제 콜백 함수
void cpp_disconnect_callback(video_client ctx, int code, const char* msg) {
    py::gil_scoped_acquire acquire;
    auto it = g_py_disconnect_callbacks.find(ctx);
    if (it != g_py_disconnect_callbacks.end()) {
        try {
            it->second(VideoClientHandle(ctx), code, msg);
        } catch (py::error_already_set &e) {
            std::cerr << "Python disconnect callback error: " << e.what() << std::endl;
        }
    }
}

PYBIND11_MODULE(video_client, m) {
    m.doc() = "VideoClientAPI Python version";

    py::class_<VideoClientHandle>(m, "VideoClient")
        .def(py::init([]() {
            video_client client = create_video_client();
            return std::unique_ptr<VideoClientHandle>(new VideoClientHandle(client));
        }))
        .def("__del__", [](VideoClientHandle& self) {
            if (self.ptr) {
                release_video_client(self.ptr);
                self.ptr = nullptr;
            }
        });

    // function
    m.def("create_video_client", []() {
        video_client client = create_video_client();
        return std::unique_ptr<VideoClientHandle>(new VideoClientHandle(client));
    }, py::return_value_policy::take_ownership);
    m.def("release_video_client", [](VideoClientHandle& handle) {
        if (handle.ptr) {
            release_video_client(handle.ptr);
            handle.ptr = nullptr;
        }
    });
    m.def("connect_video_client", [](VideoClientHandle& handle, const char* url, float timeout_sec, py::function callback) {
        g_py_disconnect_callbacks[handle.ptr] = callback;
        return connect_video_client(handle.ptr, url, timeout_sec, cpp_disconnect_callback);
    });
    m.def("disconnect_video_client", [](VideoClientHandle& handle) {
        return disconnect_video_client(handle.ptr);
    });
    m.def("stop_video_client", [](VideoClientHandle& handle) {
        return stop_video_client(handle.ptr);
    });
    m.def("start_video_client", [](VideoClientHandle& handle, videoproc_context vp_ctx, py::function callback) {
        g_py_data_callbacks[handle.ptr] = callback;
        return start_video_client(handle.ptr, vp_ctx, cpp_data_callback);
    });
    m.def("stop_video_client", [](VideoClientHandle& handle) {
        return stop_video_client(handle.ptr);
    });
    m.def("set_max_queue_size", [](VideoClientHandle& handle, size_t size) {
        return set_max_queue_size(handle.ptr, size);
    });

    m.def("api_init", &api_init);

    // attribute
    py::enum_<api_err_t>(m, "ApiError")
    .value("SUCCESS", api_err_t::SUCCESS)
    .value("INVALID_CLIENT_CONTEXT", api_err_t::INVALID_CLIENT_CONTEXT)
    .value("INVALID_URL", api_err_t::INVALID_URL)
    .value("CONNECT_TIMEOUT", api_err_t::CONNECT_TIMEOUT)
    .value("CALLBACK_NOT_SET", api_err_t::CALLBACK_NOT_SET)
    .value("INVALID_GPU_INDEX", api_err_t::INVALID_GPU_INDEX)
    .value("INIT_VIDEO_PROCESSOR_FAILED", api_err_t::INIT_VIDEO_PROCESSOR_FAILED)
    .value("INIT_VIDEO_DECODER_FAILED", api_err_t::INIT_VIDEO_DECODER_FAILED);

    py::class_<videoproc_context>(m, "VideoProcContext")
        .def(py::init<>())
        .def_readwrite("gpu_index", &videoproc_context::gpu_index)
        .def_readwrite("target_format", &videoproc_context::target_format)
        .def_readwrite("target_fps", &videoproc_context::target_fps);

    py::enum_<pixel_format_t>(m, "PixelFormat")
        .value("NONE", pixel_format_t::none)
        .value("MONO", pixel_format_t::mono)
        .value("RGB24", pixel_format_t::rgb24)
        .value("BGR24", pixel_format_t::bgr24);
}