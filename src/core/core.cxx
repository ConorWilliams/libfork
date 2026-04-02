export module libfork.core;

// This module contains the core components of libfork, this includes:
//
// task/promise
// schedule
// concepts
// polymorphic context
// conformant stacks

// T1 partitions
export import :concepts;
export import :constants;
export import :utility;
export import :tuple;
export import :geometric_stack;
export import :dummy_stack;

// T2 partitions
export import :frame;         // concepts, constants
export import :deque;         // concepts, constants, utility
export import :thread_locals; // concepts
export import :poly_context;  // concepts

// T3 partitions
export import :ops;     // concepts, utility, tuple, frame
export import :handles; // frame

// T4 partitions
export import :promise; // concepts, frame, utility, thread_locals, ops, handles

// T5 partitions
export import :schedule; // concepts, frame, thread_locals, promise
