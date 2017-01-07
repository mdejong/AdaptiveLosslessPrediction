//
//  EncDec.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  C++ templates for encoding and decoding numbers as bytes.

#include "assert.h"

#include <vector>

#include <unordered_map>

using namespace std;

// Encode a signed delta +-N as a positive number starting at zero where
// +1 is 1 and -1 is 2 and so on.

static inline
uint32_t
convertSignedZeroDeltaToUnsigned(int delta) {
  if (delta == 0) {
    return 0;
  } else if (delta > 0) {
    // Positive value
    
    // 1 -> 1
    // 2 -> 3
    // 3 -> 5
    
    return (delta * 2) - 1;
  } else {
    // Negative value
    // -1 -> 2
    // -2 -> 4
    // -3 -> 6
    
    return (delta * -2);
  }
}

static inline
int
convertUnsignedZeroDeltaToSigned(uint32_t delta) {
  if (delta == 0) {
    return 0;
  } else if ((delta & 0x1) == 1) {
    // Odd value means positive delta
    
    // 1 -> 1
    // 3 -> 2
    // 5 -> 3
    
    return (int) ((delta >> 1) + 1);
  } else {
    // Even value means negative delta
    
    // 2 -> -1
    // 4 -> -2
    // 6 -> -3
    
    int s = (delta >> 1);
    return (s * -1);
  }
}

// Convert a signed delta to an unsigned delta that takes
// care to treat the special case of the largest possible
// value as a negative number.

static inline
uint32_t
convertSignedZeroDeltaToUnsignedNbits(int delta, const int numBitsMax) {
  const unsigned int unsignedMaxValue = ~((~((unsigned int)0)) << numBitsMax);
  const unsigned int absMaxValue = (~((~((unsigned int)0)) << (numBitsMax - 1))) + 1;
  const int signedMaxValue = -1 * absMaxValue;
  
  if (delta == signedMaxValue) {
    // For example, at 4 bits the maximum value is -8 -> 0xFF
    return unsignedMaxValue;
  } else {
    return convertSignedZeroDeltaToUnsigned(delta);
  }
}

static inline
int
convertUnsignedZeroDeltaToSignedNbits(uint32_t delta, const int numBitsMax) {
  const unsigned int unsignedMaxValue = ~((~((unsigned int)0)) << numBitsMax);
  const unsigned int absMaxValue = (~((~((unsigned int)0)) << (numBitsMax - 1))) + 1;
  const int signedMaxValue = -1 * absMaxValue;
  
  if (delta == unsignedMaxValue) {
    // For example, at 4 bits the maximum value is 0xFF -> -8
    return (int)signedMaxValue;
  } else {
    return convertUnsignedZeroDeltaToSigned(delta);
  }
}

// When dealing with a table of N values, a general purpose "wraparound"
// delta from one element to another can be encoded to take the size of
// table into account. For example, for a table of 3 values (0 10 30)
// a delta from (30 to 0) can be represented by -2 but a smaller rep
// would be +1 where adding one to the offset wraps around to the start.

#define WRAPPED_ALLOW_N_1 1

static inline
int
convertToWrappedTableDelta(unsigned int off1, unsigned int off2, const unsigned int N) {
  const bool debug = false;
  
  if (debug) {
    printf("convertToWrappedTableDelta %d %d and N = %d\n", off1, off2, N);
  }

#if defined(DEBUG)
# if defined(WRAPPED_ALLOW_N_1)
# else
  assert(N > 1);
# endif // WRAPPED_ALLOW_N_1
  assert(off1 < N);
  assert(off2 < N);
#endif // DEBUG
  
  // delta indicates move needed to adjust off1 to match off2
  
  int delta = off2 - off1;
  
  if (debug) {
    printf("delta %d\n", (int)delta);
  }
  
#if defined(DEBUG)
  if (abs(delta) >= N) {
    // For example, 4 bit range is (-15, 15)
    assert(0);
  }
#endif // DEBUG
  
  // For (-15, 15) mid = 7, negMid = -8
  // For (-3, 3)   mid = 1, negMid = -2
  
  const int mid = N >> 1; // N / 2
  int negMid = mid * -1;
  
  if ((N & 0x1) == 0) {
    negMid += 1;
    
    if (debug) {
      printf("even wrap range adjust upward %d\n", (int)negMid);
    }
  }
  
  if (debug) {
    printf("wrap range (%d,0,%d)\n", (int)negMid, (int)mid);
  }
  
  int wrappedDelta = delta;
  
  if (delta > mid) {
    wrappedDelta = ((int)N - delta) * -1;
    
    if (debug) {
      printf("wrappedDelta : %d - %d = %d\n", (int)N, (int)delta, wrappedDelta);
    }
  } else if (delta < negMid) {
    // Note that a positive delta that reaches the same offset is preferred
    // as compare to a negative delta.
    
    wrappedDelta = (int)N + delta;
    
    if (debug) {
      printf("wrappedDelta : %d + %d = %d\n", (int)N, (int)delta, wrappedDelta);
    }
  }
  
  return wrappedDelta;
}

// A wrapped table delta should be added to the previous offset
// to regenerate a table offset. The returned value is the table
// offset after the delta has been applied.

static inline
uint32_t
convertFromWrappedTableDelta(uint32_t offset, int wrappedDelta, const unsigned int N) {
  const bool debug = false;
  
#if defined(DEBUG)
# if defined(WRAPPED_ALLOW_N_1)
# else
  assert(N > 1);
# endif // WRAPPED_ALLOW_N_1
  assert(offset < N);
  if (abs(wrappedDelta) > N) {
    // For example, 4 bit range is (-15, 15)
    assert(0);
  }
#endif // DEBUG
  
  // For (-15, 15) mid = 7, negMid = -8
  // For (-3, 3)   mid = 1, negMid = -2
  
  //const int mid = N >> 1; // N / 2
  //const int negMid = (mid + 1) * -1;
  
  int signedOffset = (int)offset + wrappedDelta;
  
  if (debug) {
    printf("offset + delta : %d + %d = %d\n", (int)offset, (int)wrappedDelta, signedOffset);
  }

  if (signedOffset < 0) {
    // Wrapped around from zero to the end
    signedOffset += N;
  } else if (signedOffset >= N) {
    // Wrapped around from end to zero
    signedOffset -= N;
  }
  
  if (debug) {
    printf("adjusted to (0,N): %d\n", (int)signedOffset);
  }
  
#if defined(DEBUG)
  assert(signedOffset >= 0);
  assert(signedOffset < N);
#endif // DEBUG
  
  return (uint32_t) signedOffset;
}

// Convert table offsets to signed offsets that are then represented
// by unsigned 32 bit values.

template <typename T>
vector<uint32_t>
convertToWrappedUnsignedTableDeltaVector(const vector<T> & inVec, const unsigned int N)
{
  int prev;
  vector<uint32_t> deltas;
  deltas.reserve(inVec.size());
  
  // The first value is always a delta from zero, so handle it before
  // the loop logic.
  
  {
    int val = inVec[0];
    deltas.push_back(val);
    prev = val;
  }
  
  int maxi = (int) inVec.size();
  for (int i = 1; i < maxi; i++) {
    int cur = inVec[i];
    int wrappedDelta = convertToWrappedTableDelta(prev, cur, N);
    uint32_t unsignedDelta = convertSignedZeroDeltaToUnsigned(wrappedDelta);
    deltas.push_back(unsignedDelta);
    prev = cur;
  }
  
  return std::move(deltas);
}

// FIXME: need to supply table N values for each block

// Read unsigned value, convert to signed, then unwrap based on table N

static inline
vector<int>
convertFromWrappedUnsignedTableDeltaVector(const vector<uint32_t> & inVec, const unsigned int N)
{
  int prev;
  vector<int> offsets;
  offsets.reserve(inVec.size());
  
  // The first value is always a delta from zero, so handle it before
  // the loop logic.
  
  {
    int val = inVec[0];
    offsets.push_back(val);
    prev = val;
  }
  
  const int maxi = (int) inVec.size();
  for (int i = 1; i < maxi; i++) {
    int unsignedDelta = inVec[i];
    int wrappedDelta = convertUnsignedZeroDeltaToSigned(unsignedDelta);
    uint32_t offset = convertFromWrappedTableDelta(prev, wrappedDelta, N);
    offsets.push_back(offset);
    prev = offset;
  }
  
  return std::move(offsets);
}


