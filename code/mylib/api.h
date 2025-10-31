// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <splat/symbol_export.h>

#if PAR_SHARED
#   if BUILDING_PAR
#       define PAR_API SYMBOL_EXPORT
#   else
#       define PAR_API SYMBOL_IMPORT
#   endif
#else
#   define PAR_API
#endif
