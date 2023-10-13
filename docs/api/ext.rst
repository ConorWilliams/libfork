Extension API
=========================

Writing  schedulers
-----------------------------

A scheduler has an associated ``thread_context`` type. A scheduler is 
expected to deploy a number of worker threads each of which has an instance
of the schedulers context type. These worker threads must call ``worker_init``
and ``worker_finalize`` at their construction and destruction respectively.

The core concept
~~~~~~~~~~~~~~~~

.. doxygenconcept:: lf::ext::thread_context

Thread setup and teardown
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: lf::ext::worker_init

.. doxygenfunction:: lf::ext::worker_finalize

Handle types
~~~~~~~~~~~~

.. doxygenstruct:: lf::ext::task_h

.. doxygenstruct:: lf::ext::submit_h

.. doxygentypedef:: lf::ext::intruded_h

The cactus stack
~~~~~~~~~~~~~~~~

.. doxygenclass:: lf::ext::fibre_stack
    :members:

Submission lists
~~~~~~~~~~~~~~~~~~~~~~~~~

.. toctree::
   :glob: 
   :maxdepth: 1

   ext/list.rst


Exposed internals
--------------------

These classes are used to build ``libfork``s schedulers. They are exposed for reuse.

.. toctree::
   :glob: 
   :maxdepth: 1

   ext/deque.rst
   ext/buffer.rst
   ext/event_count.rst
   ext/numa.rst
   ext/random.rst


Generated listing
---------------------

.. toctree::
   ext.gen.rst



 