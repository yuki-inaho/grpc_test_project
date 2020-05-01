import os
import sys
sys.path.append(os.getcwd())
import numpy as np
import pytest
from utils import ndarray_to_bytes, bytes_to_ndarray

@pytest.mark.parametrize(
    "ndary",
    [
        np.eye(3, dtype=np.int8),
        np.eye(3, dtype=np.int16),
    ],
)

def test_numproto(ndary):
    result = bytes_to_ndarray(ndarray_to_bytes(ndary))
    assert np.array_equal(ndary, result)