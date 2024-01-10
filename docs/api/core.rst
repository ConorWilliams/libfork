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

.. doxygenfunction:: lf::core::sync_wait

Fork-join
~~~~~~~~~~~~

.. doxygenvariable:: lf::core::fork

.. doxygenvariable:: lf::core::call

.. doxygenvariable:: lf::core::join


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

Context
~~~~~~~

.. doxygenclass:: lf::ext::context
   :members:

Stack allocation
------------------

.. doxygenfunction:: lf::core::co_new(std::size_t count)
.. doxygenfunction:: lf::core::co_new()

.. doxygenfunction:: lf::core::co_delete(std::span<T, Extent> span)
.. doxygenfunction:: lf::core::co_delete(T *ptr)
