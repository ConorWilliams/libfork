/*
 * Copyright (c) 2012 David Rodrigues
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __MACROLOGGER_H__
#define __MACROLOGGER_H__

#include <string.h>

namespace riften::detail {

static thread_local std::size_t static_id;

#define RIFTEN_FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define NO_LOG 0x00
#define DEBUG_LEVEL 0x01

#ifndef LOG_LEVEL
#    define LOG_LEVEL NO_LOG
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#    define LOG_DEBUG(message)                            \
        fprintf(stderr,                                   \
                "%-12s%5d | %-15s | %2ld |" message "\n", \
                RIFTEN_FILE,                              \
                __LINE__,                                 \
                __FUNCTION__,                             \
                riften::detail::static_id)
#else
#    define LOG_DEBUG(message)
#endif

}  // namespace riften::detail

#endif