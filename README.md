# VideoClientAPI-Python

VideoClientAPI Python version repo used for connecting Grab-server and Video-server.

## Dependencies

* [VideoClientAPI](https://github.com/PXScope/VideoClientAPI.git)
* [Pybind11](https://github.com/pybind/pybind11.git)
* [pxcast-protocols-generated](https://github.com/PXScope/pxcast-protocols-generated.git)
* [cmake](https://cmake.org/)

## Installation

### C++ Library Build
**VideoClientAPI**
```shell
git clone ?
cd VideoClientAPI
mkdir build && cd build
make install
cp -r ../linux_out/* ../../videoclientapi_python
```
**pxcast-protocols-generated**

Please check out 솓 README.md file in [pxcast-protocols-generated](https://github.com/PXScope/pxcast-protocols-generated.git)
## Usage

### Server connection (video-server or grab-server)
```python
# video-server connection
from videoclientapi_python.client import GrabClient

def callback(param):
    pass

client = GrabClient(
                 callback,
                 host="127.0.0.1",
                 port=31000,
                 devices=["MV-GTL-DEV-001"],
                 fps=30,
                 gpu_index=0,
                 colorspace="rgb",
                 protocol="shmd",
                 max_buffer_size=120,
                 stabilize_sec=1,
                 verbose=False)
client.start_consumming()
```

```python
# grab-server connection
from videoclientapi_python.client import GrabClient

def callback(param):
    pass

client = GrabClient(
                 callback,
                 host="",
                 port=0,
                 devices=["DA3180173"],
                 fps=30,
                 gpu_index=0,
                 colorspace="rgb",
                 protocol="tcp",
                 max_buffer_size=120,
                 stabilize_sec=1,
                 verbose=False)
client.start_consumming()
```
You can use functions from [utils.py](videoclientapi_python/utils.py) to define your callback function as shown below
```python
from videoclientapi_python.utils import Visualizer

global visualizer
visualizer = Visualizer()
    

def cb(param):
    global visualizer
    print(visualizer.get_info_string(param))
    visualizer.save_frame_as_image(param, save_path="/home/kwon/Downloads/images")    
```
## Library Packaging
### Clear package cache
```shell
rm -rf build/ dist/ *.egg-info/
find . -type d -name "__pycache__" -exec rm -rf {} +
find . -name "*.pyc" -delete
```
### Making .whl file
```shell
# install build package
pip install build
```
```shell
cd ${ROOT_DIR}
python -m build
```
