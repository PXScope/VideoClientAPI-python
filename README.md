# VideoClientAPI-Python

VideoClientAPI Python version repo used for connecting Grab-server and Video-server.

## Dependencies

* [VideoClientAPI](https://github.com/PXScope/VideoClientAPI.git)
* [Pybind11](https://github.com/pybind/pybind11.git)
* [pxcast-protocols-generated](https://github.com/PXScope/pxcast-protocols-generated.git)
* cmake

## Installation
```shell
git clone ?
git submodule init
git submodule update
```
### C++ 라이브러리 빌드
```shell
cd VideoClientAPI
mkdir build && cd build
make install
cp -r ../linux_out/* ../../videoclientapi_python
```
### 라이브러리 설치
```shell
pip install -e .
```
## Usage
### Total pipeline
```python
from videoclientapi_python.client import GrabClient


def main():
    # 결과 확인을 위한 시각화클래스 호출
    from videoclientapi_python.utils import Visualizer
    global visualizer
    visualizer = Visualizer()
    
    # 콜백함수 정의
    def cb(param):
        global visualizer
        print(visualizer.get_info_string(param))
        visualizer.save_frame_as_image(param, save_path="/home/kwon/Downloads/images")
    
    # 클라이언트 설정
    client = GrabClient(callback=cb,
                        host="127.0.0.1",
                        port=31000,
                        devices=["MV-GTL-DEV-001"],
                        fps=30,
                        colorspace="rgb")
    # 클라이언트 수신 실행
    client.start_consumming()
```
### Server connection (video-server or grab-server)
```python
# video-server connection
from videoclientapi_python.client import GrabClient

client = GrabClient(callback=None,
                    host="127.0.0.1",
                    port=31000,
                    devices=["MV-GTL-DEV-001"],
                    fps=30,
                    colorspace="rgb")
client.start_consumming()
```

```python
# grab-server connection
from videoclientapi_python.client import GrabClient

client = GrabClient(callback=None,
                    host="",
                    port="",
                    devices=["DA3180173"],
                    protocol="shdm",
                    gpu_index=0,
                    fps=30,
                    colorspace="rgb")
client.start_consumming()
```

## Etc
### Making .whl file
```shell
cd ${ROOT_DIR}
pip install wheel
python setup.py bdist_wheel
```