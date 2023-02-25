task.hpp
=====================


.. doxygenfile:: task.hpp
    :sections: briefdescription detaileddescription

.. doxygenconcept:: lf::context

Tasks
------------------

.. doxygenclass:: lf::basic_task
    :members:
    :undoc-members:

Futures
-------------------

.. doxygenclass:: lf::basic_future
    :members:
    :undoc-members:

Handles
--------------

.. doxygentypedef:: lf::work_handle

.. doxygentypedef:: lf::root_handle


-------------------------------------

.. doxygenclass:: lf::task_handle
    :members:
    :undoc-members:


Functions
----------------------

.. doxygenfunction:: lf::join

-------------------------------------

.. doxygenfunction:: lf::get_context

-------------------------------------

.. doxygenfunction:: lf::as_root(basic_task<T, Context, Allocator, false> &&task)

.. doxygenfunction:: lf::as_root(basic_task<T, Context, Allocator, true> &&task)


