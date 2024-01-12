Extension API
=============

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

These types are components of the scheduler API.

.. doxygentypedef:: lf::ext::nullary_function_t

.. doxygentypedef:: lf::ext::intruded_list

.. doxygenclass:: lf::ext::task_t
   :members:

.. doxygenclass:: lf::ext::submit_t
   :members:

.. doxygentypedef:: lf::ext::task_handle

.. doxygentypedef:: lf::ext::submit_handle

Worker context
...............

.. doxygenclass:: lf::ext::worker_context
   :members:

Scheduler API functions
-----------------------

.. doxygenfunction:: lf::ext::worker_init

.. doxygenfunction:: lf::ext::finalize

.. doxygenfunction:: lf::ext::resume(task_handle ptr)

.. doxygenfunction:: lf::ext::resume(submit_handle ptr)



