libfork.hpp
=====================

.. doxygenfile:: libfork.hpp
    :sections: briefdescription detaileddescription


Interfaces
-------------------------

.. doxygenconcept:: lf::scheduler

.. doxygenconcept:: lf::thread_context

.. doxygenconcept:: lf::first_arg

.. doxygenconcept:: lf::stateless

.. doxygenconcept:: lf::no_forked_rvalue

-------------------------

.. doxygenstruct:: lf::first_arg_t

.. doxygenstruct:: lf::first_arg_t< R, Tag, async_fn< F > >
    :members:
    :undoc-members:

.. doxygenstruct:: lf::first_arg_t< R, Tag, async_mem_fn< F >, Self >
    :members:
    :undoc-members:

Building asynchronous functions
--------------------------------

.. doxygenclass:: lf::task
    :members:
    :undoc-members:

.. doxygenstruct:: lf::async_fn
    :members:
    :undoc-members:

.. doxygenstruct:: lf::async_mem_fn
    :members:
    :undoc-members:


Asynchronous control flow
----------------------------

.. doxygenvariable:: lf::fork

.. doxygenvariable:: lf::call

.. doxygenvariable:: lf::join

.. doxygenfunction:: lf::sync_wait(S &&scheduler, async_fn<F> async_function, Args&&... args)

.. doxygenfunction:: lf:: sync_wait(S &&scheduler, async_mem_fn<F> async_member_function, Self &&self, Args&&... args)

---------------------------

.. doxygenenum:: lf::tag

.. doxygenstruct:: lf::bind_task
    :members:
    :undoc-members:

Virtual stacks
------------------------------

.. doxygenclass:: lf::virtual_stack
    :members:
    :undoc-members:

