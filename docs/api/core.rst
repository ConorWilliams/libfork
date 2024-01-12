Core API
===================

The ``core`` subfolder/namespace contains the basic building blocks of the library, this is the API to describe DAG's and internal/backend machinery for memory management. To include just libfork's core use ``#include <libfork/core.hpp>``.

Enums
-----

.. doxygenenum:: lf::core::tag

Concepts and traits
-----------------------------

Libfork has a concept-driven API, built upon the following concepts.

Building blocks
~~~~~~~~~~~~~~~

.. doxygenconcept:: lf::core::returnable

.. doxygenconcept:: lf::core::dereferenceable

.. doxygenconcept:: lf::core::quasi_pointer

API
~~~

.. doxygenconcept:: lf::core::async_function_object
   
.. doxygenconcept:: lf::core::first_arg

.. doxygenconcept:: lf::core::scheduler

.. doxygenconcept:: lf::core::co_allocable

.. doxygenconcept:: lf::core::external_awaitable

Functional
~~~~~~~~~~

.. doxygenconcept:: lf::core::async_invocable

.. doxygenconcept:: lf::core::invocable

.. doxygenconcept:: lf::core::rootable

.. doxygenconcept:: lf::core::forkable

Utility
.......

.. doxygentypedef:: async_result_t


Control flow
--------------------------------

Blocking 
~~~~~~~~~

.. doxygenfunction:: lf::core::sync_wait

Fork-join
~~~~~~~~~~~~

.. doxygenvariable:: lf::core::fork

.. doxygenvariable:: lf::core::call

.. doxygenvariable:: lf::core::join


Explicit
~~~~~~~~

.. doxygenfunction:: lf::core::resume_on

.. doxygenstruct:: lf::core::schedule_on_context

Classes
----------------

Task
~~~~

.. doxygenstruct:: lf::core::task
    :members:
    :undoc-members:

Eventually
~~~~~~~~~~

.. doxygenclass:: lf::core::eventually
   :members:
   :undoc-members:


Defer
~~~~~

.. doxygenclass:: lf::core::defer
    :members:
    :undoc-members:


Stack allocation
------------------

Functions
~~~~~~~~~

.. doxygenfunction:: lf::core::co_new

Classes
~~~~~~~

.. doxygenclass:: lf::core::stack_allocated
   :members:

