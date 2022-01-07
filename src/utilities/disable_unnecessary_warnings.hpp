#pragma once


// The warnings could be disabled through the
// project settings, but this allows comments.


#ifdef _MSC_VER // MS warnings only

#pragma push_macro("DISABLE_HELPER")
#pragma push_macro("DISABLE")
#undef DISABLE_HELPER
#undef DISABLE

// The helper is necessary because _Pragma does not support string concatenation
#define DISABLE_HELPER(s) _Pragma(#s)
#define DISABLE(n) DISABLE_HELPER(warning(disable: n))



DISABLE(4710) // function not inlined
DISABLE(4711) // function selected for automatic inline expansion

DISABLE(4866) // left-to-right evaluation not enforced



#pragma pop_macro("DISABLE_HELPER")
#pragma pop_macro("DISABLE")

#endif // defined _MSVC_VER