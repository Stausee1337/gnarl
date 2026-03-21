
#ifndef GNARL_FUNCTIONS_H_
#define GNARL_FUNCTIONS_H_

#include <vector>
#include "nodes.h"

namespace gnarl {

// LFF: linkage flat function
//      args
// LFM: linkage flat macro
//      args node
// LBF: linkage block function
//      args, block node
// LBM: linkage block macro
//      args node, block node

#define BUILTIN_LIST(LFF, LFM, LBF, LBM)    \
    LFF(assert)                             \
    LFF(forward_variables_from)             \
    LFF(get_path_info)                      \
    LFF(getenv)                             \
    LFF(import)                             \
    LFF(len)                                \
    LFF(not_needed)                         \
    LFF(print)                              \
    LFF(split_list)                         \
    LFF(string_join)                        \
    LFF(string_replace)                     \
    LFF(string_split)                       \
    LFM(defined)                            \
    LBM(foreach)                            \
    LBF(declare_args)                       \
    LBF(template)

#define CONTEXT Scope* scope, Error* error, const Span& call

#define DECL_FLAT_FUNC(name) \
    Value builtin_##name(CONTEXT, const std::vector<Value>& args);

#define DECL_FLAT_MACRO(name) \
    Value builtin_##name(CONTEXT, const ListNode& args_node);

#define DECL_BLOCK_FUNC(name) \
    Value builtin_##name(CONTEXT, const std::vector<Value>& args, const BlockNode& block);

#define DECL_BLOCK_MACRO(name) \
    Value builtin_##name(CONTEXT, const ListNode& args_node, const BlockNode& block);

BUILTIN_LIST(DECL_FLAT_FUNC, DECL_FLAT_MACRO, DECL_BLOCK_FUNC, DECL_BLOCK_MACRO)

#undef DECL_FLAT_FUNC
#undef DECL_BLOCK_MACRO

#undef CONTEXT
}

#endif // GNARL_FUNCTIONS_H_

