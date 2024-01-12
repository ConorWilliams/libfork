Extension API
=============

Worker API
----------

A worker thread is registered by calling the appropriate functions.

.. doxygenfunction:: lf::ext::worker_init

.. doxygenfunction:: lf::ext::finalize


The worker's context
~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: lf::ext::worker_context
   :members:

Worker functions
~~~~~~~~~~~~~~~~

.. doxygenfunction:: lf::ext::resume(task_handle ptr)

.. doxygenfunction:: lf::ext::resume(submit_handle ptr)


Types
-----

.. doxygentypedef:: lf::ext::nullary_function_t

---------------------------------------------------

.. doxygenclass:: lf::ext::task_t
   :members:

.. doxygentypedef:: lf::ext::task_handle

---------------------------------------------------

.. doxygenclass:: lf::ext::submit_t
   :members:

.. doxygentypedef:: lf::ext::submit_handle

Containers
------------

.. toctree::
   :maxdepth: 2

   ext/list.rst
   ext/deque.rst


Components
----------

.. toctree::
   :maxdepth: 2

   ext/event_count.rst
   ext/random.rst
   ext/numa.rst

Interface types
---------------



Worker context
...............






