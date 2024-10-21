import time
import traceback
from collections import deque
from threading import Thread, Lock
import numpy as np
from easydict import EasyDict as edict
import logging
from .logger import get_logger
import videoclientapi_python as api
import pxproto.vision.detect as proto
import copy

logger = get_logger(__name__)


class CustomDeque(deque):

    def __init__(self, maxlen, ctx):
        super().__init__(maxlen=maxlen)
        self.c = ctx

    def append(self, item):
        if len(self) == self.maxlen:
            self.release_packet(self[0])
            self.popleft()
        super().append(item)

    def release_packet(self, item):
        try:
            api.release_frame(self.c, item.package)
        except Exception as e:
            logger.warning(
                "failed to checkin package in deque: {}".format(e))


class BufferQueue:

    def __init__(self, size, ctx):
        self.queue = CustomDeque(maxlen=size, ctx=ctx)
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
        self.is_exit = True

        self.fps = fps

        self.stabilize_ts = None
        self.stabilize_sec = stabilize_sec

        self.verbose = verbose
        if self.verbose:
            logger.setLevel(logging.DEBUG)
        else:
            logger.setLevel(logging.CRITICAL + 1)
            logger.addHandler(logging.NullHandler())

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
            api.disconnect_video_client(self.c)
            api.release_video_client(self.c)
            raise Exception(f"[{ret}] Failed to create client: {device}")
        else:
            logger.info("Connected to " + (f"{protocol}://{device}" if protocol == "shdm" else f"{device}://{host}:{port}/{device}"))

        # Setting videoproc_context
        self.p = api.VideoProcContext()
        if fps > 0:
            self.p.target_fps = fps
        self.p.gpu_index = gpu_index
        self.p.target_format = DST_COLORSPACE

        self.buffer_queue = BufferQueue(size=max_buffer_size, ctx=self.c)
        api.set_max_queue_size(self.c, max_buffer_size)

        self.handler_thread = Thread(target=self.handler,
                                     args=(
                                         self.buffer_queue,
                                     ),
                                     daemon=True)

    def on_disconnected(self, ctx, code, msg):
        logger.info(f"[{code}]client is disconnected: {msg}")
        self.buffer_queue.put(None)

    def on_frame_package_received(self, ctx, data, size, info):
        if self.stabilize_ts is None:
            self.stabilize_ts = time.perf_counter()
            logger.info(
                f"waiting {self.stabilize_sec} sec for grab client stabilization"
            )
            return True
        else:
            if time.perf_counter() - self.stabilize_ts < self.stabilize_sec:
                return True
        _info = copy.deepcopy(info)

        package_number = _info.nFrameNum
        frame = data
        timestamp = _info.utc_timestamp_us
        width = int(_info.deviceInfo.nWidth)
        height = int(_info.deviceInfo.nHeight)
        device_name = str(_info.deviceInfo.channelName)
        channels = 1 if self.colorspace == "mono" else 3

        if size != height * width * channels:
            logger.error(f"Frame size is weird: size: {size}, width: {width}, height: {height}, channels: {channels}")
            return True
        image = np.frombuffer(frame, dtype=np.uint8)
        image = image.reshape((height, width, channels))

        # get camera parameters
        camera_params = _info.deviceInfo.camera_parameter
        intrinsic_params = camera_params.intrinsic
        extrinsic_params = camera_params.extrinsic
        if _info.deviceInfo.camera_parameter.camera_model == api.PxMvCameraModel.PXMV_CAMERA_MODEL_OPENCV:
            intrinsic_params_proto = proto.CameraIntrinsic(
                id=camera_params.intrinsic_id,
                fx_fy_cx_cy=[float(x) for x in [
                    intrinsic_params.cv.fx, intrinsic_params.cv.fy,
                    intrinsic_params.cv.cx, intrinsic_params.cv.cy
                ]],
                distortion=[float(x) for x in [
                    intrinsic_params.cv.k1, intrinsic_params.cv.k2,
                    intrinsic_params.cv.p1, intrinsic_params.cv.p2,
                    intrinsic_params.cv.k3
                ]]
            )
        elif _info.deviceInfo.camera_parameter.camera_model == api.PxMvCameraModel.PXMV_CAMERA_MODEL_OPENCV_FISHEYE:
            intrinsic_params_proto = proto.CameraIntrinsic(
                id=camera_params.intrinsic_id,
                fx_fy_cx_cy=[float(x) for x in [
                    intrinsic_params.fisheye.fx, intrinsic_params.fisheye.fy,
                    intrinsic_params.fisheye.cx, intrinsic_params.fisheye.cy
                ]],
                distortion=[float(x) for x in [
                    intrinsic_params.fisheye.k1, intrinsic_params.fisheye.k2,
                    intrinsic_params.fisheye.k3, intrinsic_params.fisheye.k4,
                ]]
            )
        else:
            raise ValueError("Unsupported camera model")
        extrinsic_params_proto = proto.CameraExtrinsic(
            id=camera_params.extrinsic_id,
            rvec=[float(x) for x in extrinsic_params.rvec],
            tvec=[float(x) for x in extrinsic_params.tvec],
            rot_sys=[
                proto.Axis.Z_Plus, proto.Axis.X_Plus, proto.Axis.Y_Minus
            ],
            pos_sys=[
                proto.Axis.Y_Plus, proto.Axis.X_Plus, proto.Axis.Z_Plus
            ]
        )
        package_key = f"{device_name}_{timestamp}"
        package = data.__array_interface__['data'][0]

        self.buffer_queue.put(
            edict({
                "package_number": package_number,
                "timestamp": timestamp,
                "image": image,
                "intrinsic": intrinsic_params_proto,
                "extrinsic": extrinsic_params_proto,
                "package": package,
                "package_key": package_key
            }))
        return False

    def start_consumming(self):
        ret = api.start_video_client(self.c, self.p, self.on_frame_package_received)

        if ret == api.ApiError.SUCCESS:
            self.is_running = True
            self.handler_thread.start()

            while self.is_running:
                time.sleep(1e-6)

            self.handler_thread.join()

        # ******************
        # Clear VideoClient
        # ******************
        api.stop_video_client(self.c)
        time.sleep(1)

        api.clear_all_frames(self.c)
        time.sleep(1)

        api.disconnect_video_client(self.c)
        api.release_video_client(self.c)

        logger.info("client is terminated")

    def handler(self, buffer_queue):
        while True:
            data = buffer_queue.get()

            if data is None:
                break

            try:
                self.callback(data)
            except:
                logger.error(traceback.format_exc())
                break
            try:
                api.release_frame(self.c, data.package)
            except Exception as e:
                logger.warning(
                    "failed to checkin package in handler: {}".format(e)
                )

        self.is_running = False

        logger.info("handler is terminated")
