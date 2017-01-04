// ================================================================ //
//                                                                  //
//   File      : cxxforward.h                                       //
//   Purpose   : macros for forward usage of C++11 features         //
//               w/o loosing compatibility to C++03 compilers       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2012   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef CXXFORWARD_H
#define CXXFORWARD_H

#ifndef GCCVER_H
#include "gccver.h"
#endif

#if defined(__cplusplus)
# if (GCC_VERSION_CODE >= 407)
#  if (__cplusplus == 199711L)
#  else
#   if (__cplusplus == 201103L)
#    define ARB_ENABLE_Cxx11_FEATURES
#   else
#    if (__cplusplus == 201402L)
#     define ARB_ENABLE_Cxx11_FEATURES
// #     define ARB_ENABLE_Cxx14_FEATURES // not needed yet
#    else
#     error Unknown C++ standard defined in __cplusplus
#    endif
#   endif
#  endif
# endif
#else
# warning C compilation includes cxxforward.h
#endif

#ifdef ARB_ENABLE_Cxx11_FEATURES

// C++11 is enabled starting with gcc 4.7 in ../Makefile@USE_Cxx11
//
// Full support for C++11 is available starting with gcc 4.8.
// Use #ifdef Cxx11 to insert conditional sections using full C++11
# if (GCC_VERSION_CODE >= 408)
#  define Cxx11 1
# endif

// allows static member initialisation in class definition:
# define CONSTEXPR        constexpr
# define CONSTEXPR_RETURN constexpr

// allows to protect overloading functions against signature changes of overload functions:
# define OVERRIDE override

// allows additional optimization of virtual calls
// (does not only allow to replace virtual by a normal call, it also allows inlining!)
# define FINAL_TYPE     final
# define FINAL_OVERRIDE final override

#else
// backward (non C++11) compatibility defines:
# define CONSTEXPR        const
# define CONSTEXPR_RETURN
# define OVERRIDE
# define FINAL_TYPE
# define FINAL_OVERRIDE

#endif

// allow to hide unwanted final suggestions
#ifdef SUGGESTS_FINAL

# define NF_JOIN(X,Y) X##Y
# define MARK_NONFINAL_CLASS(BASE)                      \
    namespace final_unsuggest {                         \
        struct NF_JOIN(unfinalize,BASE) final : BASE {  \
        };                                              \
    }
// PARAMS has to contain parentheses and attributes like const!
# define MARK_NONFINAL_METHOD(BASE,RETURN_TYPE,METHOD_NAME,PARAMS)      \
    namespace final_unsuggest {                                         \
        struct NF_JOIN(BASE,METHOD_NAME) final : BASE                   \
        {                                                               \
            inline RETURN_TYPE METHOD_NAME PARAMS override;             \
        };                                                              \
    }

#else

# define MARK_NONFINAL_CLASS(BASE)
# define MARK_NONFINAL_METHOD(BASE,RETURN_TYPE,METHOD_NAME,PARAMS)

#endif

#define MARK_NONFINAL_DTOR(BASE) MARK_NONFINAL_CLASS(BASE)

#else
#error cxxforward.h included twice
#endif // CXXFORWARD_H
