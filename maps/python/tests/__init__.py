import glob
import os
import unittest
import time

import Atrinik


def run():
    import tests.Atrinik_tests.Atrinik
    import tests.Atrinik_tests.Object

    all_suites = []
    all_suites += tests.Atrinik_tests.Atrinik.suites
    all_suites += tests.Atrinik_tests.Object.suites

    all_tests = unittest.TestSuite(all_suites)

    try:
        import xmlrunner
        path = "tests/unit/plugins/python"

        if os.path.exists(path):
            for name in glob.glob(os.path.join(path, "*.xml")):
                os.unlink(name)

        xmlrunner.XMLTestRunner(output=path).run(all_tests)
    except ImportError:
        unittest.TextTestRunner(verbosity=2).run(all_tests)


def simulate_server(seconds=None, count=None, wait=True, before_cb=None,
                    after_cb=None, **kwargs):
    """
    Simulates the server game loop using :func:`Atrinik.Process`.

    :param seconds: How many seconds to simulate for.
    :type seconds: float
    :param count: How many times to iterate the game loop.
    :type count: int
    :param wait: Whether to wait the correct amount of time after a single
                 iteration (emulating the ticks behavior).
    :type wait: bool
    :param before_cb: Optional function to call before calling
                      :func:`Atrinik.Process`.
    :type before_cb: collections.Callable
    :param after_cb: Optional function to after before calling
                     :func:`Atrinik.Process`.
    :type after_cb: collections.Callable
    :param \**kwargs: Rest of keyword arguments are passed to *before_cb* and
                      *after_cb*.
    :return: How many times the loop was iterated.
    :rtype: int
    """

    assert(count is not None or seconds is not None)

    now = time.time()
    i = 0
    while ((count is not None and i < count) or
           (seconds is not None and time.time() - now < seconds)):
        i += 1

        if before_cb is not None:
            before_cb(**kwargs)

        start = time.time()
        Atrinik.Process()
        end = time.time()

        if after_cb is not None:
            after_cb(**kwargs)

        if not wait:
            continue

        to_wait = (Atrinik.MAX_TIME - (end - start)) / 1000000.0
        if to_wait > 0.0:
            time.sleep(to_wait)

    return i

__all__ = ["run", "simulate_server"]
