/*
 * Copyright Â© 2023 I2P
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the âSoftwareâ), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED âAS ISâ, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://git.idk.i2p/i2p-hackers/libsam3/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../src/ext/tinytest.h"
#include "../../src/ext/tinytest_macros.h"
#include "../../src/libsam3/libsam3.h"

static int testb32(const char *src, const char *res) {
  size_t dlen = sam3Base32EncodedLength(strlen(src)), len;
  char dest[128];
  //
  len = sam3Base32Encode(dest, sizeof(dest), src, strlen(src));
  tt_int_op(len, ==, dlen);
  tt_int_op(len, ==, strlen(res));
  tt_str_op(res, ==, dest);
  return 1;

end:
  return 0;
}

void test_b32_encode(void *data) {
  (void)data; /* This testcase takes no data. */

  tt_assert(testb32("", ""));
  tt_assert(testb32("f", "my======"));
  tt_assert(testb32("fo", "mzxq===="));
  tt_assert(testb32("foo", "mzxw6==="));
  tt_assert(testb32("foob", "mzxw6yq="));
  tt_assert(testb32("fooba", "mzxw6ytb"));
  tt_assert(testb32("foobar", "mzxw6ytboi======"));

end:;
}

struct testcase_t b32_tests[] = {{
                                     "encode",
                                     test_b32_encode,
                                 },
                                 END_OF_TESTCASES};
