# Copyright (c) 2021-2022 Intel Corporation
#
# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License..
# ==============================================================================
'''
itex_setup.py file to build wheel for Intel® Extension for TensorFlow*
'''

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import fnmatch
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'intel_extension_for_tensorflow/python')) # pylint: disable=line-too-long
from version import __version__

from setuptools import setup
from setuptools.command.install import install as InstallCommandBase
from setuptools.dist import Distribution


# This version string is semver compatible, but incompatible with pip.
# For pip, we will remove all '-' characters from this string, and use the
# result for pip.
# Also update itex/itex.bzl
_VERSION = __version__

REQUIRED_PACKAGES = []

if sys.byteorder == 'little':
  # grpcio does not build correctly on big-endian machines due to lack of
  # BoringSSL support.
  # See https://github.com/tensorflow/tensorflow/issues/17882.
  REQUIRED_PACKAGES.append('grpcio >= 1.8.6')

project_name = 'intel_extension_for_tensorflow'
if '--project_name' in sys.argv:
  project_name_idx = sys.argv.index('--project_name')
  project_name = sys.argv[project_name_idx + 1]
  sys.argv.remove('--project_name')
  sys.argv.pop(project_name_idx)
REQUIRED_PACKAGES.append('wheel')
REQUIRED_PACKAGES.append('tensorflow>=2.10')
CONSOLE_SCRIPTS = []


_ext_path = 'intel_extension_for_tensorflow'

long_description = ''
this_directory = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(this_directory, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()
    
class BinaryDistribution(Distribution):

  def has_ext_modules(self):
    return True


class InstallCommand(InstallCommandBase):
  """Override the dir where the headers go."""

  def finalize_options(self):
    ret = InstallCommandBase.finalize_options(self)  # pylint: disable=assignment-from-no-return
    self.install_headers = os.path.join(self.install_platlib, 'intel_extension_for_tensorflow',
                                        'include')
    self.install_lib = self.install_platlib
    return ret

setup(
    name=project_name,
    version=_VERSION.replace('-', ''),
    description='Intel® Extension for Tensorflow*',
    long_description=long_description,
    long_description_content_type='text/markdown',
    # pylint: disable=line-too-long
    url='https://github.com/intel/intel-extension-for-tensorflow',
    download_url='https://github.com/intel/intel-extension-for-tensorflow/tags',
    project_urls={
            "Bug Tracker": "https://github.com/intel/intel-extension-for-tensorflow/issues",
    },
    # pylint: enable=line-too-long
    author='Intel Corporation',
    author_email='itex.maintainers@intel.com',
    # Contained modules and scripts.
    packages=[_ext_path],
    entry_points={
        'console_scripts': CONSOLE_SCRIPTS,
    },
    install_requires=REQUIRED_PACKAGES,
    tests_require=REQUIRED_PACKAGES,
    # Add in any packaged data.
    package_data={
        _ext_path: [
            '*.py',
            'python/*.py', 
            'python/ops/*.py',
            'python/test_func/*.py',
            'core/utils/protobuf/*.py',
            "third-party-programs/*",
        ],
    },
    exclude_package_data={
        'intel_extension_for_tensorflow': ['tools']
    },
    python_requires='>=3.7',
    zip_safe=False,
    distclass=BinaryDistribution,
    extras_require={
        'cpu': [f'intel_extension_for_tensorflow_lib=={_VERSION}.0'],
        'gpu': [f'intel_extension_for_tensorflow_lib=={_VERSION}.1'], 
    },
    # PyPI package information.
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Intended Audience :: Education',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Mathematics',
        'Topic :: Scientific/Engineering :: Artificial Intelligence',
        'Topic :: Software Development',
        'Topic :: Software Development :: Libraries',
        'Topic :: Software Development :: Libraries :: Python Modules',
    ],
    license='Apache 2.0',
    keywords='Intel® Extension for Tensorflow*',
        cmdclass={
        'install': InstallCommand,
    },
)
