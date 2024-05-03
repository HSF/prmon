#! /usr/bin/env python
#
# This is a slightly adapted "hello, world" script from
# pycuda, that can be used for stressing a CUDA GPU for
# tests
#
# pycuda is required!
#

import pycuda.autoinit
import pycuda.driver as drv
import numpy
from time import time

from pycuda.compiler import SourceModule
mod = SourceModule("""
__global__ void multiply_them(float *dest, float *a, float *b, float *c)
{
  const int i = threadIdx.x;
  dest[i] = a[i] * b[i] + c[i];
}
""")

multiply_them = mod.get_function("multiply_them")

a = numpy.random.randn(1024).astype(numpy.float32)
b = numpy.random.randn(1024).astype(numpy.float32)
c = numpy.random.randn(1024).astype(numpy.float32)

dest = numpy.zeros_like(a)

start = time()
while (time() - start < 20):
    multiply_them(
            drv.Out(dest), drv.In(a), drv.In(b), drv.In(c),
            block=(1024,1,1), grid=(1,1))

print(dest-a*b+c)
