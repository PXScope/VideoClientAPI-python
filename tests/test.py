from video_client import (create_video_client, connect_video_client, start_video_client, stop_video_client,
                          disconnect_video_client, release_video_client, api_init)
from video_client import ApiError, PixelFormat, VideoProcContext, MV_FRAME_INFO
import time
import sys
import copy

def get_frame_info_string(frame_info):
    info = copy.deepcopy(frame_info)
    oss = ''
    oss += "\n[Frame Info]\n"
    oss += "- frame_num: " + info.nFrameNum + "\n"
    oss += "- timestamp: " + info.utc_timestamp_us + "\n"
    # oss += "- deviceInfo: " + info.deviceInfo
    return oss

def on_data_callback(ctx, data, size, frame_info):
    # print(get_frame_info_string(frame_info))
    print('on_data_callback')

def on_disconnect_callback(ctx, code, msg):
    print(sys.stdout)
    print(f"Disconnected: code={code}, msg={msg}")


api_init()

clnt = create_video_client()
if clnt:
    ret = connect_video_client(clnt, "tcp://127.0.0.1:31000/MV-GTL-DEV-001", 3, on_disconnect_callback)

    if ret == ApiError.SUCCESS.value:
        dec_ctx = VideoProcContext()
        dec_ctx.gpu_index = 0
        dec_ctx.target_format = PixelFormat.RGB24
        dec_ctx.target_fps = 30
        start_video_client(clnt, dec_ctx, on_data_callback)

        time.sleep(5)

if clnt:
    stop_video_client(clnt)

disconnect_video_client(clnt)
release_video_client(clnt)
