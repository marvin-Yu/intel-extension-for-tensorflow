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
from tensorflow.python.ops import gen_math_ops
from tensorflow.python.framework import constant_op
from utils import multi_run, add_profiling, flush_cache, tailed_no_tailed_size

try:
    from intel_extension_for_tensorflow.python.test_func import test
except ImportError:
    from tensorflow.python.platform import test

FLOAT_COMPUTE_TYPE = [dtypes.float32, dtypes.float16, dtypes.bfloat16]
ITERATION = 5

class TanhGradTest(test.TestCase):
    def _test_impl(self, size, dtype):
        x = np.random.normal(size=size)
        x = constant_op.constant(x, dtype=dtype)
        y = np.random.normal(size=size)
        y = constant_op.constant(y, dtype=dtype)
        flush_cache()
        out_gpu = gen_math_ops.tanh_grad(x, y)

    @add_profiling
    @multi_run(ITERATION)
    def testTanhGrad(self):
        for dtype in FLOAT_COMPUTE_TYPE:
            # test tailed_no_tailed_size
            for in_size in tailed_no_tailed_size:
                self._test_impl([in_size], dtype)
            
if __name__ == '__main__':
    test.main()    
