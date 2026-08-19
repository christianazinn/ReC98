// continue_prompt() comes first: it is where EXECL_TEXT's root contribution
// now ends, so this object has to append it ahead of score_last_commit() and
// GameExecl() to leave every byte at its original address. It cannot live in
// th04/main/execl.cpp, which th05/execl.cpp #includes as a shared body.
// (kb/codegen/0129)
#include "th04/main/continue.cpp"
#include "th04/main/execl.cpp"
