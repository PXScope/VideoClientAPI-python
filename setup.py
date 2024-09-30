from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension
import os

project_root = os.path.abspath(os.path.dirname(__file__))
lib_dir = os.path.join(project_root, "VideoClientAPI", "linux_out", "lib")
include_dir = os.path.join(project_root, "VideoClientAPI", "linux_out", "include")

ext_modules = [
    Pybind11Extension(
        "video_client.video_client",
        ["src/plugins.cpp"],
        include_dirs=[include_dir],
        library_dirs=[lib_dir],
        libraries=["VideoClientAPI", "NvDecoder"],
        runtime_library_dirs=[lib_dir],
        extra_link_args=[f"-Wl,-rpath,{lib_dir}"],
    ),
]

setup(
    name='video_client',
    version='0.0.1',
    packages=['video_client'],
    ext_modules=ext_modules,
    package_data={'video_client': ['../VideoClientAPI/linux_out/lib/*.so*']},
    include_package_data=True,
)