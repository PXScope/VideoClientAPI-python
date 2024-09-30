#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "video_client_api.h"
#include "MvFrameHeader.h"

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

            py::object py_frame_info = py::cast(static_cast<MV_FRAME_INFO*>(frame_info), py::return_value_policy::reference);

            it->second(VideoClientHandle(ctx), py_data, size, py_frame_info);
        } catch (py::error_already_set &e) {
            std::cerr << "Python data callback error: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No Python callback found for client: " << ctx << std::endl;
    }
}

// C++ 연결 해제 콜백 함수
void cpp_disconnect_callback(video_client ctx, int code, const char* msg) {
    py::gil_scoped_acquire acquire;
    auto it = g_py_disconnect_callbacks.find(ctx);
    if (it != g_py_disconnect_callbacks.end()) {
        try {
            it->second(VideoClientHandle(ctx), code, msg);
            std::cout << "Success: cpp_disconnect_callback" << std::endl;
        } catch (py::error_already_set &e) {
            std::cerr << "Python disconnect callback error: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No Python callback found for client: " << ctx << std::endl;
    }
}

PYBIND11_MODULE(video_client, m) {
    m.doc() = "VideoClientAPI Python version";

    // ********************
    // Function
    // ********************
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

    // ********************
    // Attribute
    // ********************
    py::enum_<api_err_t>(m, "ApiError")
        .value("SUCCESS", api_err_t::SUCCESS)
        .value("INVALID_CLIENT_CONTEXT", api_err_t::INVALID_CLIENT_CONTEXT)
        .value("INVALID_URL", api_err_t::INVALID_URL)
        .value("CONNECT_TIMEOUT", api_err_t::CONNECT_TIMEOUT)
        .value("CALLBACK_NOT_SET", api_err_t::CALLBACK_NOT_SET)
        .value("INVALID_GPU_INDEX", api_err_t::INVALID_GPU_INDEX)
        .value("INIT_VIDEO_PROCESSOR_FAILED", api_err_t::INIT_VIDEO_PROCESSOR_FAILED)
        .value("INIT_VIDEO_DECODER_FAILED", api_err_t::INIT_VIDEO_DECODER_FAILED);

    py::enum_<pixel_format_t>(m, "PixelFormat")
        .value("NONE", pixel_format_t::none)
        .value("MONO", pixel_format_t::mono)
        .value("RGB24", pixel_format_t::rgb24)
        .value("BGR24", pixel_format_t::bgr24);

    // ********************
    // Class
    // ********************
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
        })
    ;

    py::class_<videoproc_context>(m, "VideoProcContext")
        .def(py::init<>())
        .def_readwrite("gpu_index", &videoproc_context::gpu_index)
        .def_readwrite("target_format", &videoproc_context::target_format)
        .def_readwrite("target_fps", &videoproc_context::target_fps
        )
    ;

    py::class_<PxMvDeviceInfo>(m, "PxMvDeviceInfo")
        .def(py::init<>())
        .def_readwrite("nWidth", &PxMvDeviceInfo::nWidth)
        .def_readwrite("nHeight", &PxMvDeviceInfo::nHeight)
        .def_readwrite("name_hash", &PxMvDeviceInfo::name_hash)
        .def_readwrite("enPixelType", &PxMvDeviceInfo::enPixelType)
        .def_readwrite("fps", &PxMvDeviceInfo::fps)
        .def_readwrite("camera_parameter", &PxMvDeviceInfo::camera_parameter)
        .def_property("channelName",
            [](const PxMvDeviceInfo& self) -> std::string {
                return std::string(self.channelName);
            },
            [](PxMvDeviceInfo& self, const std::string& value) {
                std::strncpy(self.channelName, value.c_str(), sizeof(self.channelName)-1);
                self.channelName[sizeof(self.channelName) - 1] = '\0';
            }
        )
        .def_property("vendor",
            [](const PxMvDeviceInfo& self) -> std::string {
                return std::string(self.vendor);
            },
            [](PxMvDeviceInfo& self, const std::string& value) {
                std::strncpy(self.vendor, value.c_str(), sizeof(self.vendor)-1);
                self.vendor[sizeof(self.vendor) - 1] = '\0';
            }
        )
    ;

    py::class_<PxMvCameraParameter>(m, "PxMvCameraParameter")
        .def(py::init<>())
        .def_readwrite("_reserved0", &PxMvCameraParameter::_reserved0)
        .def_readwrite("intrinsic_id", &PxMvCameraParameter::intrinsic_id)
        .def_readwrite("extrinsic_id", &PxMvCameraParameter::extrinsic_id)
        .def_property("camera_model",
            [](const PxMvCameraParameter& self) -> const PxMvCameraModel& { return self.camera_model; },
            [](PxMvCameraParameter& self, const PxMvCameraModel value) { self.camera_model = value; })
        .def_property("intrinsic",
            [](const PxMvCameraParameter& self) -> const PxMvCameraIntrinsicUnion& { return self.intrinsic; },
            [](PxMvCameraParameter& self, const PxMvCameraIntrinsicUnion& value) { self.intrinsic = value; })
        .def_property("extrinsic",
            [](const PxMvCameraParameter& self) -> const PxMvCameraExtrinsic& { return self.extrinsic; },
            [](PxMvCameraParameter& self, const PxMvCameraExtrinsic& value) { self.extrinsic = value; })
        .def_property("_reserved1",
            [](const PxMvCameraParameter& self) { return py::array_t<uint64_t>(3, self._reserved1); },
            [](PxMvCameraParameter& self, py::array_t<uint64_t> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 3)
                    throw std::runtime_error("Input array must have 3 elements");
                for (py::ssize_t i=0; i<3; ++i)
                    self._reserved1[i] = r(i);
            })
    ;

    py::class_<MV_FRAME_INFO>(m, "MV_FRAME_INFO")
        .def(py::init<>())
        .def_readwrite("header_size", &MV_FRAME_INFO::header_size)
        .def_readwrite("version", &MV_FRAME_INFO::version)
        .def_readwrite("nFrameNum", &MV_FRAME_INFO::nFrameNum)
        .def_readwrite("nHWFrameNum", &MV_FRAME_INFO::nHWFrameNum)
        .def_readwrite("utc_timestamp_us", &MV_FRAME_INFO::utc_timestamp_us)
        .def_readwrite("hw_timestamp_us", &MV_FRAME_INFO::hw_timestamp_us)
        .def_readwrite("nOffsetX", &MV_FRAME_INFO::nOffsetX)
        .def_readwrite("nOffsetY", &MV_FRAME_INFO::nOffsetY)
        .def_readwrite("nLostPacket", &MV_FRAME_INFO::nLostPacket)
        .def_readwrite("nFrameLen", &MV_FRAME_INFO::nFrameLen)
        .def_property("start_code",
            [](const MV_FRAME_INFO& self) { return py::array_t<uint8_t>(4, self.start_code); },
            [](MV_FRAME_INFO& self, py::array_t<uint8_t> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 4)
                    throw std::runtime_error("Input array must have 3 elements");
                for (py::ssize_t i=0; i<4; ++i)
                    self.start_code[i] = r(i);
        })
        .def_property("deviceInfo",
            [](const MV_FRAME_INFO& self) { return self.deviceInfo; },
            [](MV_FRAME_INFO& self, PxMvDeviceInfo& value) { return self.deviceInfo; }
        )
    ;

}
