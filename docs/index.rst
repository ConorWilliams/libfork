Welcome to ``libfork`` v3.5.0
============================================

This is libfork's API documentation, for info on building and installing please see the `README.md <https://github.com/ConorWilliams/libfork>`_.  

Core API
---------------------------

.. doxygennamespace:: lf::core
    :desc-only:
    :no-link:

.. toctree::
   :maxdepth: 3

   api/core.rst

Schedulers
------------------------

Schedulers are objects that map the computation graph to worker threads, ``libfork`` supplies a selection of useful defaults.

.. toctree::
   :maxdepth: 2

   api/schedule.rst


Algorithms
------------------------

Algorithms operate at a higher level of abstraction than pure fork-join, currently ``libfork`` does not have many but watch this space...

.. toctree::
   :maxdepth: 3

   api/algorithm.rst


Extension API
---------------------------

.. doxygennamespace:: lf::ext
    :desc-only:
    :no-link:  

.. toctree::
   :maxdepth: 2

   api/ext.rst


.. Macros in ``libfork``
.. ---------------------------

.. .. doxygenfile:: libfork/core/macro.hpp


Internal documentation
---------------------------

.. doxygennamespace:: lf::impl
    :desc-only:
    :no-link:

.. toctree::
   :maxdepth: 3

   api/impl.rst
















