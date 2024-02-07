Core API
===================

The ``core`` subfolder/namespace contains the basic building blocks of the library, this is the API to describe DAG's and internal/backend machinery for memory management. To include just libfork's core use ``#include <libfork/core.hpp>``.

Enums and tags
--------------

Dispatch tags
~~~~~~~~~~~~~

.. doxygenenum:: lf::core::tag

Dispatch modifiers
~~~~~~~~~~~~~~~~~~

.. doxygennamespace:: lf::core::modifier

Concepts and traits
-----------------------------

Libfork has a concept-driven API, built upon the following concepts and traits.

Building blocks
~~~~~~~~~~~~~~~

.. doxygenconcept:: lf::core::storable

.. doxygenconcept:: lf::core::returnable

.. doxygenconcept:: lf::core::referenceable

.. doxygenconcept:: lf::core::dereferenceable

.. doxygenconcept:: lf::core::quasi_pointer

.. doxygenconcept:: lf::core::async_function_object

API
~~~

.. doxygenconcept:: lf::core::first_arg

.. doxygenconcept:: lf::core::scheduler

.. doxygenconcept:: lf::core::context_switcher
   
.. doxygenconcept:: lf::core::co_allocable

.. doxygenconcept:: lf::core::stash_exception_in_return

.. doxygenconcept:: lf::core::modifier_for

Functional
~~~~~~~~~~

.. doxygenconcept:: lf::core::async_tag_invocable

.. doxygenconcept:: lf::core::callable

.. doxygenconcept:: lf::core::rootable

.. doxygenconcept:: lf::core::forkable

.. doxygenconcept:: lf::core::async_invocable

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


Invocation
~~~~~~~~~~

.. doxygenvariable:: lf::core::just

Explicit
~~~~~~~~

.. doxygenfunction:: lf::core::resume_on

.. doxygenstruct:: lf::core::resume_on_quasi_awaitable

Advanced/generic
~~~~~~~~~~~~~~~~

.. doxygenvariable:: lf::core::dispatch

Classes
----------------

Task
~~~~

.. doxygenstruct:: lf::core::task
    :members:
    :undoc-members:

Eventually
~~~~~~~~~~

.. toctree::
   :maxdepth: 2

   eventually.rst


.. doxygentypedef:: lf::core::eventually

.. doxygentypedef:: lf::core::try_eventually



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

