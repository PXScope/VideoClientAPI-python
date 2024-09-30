# VideoClientAPI-Python

VideoClientAPI Python version repo

## Dependencies

* [VideoClientAPI](git@github.com:PXScope/VideoClientAPI.git)
* [Pybind11](https://github.com/pybind/pybind11.git)
* cmake

## Installation
```shell
git clone ?
git submodule init
git submodule update
```
```shell
pip install -e .
```

## Usage
```python
from video_client import *

# api 초기화(가장 먼저 실행)
api_init()

# 새로운 비디오클라이언트 인스턴스 생성
clnt = create_video_client()
if clnt:
    # 비디오 서버 연결
    ret = connect_video_client(clnt, "", 3)
    if ret == ApiError.SUCCESS.value:
        dec_ctx = VideoProcContext()
        dec_ctx.gpu_index = 0
        dec_ctx.target_format = PixelFormat.RGB24
        dec_ctx.target_fps = 30

        start_video_client(clnt, dec_ctx, on_data_callback)
    else:
        print(ret)

if clnt:
    stop_video_client(clnt)

disconnect_video_client(clnt)
release_video_client(clnt)
```
## Issue
* start_video_client 구현중
* 콜백함수 파이썬 버전 구현중