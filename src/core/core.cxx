export module libfork.core;

// T1 partitions
export import :concepts;
export import :constants;
export import :frame;
export import :utility;
export import :tuple;

// T2 partitions
export import :ops;     // concepts, tuple, utility
export import :context; // frame

// T3 partitions
export import :promise; // context, ops
