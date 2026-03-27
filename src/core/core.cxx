export module libfork.core;

// T1 partitions
export import :concepts;
export import :constants;
export import :utility;
export import :tuple;

// T2 partitions
export import :frame;         // concepts, constants, utility
export import :deque;         // concepts, constants, utility
export import :thread_locals; // concepts
export import :poly_context;  // concepts

// T3 partitions
export import :ops;     // concepts, utility, tuple, frame
export import :handles; // frame

// T4 partitions
export import :promise; // concepts, frame, utility, poly_context, thread_locals, ops, handles

// T5 partitions
export import :schedule; // concepts, frame, poly_context, thread_locals, promise
