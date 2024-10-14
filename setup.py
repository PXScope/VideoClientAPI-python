from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension
import os


lib_dir = "videoclientapi_python/lib"
include_dir = "videoclientapi_python/include"

ext_modules = [
    Pybind11Extension(
        "videoclientapi_python.videoclientapi_python",
        ["src/plugins.cpp"],
        py_limited_api=True,
        include_dirs=['videoclientapi_python/include'],
        library_dirs=['videoclientapi_python/lib'],
        libraries=["VideoClientAPI", "NvDecoder"],
        runtime_library_dirs=['$ORIGIN', '$ORIGIN/lib'],
        extra_link_args=['-Wl,-rpath,$ORIGIN', '-Wl,-rpath,$ORIGIN/lib']
    ),
]

setup(
    name="videoclientapi_python",
    version="1.0.0",
    python_requires=">=3.8",
    packages=find_packages(),
    ext_modules=ext_modules,
    package_data={"videoclientapi_python": [
        "lib/*.so*",
        "include/*.h",
    ]},
    include_package_data=True,
    install_requires=[
                      "pydantic==1.8.2",
                      "numpy",
                      "opencv-python",
                      "easydict"
    ],
)