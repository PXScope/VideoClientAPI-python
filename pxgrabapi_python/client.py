import time
import traceback
from collections import deque
from threading import Thread, Lock
import numpy as np
from easydict import EasyDict as edict

from .logger import get_logger
import pxgrabapi_python as api
import pxproto.vision.detect as proto

logger = get_logger(__name__)


class CustomDeque(deque):

    def __init__(self, maxlen, packages, lock):
        super().__init__(maxlen=maxlen)
        self.packages = packages
        self.lock = lock

    def append(self, item):
        if len(self) == self.maxlen:
            self.release_packet(self[0])
            self.popleft()
        super().append(item)

    def release_packet(self, item):
        with self.lock:
            try:
                del self.packages[item.package_key]
            except Exception as e:
                logger.warning(
                    "failed to checkin package in deque: {}".format(e))


class BufferQueue:

    def __init__(self, size, packages, package_lock):
        self.queue = CustomDeque(maxlen=size,
                                 packages=packages,
                                 lock=package_lock)
        self.lock = Lock()

    def put(self, x):
        with self.lock:
            self.queue.append(x)

    def get(self):
        while True:
            with self.lock:
                if len(self.queue) > 0:
                    return self.queue.popleft()
            time.sleep(1e-6)

    def __len__(self):
        with self.lock:
            return len(self.queue)


class GrabClient:

    def __init__(self,
                 callback,
                 host,
                 port,
                 devices,
                 fps=0,
                 gpu_index=0,
                 colorspace="rgb",
                 protocol="tcp",
                 max_buffer_size=120,
                 stabilize_sec=1,
                 verbose=False):

        # check devices param is valid value
        assert 0 < len(devices) <= 1
        device = devices[0]

        # check colorspace
        assert colorspace in ["rgb", "bgr", "mono"]

        self.callback = callback
        self.colorspace = colorspace

        self.is_running = False
        # self.is_exit = False

        self.packages = {}
        self.package_lock = Lock()

        self.fps = fps
        self.buffer_queue = BufferQueue(max_buffer_size, self.packages, self.package_lock)

        self.stabilize_ts = None
        self.stabilize_sec = stabilize_sec

        self.verbose = verbose

        if colorspace == "rgb":
            DST_COLORSPACE = api.PixelFormat.RGB24
        elif colorspace == "bgr":
            DST_COLORSPACE = api.PixelFormat.BGR24
        elif colorspace == "mono":
            DST_COLORSPACE = api.PixelFormat.MONO
        else:
            DST_COLORSPACE = api.PixelFormat.NONE

        self.c = api.create_video_client()
        if protocol == "tcp":
            ret = api.connect_video_client(self.c, f"{protocol}://{host}:{port}/{device}", 3, self.on_disconnected)
        elif protocol == "shdm":
            ret = api.connect_video_client(self.c, f"{protocol}://{device}", 3, self.on_disconnected)
        else:
            raise ValueError(f"Input protocol is {protocol}. Protocol must be either tcp or shdm")

        if ret != api.ApiError.SUCCESS:
            raise Exception(f"[{ret}] Failed to create client: {device}")

        logger.info("Connected to " + f"{protocol}://{device}" if protocol == "shdm" else f"{device}://{host}:{port}/{device}")

        self.p = api.VideoProcContext()

        if fps > 0:
            self.p.target_fps = fps
        self.p.gpu_index = gpu_index
        self.p.target_format = DST_COLORSPACE

        self.handler_thread = Thread(target=self.handler,
                                     args=(
                                         self.buffer_queue,
                                         self.package_lock,
                                     ),
                                     daemon=True)

    def on_disconnected(self, ctx, code, msg):
        logger.info(f"[{code}]client is disconnected: {msg}")

    def on_frame_package_received(self, ctx, data, size, info):
        if self.stabilize_ts is None:
            self.stabilize_ts = time.perf_counter()
            logger.info(
                f"waiting {self.stabilize_sec} sec for grab client stabilization"
            )
            return
        else:
            if time.perf_counter() - self.stabilize_ts < self.stabilize_sec:
                return

        package_number = info.nFrameNum
        timestamp = info.utc_timestamp_us
        frame = data

        width = int(info.deviceInfo.nWidth)
        height = int(info.deviceInfo.nHeight)
        device_name = str(info.deviceInfo.channelName)
        channels = 1 if self.colorspace == "mono" else 3

        if size != height * width * channels:
            height = int(size / (width * channels))
            logger.warn(f"Frame size is weird: size: {size}, width: {width}, height: {height}, channels: {channels}")
        image = np.asarray(frame)
        image = image.reshape((height, width, channels))

        # get camera parameters
        camera_params = info.deviceInfo.camera_parameter
        intrinsic_params = camera_params.intrinsic
        extrinsic_params = camera_params.extrinsic
        if info.deviceInfo.camera_parameter.camera_model == api.PxMvCameraModel.PXMV_CAMERA_MODEL_OPENCV:
            intrinsic_params_proto = proto.CameraIntrinsic(
                id=camera_params.intrinsic_id,
                fx_fy_cx_cy=[
                    intrinsic_params.cv.fx, intrinsic_params.cv.fy,
                    intrinsic_params.cv.cx, intrinsic_params.cv.cy
                ],
                distortion=[
                    intrinsic_params.cv.k1, intrinsic_params.cv.k2,
                    intrinsic_params.cv.p1, intrinsic_params.cv.p2,
                    intrinsic_params.cv.k3
                ]
            )
        elif info.deviceInfo.camera_parameter.camera_model == api.PxMvCameraModel.PXMV_CAMERA_MODEL_OPENCV_FISHEYE:
            intrinsic_params_proto = proto.CameraIntrinsic(
                id=camera_params.intrinsic_id,
                fx_fy_cx_cy=[
                    intrinsic_params.fisheye.fx, intrinsic_params.fisheye.fy,
                    intrinsic_params.fisheye.cx, intrinsic_params.fisheye.cy
                ],
                distortion=[
                    intrinsic_params.fisheye.k1, intrinsic_params.fisheye.k2,
                    intrinsic_params.fisheye.k3, intrinsic_params.fisheye.k4,
                ]
            )
        else:
            raise ValueError("Unsupported camera model")
        extrinsic_params_proto = proto.CameraExtrinsic(
            id=camera_params.extrinsic_id,
            rvec=[x for x in extrinsic_params.rvec],
            tvec=[x for x in extrinsic_params.tvec],
            rot_sys=[
                proto.Axis.Z_Plus, proto.Axis.X_Plus, proto.Axis.Y_Minus
            ],
            pos_sys=[
                proto.Axis.Y_Plus, proto.Axis.X_Plus, proto.Axis.Z_Plus
            ]
        )
        package_key = f"{device_name}_{timestamp}"

        self.buffer_queue.put(
            edict({
                "package_number": package_number,
                "timestamp": timestamp,
                "image": image,
                "intrinsic": intrinsic_params_proto,
                "extrinsic": extrinsic_params_proto,
                "package_key": package_key
            }))

    def start_consumming(self):
        self.is_running = True
        api.start_video_client(self.c, self.p, self.on_frame_package_received)

        self.handler_thread.start()

        while self.is_running:
            time.sleep(1e-6)

        self.handler_thread.join()

        api.stop_video_client(self.c)
        time.sleep(1)

        api.disconnect_video_client(self.c)
        api.release_video_client(self.c)

        logger.info("client is terminated")

    def handler(self, buffer_queue, package_lock):
        while True:
            data = buffer_queue.get()

            if data is None:
                break

            try:
                self.callback(data)
            except:
                logger.error(traceback.format_exc())
                break

        self.is_running = False

        logger.info("handler is terminated")
