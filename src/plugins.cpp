#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdexcept>
#include <pybind11/numpy.h>

#include "video_client_api.h"
#include "MvFrameHeader.h"
#include <chrono>

namespace py = pybind11;

// 불투명 핸들을 위한 커스텀 타입
struct VideoClientHandle {
    video_client ptr;

    explicit VideoClientHandle(video_client p) : ptr(p) {}

    ~VideoClientHandle() {
        if (ptr) {
            ptr = nullptr;
        }
    }
};

// 전역 Python 콜백 저장소
std::unordered_map<video_client, py::function> g_py_data_callbacks;
std::unordered_map<video_client, py::function> g_py_disconnect_callbacks;


bool cpp_data_callback(video_client ctx, uint8_t* data, size_t size, void* frame_info) {
    bool is_release = true;
    {
        py::gil_scoped_acquire acquire;
        auto it = g_py_data_callbacks.find(ctx);
        if (it != g_py_data_callbacks.end()) {
            try {
    //            auto start = std::chrono::high_resolution_clock::now();
    //            auto checkpoint = start;

                // Step 1: Create py::array_t
    //            std::cout << "C++ reference addresses: " << (void*)data << std::endl;
                py::array_t<uint8_t> py_data({size}, {sizeof(uint8_t)}, data, py::none());
    //            auto after_array_creation = std::chrono::high_resolution_clock::now();
    //            std::cout << "Array creation time: "
    //                      << std::chrono::duration_cast<std::chrono::microseconds>(after_array_creation - checkpoint).count()
    //                      << " us" << std::endl;
    //            checkpoint = after_array_creation;

                // Step 2: Create py::object for frame_info
                py::object py_frame_info = py::cast(static_cast<MV_FRAME_INFO*>(frame_info), py::return_value_policy::reference);
    //            auto after_frame_info = std::chrono::high_resolution_clock::now();
    //            std::cout << "Frame info creation time: "
    //                      << std::chrono::duration_cast<std::chrono::microseconds>(after_frame_info - checkpoint).count()
    //                      << " us" << std::endl;
    //            checkpoint = after_frame_info;

                // Step 3: Call Python callback
                py::object py_is_release = it->second(VideoClientHandle(ctx), py_data, size, py_frame_info);
                is_release = py_is_release.cast<bool>();
    //            std::cout << "C++ data address: " << (void*)data << std::endl;

    //            auto after_callback = std::chrono::high_resolution_clock::now();
    //            std::cout << "Python callback time: "
    //                      << std::chrono::duration_cast<std::chrono::microseconds>(after_callback - checkpoint).count()
    //                      << " us" << std::endl;

    //            auto end = std::chrono::high_resolution_clock::now();
    //            std::cout << "Total time: "
    //                      << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
    //                      << " us" << std::endl;
            } catch (py::error_already_set &e) {
                std::cerr << "Python data callback error: " << e.what() << std::endl;
            }
        } else {
            std::cout << "No Python callback found for client: " << ctx << std::endl;
        }
    }
    return is_release;
}

void cpp_disconnect_callback(video_client ctx, int code, const char* msg) {
    py::gil_scoped_acquire acquire;
    auto it = g_py_disconnect_callbacks.find(ctx);
    if (it != g_py_disconnect_callbacks.end()) {
        try {
            it->second(VideoClientHandle(ctx), code, msg);
        } catch (py::error_already_set &e) {
            std::cerr << "Python disconnect callback error: " << e.what() << std::endl;
        }
    } else {
        std::cout << "No Python callback found for client: " << ctx << std::endl;
    }
}

PxMvCameraParameter copy_PxMvCameraParameter(const PxMvCameraParameter& self) {
    PxMvCameraParameter cameraParam;

    cameraParam.camera_model = self.camera_model;
    cameraParam._reserved0 = self._reserved0;
    cameraParam.intrinsic_id = self.intrinsic_id;
    cameraParam.extrinsic_id = self.extrinsic_id;

    std::memcpy(&cameraParam.intrinsic, &self.intrinsic, sizeof(PxMvCameraIntrinsicUnion));
    std::memcpy(&cameraParam.extrinsic, &self.extrinsic, sizeof(PxMvCameraExtrinsic));
    std::memcpy(cameraParam._reserved1, self._reserved1, sizeof(cameraParam._reserved1));

    return cameraParam;
}

PxMvDeviceInfo copy_PxMvDeviceInfo(const PxMvDeviceInfo& self) {
    PxMvDeviceInfo deviceInfo;

    deviceInfo.nWidth = self.nWidth;
    deviceInfo.nHeight = self.nHeight;
    deviceInfo.name_hash = self.name_hash;
    deviceInfo.enPixelType = self.enPixelType;
    deviceInfo.fps = self.fps;

    std::memcpy(deviceInfo.channelName, self.channelName, sizeof(deviceInfo.channelName));
    std::memcpy(deviceInfo.vendor, self.vendor, sizeof(deviceInfo.vendor));
    deviceInfo.camera_parameter = copy_PxMvCameraParameter(self.camera_parameter);

    return deviceInfo;
}

MV_FRAME_INFO copy_MV_FRAME_INFO(const MV_FRAME_INFO& self) {
    MV_FRAME_INFO frameInfo;

    std::memcpy(frameInfo.start_code, self.start_code, sizeof(frameInfo.start_code));

    frameInfo.header_size = self.header_size;
    frameInfo.version = self.version;
    frameInfo.nFrameNum = self.nFrameNum;
    frameInfo.nHWFrameNum = self.nHWFrameNum;
    frameInfo.utc_timestamp_us = self.utc_timestamp_us;
    frameInfo.hw_timestamp_us = self.hw_timestamp_us;
    frameInfo.nOffsetX = self.nOffsetX;
    frameInfo.nOffsetY = self.nOffsetY;
    frameInfo.nLostPacket = self.nLostPacket;
    frameInfo.nFrameLen = self.nFrameLen;

    frameInfo.deviceInfo = copy_PxMvDeviceInfo(self.deviceInfo);

    return frameInfo;
}

PYBIND11_MODULE(videoclientapi_python, m) {
    m.doc() = "VideoClientAPI Python version.";

    // ********************
    // Function
    // ********************
    m.def("create_video_client", []() {
        video_client client = create_video_client();
        return VideoClientHandle(client);
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
        auto result = disconnect_video_client(handle.ptr);
        return result;
    });
    m.def("stop_video_client", [](VideoClientHandle& handle) {
        return stop_video_client(handle.ptr);
    });
    m.def("start_video_client", [](VideoClientHandle& handle, videoproc_context vp_ctx, py::function callback) {
        g_py_data_callbacks[handle.ptr] = callback;
        return start_video_client(handle.ptr, vp_ctx, cpp_data_callback);
    });
    m.def("set_max_queue_size", [](VideoClientHandle& handle, size_t size) {
        return set_max_queue_size(handle.ptr, size);
    });
    m.def("api_init", &api_init);
    m.def("release_frame", [](VideoClientHandle& handle, std::uintptr_t ptr) {
        uint8_t* data_ptr = reinterpret_cast<uint8_t*>(ptr);
//        std::cout << "[Return] C++ reference addresses: " << (void*)data_ptr << std::endl;
        if (handle.ptr && ptr) {
            try {
                release_frame(handle.ptr, data_ptr);
            } catch (const std::exception& e) {
                PyErr_SetString(PyExc_RuntimeError, (std::string("Error in release_frame: ") + e.what()).c_str());
                throw py::error_already_set();
            }
        } else {
            PyErr_SetString(PyExc_ValueError, "Invalid handle or pointer");
            throw py::error_already_set();
        }
    });
    m.def("clear_all_frames", [](VideoClientHandle& handle) {
        if (handle.ptr) {
            try {
                clear_all_frames(handle.ptr);
            } catch (const std::exception& e) {
                PyErr_SetString(PyExc_RuntimeError, (std::string("Error in clear_all_frames: ") + e.what()).c_str());
                throw py::error_already_set();
            }
        } else {
            PyErr_SetString(PyExc_ValueError, "Invalid handle");
            throw py::error_already_set();
        }
    });
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

    py::enum_<PxMvCameraModel>(m, "PxMvCameraModel")
        .value("PXMV_CAMERA_MODEL_NONE", PxMvCameraModel::PXMV_CAMERA_MODEL_NONE)
        .value("PXMV_CAMERA_MODEL_OPENCV", PxMvCameraModel::PXMV_CAMERA_MODEL_OPENCV)
        .value("PXMV_CAMERA_MODEL_OPENCV_FISHEYE", PxMvCameraModel::PXMV_CAMERA_MODEL_OPENCV_FISHEYE);

    // ********************
    // Class
    // ********************
    py::class_<VideoClientHandle>(m, "VideoClient")
        .def(py::init([]() {
            video_client client = create_video_client();
            return std::make_unique<VideoClientHandle>(client);
        }))
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
        .def("__copy__", &copy_PxMvDeviceInfo)
        .def("__deepcopy__", [](const PxMvDeviceInfo &self, py::dict) {
            return copy_PxMvDeviceInfo(self);
        })
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
        .def("__copy__", &copy_MV_FRAME_INFO)
        .def("__deepcopy__", [](const MV_FRAME_INFO &self, py::dict) {
            return copy_MV_FRAME_INFO(self);})
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
            [](MV_FRAME_INFO& self, PxMvDeviceInfo& value) { self.deviceInfo = value; }
        )
    ;

    py::class_<PxMvCameraIntrinsicUnion>(m, "PxMvCameraIntrinsicUnion")
        .def(py::init<>())
        .def_property("_max_size",
            [](const PxMvCameraIntrinsicUnion& self) { return py::array_t<uint64_t>(16, self._max_size); },
            [](PxMvCameraIntrinsicUnion& self, py::array_t<uint64_t> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 16)
                    throw std::runtime_error("Input array must have 16 elements");
                for (py::ssize_t i=0; i<16; ++i)
                    self._max_size[i] = r(i);
        })
        .def_property("cv",
            [](const PxMvCameraIntrinsicUnion& self) { return self.cv; },
            [](PxMvCameraIntrinsicUnion& self, PxMvCameraModelOCV& value) { self.cv = value; })
        .def_property("fisheye",
            [](const PxMvCameraIntrinsicUnion& self) { return self.fisheye; },
            [](PxMvCameraIntrinsicUnion& self, PxMvCameraModelOCVFishEye& value) { self.fisheye = value; })
    ;

    py::class_<PxMvCameraModelOCV>(m, "PxMvCameraModelOCV")
        .def(py::init<>())
        .def_readwrite("fx", &PxMvCameraModelOCV::fx)
        .def_readwrite("fy", &PxMvCameraModelOCV::fy)
        .def_readwrite("cx", &PxMvCameraModelOCV::cx)
        .def_readwrite("cy", &PxMvCameraModelOCV::cy)
        .def_readwrite("k1", &PxMvCameraModelOCV::k1)
        .def_readwrite("k2", &PxMvCameraModelOCV::k2)
        .def_readwrite("k3", &PxMvCameraModelOCV::k3)
        .def_readwrite("p1", &PxMvCameraModelOCV::p1)
        .def_readwrite("p2", &PxMvCameraModelOCV::p2)
    ;

    py::class_<PxMvCameraModelOCVFishEye>(m, "PxMvCameraModelOCVFishEye")
        .def(py::init<>())
        .def_readwrite("fx", &PxMvCameraModelOCVFishEye::fx)
        .def_readwrite("fy", &PxMvCameraModelOCVFishEye::fy)
        .def_readwrite("cx", &PxMvCameraModelOCVFishEye::cx)
        .def_readwrite("cy", &PxMvCameraModelOCVFishEye::cy)
        .def_readwrite("k1", &PxMvCameraModelOCVFishEye::k1)
        .def_readwrite("k2", &PxMvCameraModelOCVFishEye::k2)
        .def_readwrite("k3", &PxMvCameraModelOCVFishEye::k3)
        .def_readwrite("k4", &PxMvCameraModelOCVFishEye::k4)
    ;

    py::class_<PxMvCameraExtrinsic>(m, "PxMvCameraExtrinsic")
        .def(py::init<>())
        .def_property("rvec",
            [](const PxMvCameraExtrinsic& self) { return py::array_t<double>(3, self.rvec); },
            [](PxMvCameraExtrinsic& self, py::array_t<double> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 3)
                    throw std::runtime_error("Input array must have 3 elements");
                for (py::ssize_t i=0; i<3; ++i)
                    self.rvec[i] = r(i);
        })
        .def_property("tvec",
            [](const PxMvCameraExtrinsic& self) { return py::array_t<double>(3, self.tvec); },
            [](PxMvCameraExtrinsic& self, py::array_t<double> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 3)
                    throw std::runtime_error("Input array must have 3 elements");
                for (py::ssize_t i=0; i<3; ++i)
                    self.tvec[i] = r(i);
        })
        .def_property("_reserved",
            [](const PxMvCameraExtrinsic& self) { return py::array_t<uint64_t>(4, self._reserved); },
            [](PxMvCameraExtrinsic& self, py::array_t<uint64_t> arr) {
                auto r = arr.unchecked<1>();
                if (r.shape(0) != 4)
                    throw std::runtime_error("Input array must have 3 elements");
                for (py::ssize_t i=0; i<4; ++i)
                    self._reserved[i] = r(i);
        })
    ;

}
