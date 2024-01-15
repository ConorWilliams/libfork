Extension API
=============

Worker API
----------

.. doxygentypedef:: lf::ext::nullary_function_t

.. doxygentypedef:: lf::ext::task_handle

.. doxygentypedef:: lf::ext::submit_handle

Lifetime management
~~~~~~~~~~~~~~~~~~~

A worker thread is registered/destroyed by calling the appropriate functions:

.. doxygenfunction:: lf::ext::worker_init

.. doxygenfunction:: lf::ext::finalize

A worker's context
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: lf::ext::worker_context
   :members:

Worker functions
~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: lf::ext::resume(task_handle ptr)

.. doxygenfunction:: lf::ext::resume(submit_handle ptr)


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








