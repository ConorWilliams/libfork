Core API
===================

The ``core`` namespace contains the basic building blocks of the library. 

Control flow
--------------------------------

These function-like objects are used to control the flow of execution while building a directed acyclic task graph.

.. doxygenfunction:: lf::core::sync_wait

.. doxygenvariable:: lf::core::fork

.. doxygenvariable:: lf::core::call

.. doxygenvariable:: lf::core::join

.. doxygenvariable:: lf::core::tail

Concepts
------------

.. doxygenconcept:: lf::core::scheduler

.. doxygenconcept:: lf::core::stateless

.. doxygenconcept:: lf::core::first_arg

Schedulers
--------------




Other classes
------------

.. toctree::
   :glob: 
   :maxdepth: 2

   core/async.rst
   core/eventually.rst
   core/task.rst
   core/fixed.rst
   core/result.rst
   core/in_place.rst



Generated listing
---------------------

.. toctree::
   core.gen.rst





   



 