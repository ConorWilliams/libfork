# Structure of libfork

Libfork is organized into several modules:

- `libfork`: Meta module that re-exports all public modules
  - tuple
  - etc
- `libfork.utils`: Independent internal utilities, not part of the public API
- `libfork.core`: Core functionality of libfork including:
  - Task template
  - Task handles
  - Concepts for context/stack/scheduler
  - Fork/call primitives
  - Execute primitives (for starting work)
  - Schedule primitive (for launching work)
  - Polymorphic context ABC
  - \[internal\] Promise/frame
  - \[internal\] Thread locals
- `libfork.batteries`: Collection of context, stack and other types
  - The `::stacks` namespace
  - Contexts
  - adaptors
- `libfork.schedulers`: Collection of schedulers
  - Inline scheduler
