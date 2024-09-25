from video_client import *

def on_data(ctx, data, size, frame_info):
    print(f"Received data: size={size}, frame_info={frame_info}")

def on_disconnect(ctx, code, msg):
    print(f"Disconnected: code={code}, msg={msg}")

api_init()
clnt = create_video_client()
if clnt:
    ret = connect_video_client(clnt, "", 3, on_disconnect)
    if ret == ApiError.SUCCESS.value:
        dec_ctx = VideoProcContext()
        dec_ctx.gpu_index = 0
        dec_ctx.target_format = PixelFormat.RGB24
        dec_ctx.target_fps = 30

        start_video_client(clnt, dec_ctx, on_disconnect)
    else:
        print(ret)

if clnt:
    stop_video_client(clnt)

disconnect_video_client(clnt)
release_video_client(clnt)
