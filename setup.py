from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

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
    ext_modules=ext_modules,
)