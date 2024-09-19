import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

setup(
    name='video_client',
    version='0.0.1',
    author='Eugene',
    author_email='hmkwon@pxscope.com',
    description='VideoClientAPI Python version',
    long_description='',
    ext_modules=[CMakeExtension('video_client.video_client')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
    packages=['video_client'],
)




# # setup.py
# from setuptools import setup, Extension
# from pybind11.setup_helpers import Pybind11Extension, build_ext
#
# __version__ = "0.0.1"
#
# ext_modules = [
#     Pybind11Extension("video_client.plugins",
#         ["plugins.cpp"],
#         extra_compile_args=["-std=c++11"],
#     ),
# ]
#
# setup(
#     name="video_client",
#     version=__version__,
#     python_requires=">=3.8",
#     extras_require={"test": "pytest"},
#     ext_modules=ext_modules,
#     cmdclass={"build_ext": build_ext},
# )
#

# from pybind11.setup_helpers import Pybind11Extension, build_ext
# from setuptools import setup, Extension, find_packages
#
# __version__ = "0.0.1"
#
# setup(
#     # package info
#     name="video_client",
#     version=__version__,
#     author="Eugene",
#     author_email="hmkwon@pxscope.com",
#     url="",
#     long_description="",
#     packages=find_packages(include=["video_client", "video_client.*"]),
#     # package option
#     zip_safe=False,
#     install_requires=[],
#     python_requires=">=3.8",
#     extras_require={"test": "pytest"},
#     package_data={'VideoClientAPI': ['lib/VideoClientAPI']},
# )