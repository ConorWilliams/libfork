libfork/task.hpp
=====================


.. doxygenfile:: task.hpp
    :sections: briefdescription detaileddescription

.. doxygenconcept:: lf::context


.. doxygenclass:: lf::task
    :members:
    :undoc-members:


Functions
-------------

.. doxygenfunction:: lf::join

.. doxygenfunction:: lf::just(F &fut, Fn &&fun, Args&&... args)

.. doxygenfunction:: lf::just(Fn &&fun, Args&&... args)
