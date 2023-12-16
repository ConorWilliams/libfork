Core API
===================

The ``core`` namespace contains the basic building blocks of the library.  
To include all of ``core`` use ``#include <libfork/core.hpp>``.

Control flow
--------------------------------

These function-like objects are used to control the flow of execution while building a directed acyclic task graph.

.. doxygenfunction:: lf::core::sync_wait

.. doxygenvariable:: lf::core::fork

.. doxygenvariable:: lf::core::call

.. doxygenvariable:: lf::core::join


Concepts
------------

.. doxygenconcept:: lf::core::scheduler

.. doxygenconcept:: lf::core::returnable

.. doxygenconcept:: lf::core::dereferenceable

.. doxygenconcept:: lf::core::quasi_pointer

.. doxygenconcept:: lf::core::async_function_object
   
.. doxygenconcept:: lf::core::first_arg

.. doxygenconcept:: lf::core::async_invocable

.. doxygenconcept:: lf::core::invocable

.. doxygenconcept:: lf::core::rootable

.. doxygenconcept:: lf::core::forkable

.. doxygenconcept:: lf::core::co_allocable


Other
----------------

.. doxygenclass:: lf::core::defer
    :members:
    :undoc-members:

.. doxygentypedef:: async_result_t

Classes
------------

.. toctree::
   :glob: 
   :maxdepth: 2

   core/task.rst
   core/eventually.rst

Stack allocation
------------------

.. doxygenfunction:: lf::core::co_new()

.. doxygenfunction:: lf::core::co_new(std::size_t count)

.. doxygenfunction:: lf::core::co_delete(T *ptr)

.. doxygenfunction:: lf::core::co_delete(std::span<T, Extent> span)

Generated listing
---------------------

.. toctree::
   core.gen.rst





   



 