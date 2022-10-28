# Copyright (c) 2022 Intel Corporation
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
# limitations under the License.
# ==============================================================================


import numpy as np
import tensorflow as tf
from tensorflow.python.framework import dtypes
from tensorflow.python.ops import image_ops
from tensorflow.python.framework import constant_op
from utils import multi_run, add_profiling, flush_cache

try:
    from intel_extension_for_tensorflow.python.test_func import test
    FLOAT_COMPUTE_TYPE = [dtypes.float32, dtypes.float16, dtypes.bfloat16]
except ImportError:
    from tensorflow.python.platform import test
    FLOAT_COMPUTE_TYPE = [dtypes.float32, dtypes.float16]  # BF16 is not supported by CUDA

ITERATION = 5

class AdjustContrastV2Test(test.TestCase):
    def _test_impl(self, x_size, dtype):
        x = np.random.normal(size=x_size)
        x = constant_op.constant(x, dtype=dtype)
        contrast_factor = np.random.rand() * 2.0 + 0.1
        flush_cache()
        out_gpu = image_ops.adjust_contrast(x, contrast_factor)

    @add_profiling
    @multi_run(ITERATION)
    def testAdjustContrastV2(self):
        for dtype in FLOAT_COMPUTE_TYPE:
            self._test_impl([32,4,128,14], dtype)
            self._test_impl([16,4,128,28], dtype)
            self._test_impl([1,4,512,7], dtype)
            self._test_impl([8,100,14], dtype)
            self._test_impl([8,1000,7], dtype)

if __name__ == '__main__':
    test.main()   
