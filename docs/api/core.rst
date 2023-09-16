Core
===================

The core module contains the basic building blocks of the library. 

Concepts
------------

.. doxygenconcept:: lf::core::stateless

.. doxygenconcept:: lf::core::first_arg

.. literalinclude:: ../../include/libfork/core/task.hpp
   :start-at: template <typename Arg>
   :end-before: // !END-GRAB


Control flow
--------------------------------

.. doxygenfunction:: lf::core::sync_wait

.. doxygenvariable:: lf::core::fork

.. doxygenvariable:: lf::core::call

.. doxygenvariable:: lf::core::join

.. doxygenvariable:: lf::core::tail


Classes
------------

.. toctree::
   :glob: 
   :maxdepth: 2

   core/*


Automatic listing:
---------------------

.. toctree::
   core.gen.rst





   



 