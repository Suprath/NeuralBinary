import os
import sys
from setuptools import setup, Extension

extra_compile_args = ["-std=c++20", "-O3"]
if sys.platform == "darwin":
    extra_compile_args.extend(["-arch", "arm64", "-march=armv8-a+crc"])
elif sys.platform == "win32":
    extra_compile_args = ["/std:c++20", "/O2"]

neuralbinary_zlib_module = Extension(
    "neuralbinary_zlib",
    sources=[
        "modernized/modernized_official_adler32.cpp",
        "modernized/modernized_zlib_crc32.cpp",
        "modernized/modernized_zlib_c_api.cpp",
    ],
    include_dirs=["modernized"],
    language="c++",
    extra_compile_args=extra_compile_args,
)

setup(
    name="neuralbinary-zlib",
    version="1.0.0",
    description="High-Performance Modernized C++20 + SDLG zlib Compression Engine",
    ext_modules=[neuralbinary_zlib_module],
    zip_safe=False,
)
