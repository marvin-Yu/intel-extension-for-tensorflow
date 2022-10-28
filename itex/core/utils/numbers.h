/* Copyright (c) 2021 Intel Corporation

Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef ITEX_CORE_UTILS_NUMBERS_H_
#define ITEX_CORE_UTILS_NUMBERS_H_

#include <string>

#include "itex/core/utils/platform_types.h"
#include "itex/core/utils/stringpiece.h"
#include "itex/core/utils/types.h"

namespace itex {
namespace strings {

// ----------------------------------------------------------------------
// FastIntToBufferLeft()
//    These are intended for speed.
//
//    All functions take the output buffer as an arg.  FastInt() uses
//    at most 22 bytes, FastTime() uses exactly 30 bytes.  They all
//    return a pointer to the beginning of the output, which is the same as
//    the beginning of the input buffer.
//
//    NOTE: In 64-bit land, sizeof(time_t) is 8, so it is possible
//    to pass to FastTimeToBuffer() a time whose year cannot be
//    represented in 4 digits. In this case, the output buffer
//    will contain the string "Invalid:<value>"
// ----------------------------------------------------------------------

// Previously documented minimums -- the buffers provided must be at least this
// long, though these numbers are subject to change:
//     Int32, UInt32:                   12 bytes
//     Int64, UInt64, Int, Uint:        22 bytes
//     Time:                            30 bytes
// Use kFastToBufferSize rather than hardcoding constants.
static const int kFastToBufferSize = 32;

// ----------------------------------------------------------------------
// FastInt32ToBufferLeft()
// FastUInt32ToBufferLeft()
// FastInt64ToBufferLeft()
// FastUInt64ToBufferLeft()
//
// These functions convert their numeric argument to an ASCII
// representation of the numeric value in base 10, with the
// representation being left-aligned in the buffer.  The caller is
// responsible for ensuring that the buffer has enough space to hold
// the output.  The buffer should typically be at least kFastToBufferSize
// bytes.
//
// Returns the number of characters written.
// ----------------------------------------------------------------------

size_t FastInt32ToBufferLeft(int32 i, char* buffer);    // at least 12 bytes
size_t FastUInt32ToBufferLeft(uint32 i, char* buffer);  // at least 12 bytes
size_t FastInt64ToBufferLeft(int64 i, char* buffer);    // at least 22 bytes
size_t FastUInt64ToBufferLeft(uint64 i, char* buffer);  // at least 22 bytes

// Required buffer size for DoubleToBuffer is kFastToBufferSize.
// Required buffer size for FloatToBuffer is kFastToBufferSize.
size_t DoubleToBuffer(double value, char* buffer);
size_t FloatToBuffer(float value, char* buffer);

// Convert a 64-bit fingerprint value to an ASCII representation.
std::string FpToString(Fprint fp);

// Attempt to parse a fingerprint in the form encoded by FpToString.  If
// successful, stores the fingerprint in *fp and returns true.  Otherwise,
// returns false.
bool StringToFp(const std::string& s, Fprint* fp);

// Convert a 64-bit fingerprint value to an ASCII representation that
// is terminated by a '\0'.
// Buf must point to an array of at least kFastToBufferSize characters
StringPiece Uint64ToHexString(uint64 v, char* buf, int buf_size);

// Attempt to parse a uint64 in the form encoded by FastUint64ToHexString.  If
// successful, stores the value in *v and returns true.  Otherwise,
// returns false.
bool HexStringToUint64(const StringPiece& s, uint64* v);

// Convert strings to 32bit integer values.
// Leading and trailing spaces are allowed.
// Return false with overflow or invalid input.
bool safe_strto32(StringPiece str, int32* value);

// Convert strings to unsigned 32bit integer values.
// Leading and trailing spaces are allowed.
// Return false with overflow or invalid input.
bool safe_strtou32(StringPiece str, uint32* value);

// Convert strings to 64bit integer values.
// Leading and trailing spaces are allowed.
// Return false with overflow or invalid input.
bool safe_strto64(StringPiece str, int64* value);

// Convert strings to unsigned 64bit integer values.
// Leading and trailing spaces are allowed.
// Return false with overflow or invalid input.
bool safe_strtou64(StringPiece str, uint64* value);

// Convert strings to floating point values.
// Leading and trailing spaces are allowed.
// Values may be rounded on over- and underflow.
// Returns false on invalid input or if `strlen(value) >= kFastToBufferSize`.
bool safe_strtof(StringPiece str, float* value);

// Convert strings to double precision floating point values.
// Leading and trailing spaces are allowed.
// Values may be rounded on over- and underflow.
// Returns false on invalid input or if `strlen(value) >= kFastToBufferSize`.
bool safe_strtod(StringPiece str, double* value);

inline bool ProtoParseNumeric(StringPiece s, int32* value) {
  return safe_strto32(s, value);
}

inline bool ProtoParseNumeric(StringPiece s, uint32* value) {
  return safe_strtou32(s, value);
}

inline bool ProtoParseNumeric(StringPiece s, int64* value) {
  return safe_strto64(s, value);
}

inline bool ProtoParseNumeric(StringPiece s, uint64* value) {
  return safe_strtou64(s, value);
}

inline bool ProtoParseNumeric(StringPiece s, float* value) {
  return safe_strtof(s, value);
}

inline bool ProtoParseNumeric(StringPiece s, double* value) {
  return safe_strtod(s, value);
}

// Convert strings to number of type T.
// Leading and trailing spaces are allowed.
// Values may be rounded on over- and underflow.
template <typename T>
bool SafeStringToNumeric(StringPiece s, T* value) {
  return ProtoParseNumeric(s, value);
}

// Converts from an int64 to a human readable string representing the
// same number, using decimal powers.  e.g. 1200000 -> "1.20M".
std::string HumanReadableNum(int64 value);

// Converts from an int64 representing a number of bytes to a
// human readable string representing the same number.
// e.g. 12345678 -> "11.77MiB".
std::string HumanReadableNumBytes(int64 num_bytes);

// Converts a time interval as double to a human readable
// string. For example:
//   0.001       -> "1 ms"
//   10.0        -> "10 s"
//   933120.0    -> "10.8 days"
//   39420000.0  -> "1.25 years"
//   -10         -> "-10 s"
std::string HumanReadableElapsedTime(double seconds);

}  // namespace strings
}  // namespace itex

#endif  // ITEX_CORE_UTILS_NUMBERS_H_
