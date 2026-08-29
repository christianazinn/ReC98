// OP and MAIN use the same conventional-memory admission implementation, but
// Turbo C derives object names from the source basename. Keep this unique
// wrapper so the two BINARY-specialized builds cannot overwrite one object.
#include "th02/main/memory_budget.cpp"
