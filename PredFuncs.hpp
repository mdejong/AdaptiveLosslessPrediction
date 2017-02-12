//
//  PredFuncs.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Created by Mo DeJong on 4/3/16.
//  Copyright © 2016 HelpURock. All rights reserved.
//
//  Functions to predict a pixel value given the current
//  neighbor values in a matrix.

#include "assert.h"

#include <vector>
#include <queue>
#include <set>

#include <unordered_map>

#if defined(DEBUG)
#include <iostream>
#endif // DEBUG

#import "EncDec.hpp"
#import "CalcError.h"

using namespace std;

// Component delta from one pixel to another.

static inline
uint32_t pixel_component_delta(uint32_t p1, uint32_t p2, const int numComp) {
  const bool debug = false;
  
#if defined(DEBUG)
  assert(numComp == 3 || numComp == 4);
#endif // DEBUG
  
  if (debug) {
    printf("p1 0x%08X\n", p1);
    printf("p2 0x%08X\n", p2);
  }
  
  uint32_t outPixel = 0;
  uint32_t bVal;
  
  uint32_t cp1;
  uint32_t cp2;
  
  // c0
  {
    const int comp = 0;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 - cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d - %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  // c1
  {
    const int comp = 1;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 - cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d - %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  // c2
  {
    const int comp = 2;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 - cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d - %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  if (numComp == 4) {
    // c3
    {
      const int comp = 3;
      
      cp1 = (p1 >> (comp * 8)) & 0xFF;
      cp2 = (p2 >> (comp * 8)) & 0xFF;
      
      bVal = (cp2 - cp1) & 0xFF;
      outPixel |= (bVal << (comp * 8));
      
      if (debug) {
        printf("c%d = %d : %d - %d\n", comp, bVal, cp2, cp1);
      }
    }
  }
  
  return outPixel;
}

// Component sum from one pixel to another.

static inline
uint32_t pixel_component_sum(uint32_t p1, uint32_t p2, const int numComp) {
  const bool debug = false;
  
#if defined(DEBUG)
  assert(numComp == 3 || numComp == 4);
#endif // DEBUG
  
  if (debug) {
    printf("p1 0x%08X\n", p1);
    printf("p2 0x%08X\n", p2);
  }
  
  uint32_t outPixel = 0;
  uint32_t bVal;
  
  uint32_t cp1;
  uint32_t cp2;
  
  // c0
  {
    const int comp = 0;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 + cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d + %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  // c1
  {
    const int comp = 1;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 + cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d + %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  // c2
  {
    const int comp = 2;
    
    cp1 = (p1 >> (comp * 8)) & 0xFF;
    cp2 = (p2 >> (comp * 8)) & 0xFF;
    
    bVal = (cp2 + cp1) & 0xFF;
    outPixel |= (bVal << (comp * 8));
    
    if (debug) {
      printf("c%d = %d : %d + %d\n", comp, bVal, cp2, cp1);
    }
  }
  
  if (numComp == 4) {
    // c3
    {
      const int comp = 3;
      
      cp1 = (p1 >> (comp * 8)) & 0xFF;
      cp2 = (p2 >> (comp * 8)) & 0xFF;
      
      bVal = (cp2 + cp1) & 0xFF;
      outPixel |= (bVal << (comp * 8));
      
      if (debug) {
        printf("c%d = %d : %d + %d\n", comp, bVal, cp2, cp1);
      }
    }
  }
  
  return outPixel;
}

// Execute abs() on each component

static inline
uint32_t abs_of_each_component(uint32_t deltaPixel) {
  const bool debug = false;
  
  int8_t /*c3,*/ c2, c1, c0;
  
  //c3 = (pixel >> 24) & 0xFF;
  c2 = (deltaPixel >> 16) & 0xFF;
  c1 = (deltaPixel >> 8) & 0xFF;
  c0 = (deltaPixel >> 0) & 0xFF;
  
  if (debug) {
    fprintf(stdout, "delta 0x%08X aka (%d %d %d)\n", deltaPixel, c2, c1, c0);
  }
  
  //c3 = abs(c3);
  //c3 = 0xFF;
  c2 = abs(c2);
  c1 = abs(c1);
  c0 = abs(c0);
  
  uint32_t absPixel;
  
  uint32_t c;
  
  c = c0;
  absPixel = c;
  
  c = c1;
  absPixel |= (c << 8);
  
  c = c2;
  absPixel |= (c << 16);
  
//  c = c3;
//  absPixel |= (c << 24);
  
  if (debug) {
    fprintf(stdout, "abs delta 0x%08X aka (%d %d %d)\n", absPixel, c2, c1, c0);
  }
  
  return absPixel;
}

// Convert from an unsigned by value (0, 255) to a signed int value in range (-128, 127)

static inline
int
unsigned_byte_to_signed(unsigned int bVal) {
  const int numBitsMax = 7;
  const unsigned int unsignedMaxValue = ~((~((unsigned int)0)) << numBitsMax);

  if (bVal <= unsignedMaxValue) {
    return bVal;
  } else {
    return bVal - 256;
  }
}

// Convert from signed value in the range (-128, 127) to (0, 256)

static inline
unsigned int
signed_byte_to_unsigned(int sVal) {
  if (sVal < 0) {
    return 256 - abs(sVal);
  } else {
    return ((unsigned int)sVal) & 0xFF;
  }
}

// Convert from signed value like (-128, 127) to (0, 256) (assuming maxVal = 256)

static inline
unsigned int
signed_to_unsigned(int sVal, const int maxVal) {
  if (sVal < 0) {
    return maxVal - abs(sVal);
  } else {
    return ((unsigned int)sVal) & (maxVal-1);
  }
}

// Sum the abs() values for each component, scale each component, return sum.

static inline
unsigned int
sum_of_abs_components(uint32_t pixel, const int numComp) {
  const bool debug = false;
  
  uint32_t SAC = 0;
  int8_t c3 = 0, c2, c1, c0;
  
  if (numComp == 4) {
    c3 = (pixel >> 24) & 0xFF;
  }
  c2 = (pixel >> 16) & 0xFF;
  c1 = (pixel >> 8) & 0xFF;
  c0 = (pixel >> 0) & 0xFF;
  
  if (numComp == 4) {
    SAC += abs(c3);
  }
  SAC += abs(c2);
  SAC += abs(c1);
  SAC += abs(c0);
  
  if (debug) {
    printf("sum_of_abs_components() 0x%0.8X : signed %d + %d + %d + %d : abs %d + %d + %d + %d : SAC %d\n",
           pixel,
           (int)c3,
           (int)c2,
           (int)c1,
           (int)c0,
           (int)abs(c3),
           (int)abs(c2),
           (int)abs(c1),
           (int)abs(c0),
           SAC);
  }
  
  return SAC;
}

// Execute an operation for each component in a pixel
// and return the result joined back into components.
// The function can return either a uint8_t or a uint32_t.
// numComps = 4 or 3

template <typename F>
uint32_t foreach_pixel_component(uint32_t pixel, F f, const int numComps) {
  uint32_t outPixel = 0;
  uint32_t c3;
  if (numComps == 4) {
    c3 = (pixel >> 24) & 0xFF;
  }
  uint32_t c2 = (pixel >> 16) & 0xFF;
  uint32_t c1 = (pixel >> 8) & 0xFF;
  uint32_t c0 = (pixel >> 0) & 0xFF;
  if (numComps == 4) {
    c3 = f(c3);
  }
  c2 = f(c2);
  c1 = f(c1);
  c0 = f(c0);
  if (numComps == 4) {
    outPixel |= (c3 & 0xFF) << 24;
  }
  outPixel |= (c2 & 0xFF) << 16;
  outPixel |= (c1 & 0xFF) << 8;
  outPixel |= (c0 & 0xFF) << 0;
  return outPixel;
}

// Given 2 pixels, generate a simple average prediction based on
// the 2 pixel values. This logic will generate sum for each component
// and then divide by 2 to get the average.

static inline
int ComponentAverage(const uint32_t * const pixelsPtr, int o1, int o2) {
  const bool debug = false;
  
  if (debug) {
    printf("ComponentAverage %d %d\n", o1, o2);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
#endif // DEBUG
  
  // Lookup color table offset at o1 and o2
  
  uint32_t p1 = pixelsPtr[o1];
  uint32_t p2 = pixelsPtr[o2];
  
  if (debug) {
    printf("pixel offset %2d -> 0x%08X\n", o1, p1);
    printf("pixel offset %2d -> 0x%08X\n", o2, p2);
  }
  
  if (p1 == p2) {
    if (debug) {
      printf("comp delta is ZERO special case for 2x duplicate pixel 0x%08X\n", p1);
    }

    return p1;
  }
    
  // Component delta is a simple SUB each component from p1 -> p2
  
  if (debug) {
    printf("predict(%2d,%2d) : 0x%08X -> 0x%08X\n", o1, o2, p1, p2);
  }
  
  uint32_t deltaPixel = pixel_component_delta(p1, p2, 3);
  
  if (debug) {
    printf("comp delta     : 0x%08X\n", deltaPixel);
  }
  
  // Component scale by 1/2
  
  // FIXME: this impl is not optimal since it has to convert then
  // do math op, then convert back. Better to just sum each
  // original component as positive numbers and then do shifting
  // to divide and get the prediction pixel.
  
  auto halfFunc = [](uint32_t val)->uint32_t {
    if (debug) {
      printf("un byte : %d\n", (int)val);
    }
    int sVal = unsigned_byte_to_signed(val);
    if (debug) {
      printf("in      : %d\n", (int)sVal);
    }
    sVal = sVal / 2;
    if (debug) {
      printf("out     : %d\n", (int)sVal);
    }
    return signed_byte_to_unsigned(sVal);
  };
  
  // Divide each delta in half to generate an average delta
  
  uint32_t halfDeltaPixel = foreach_pixel_component(deltaPixel, halfFunc, 3);
  
  if (debug) {
    printf("1/2 comp delta : 0x%08X\n", halfDeltaPixel);
  }
  
  // Return the original pixel plus the ave delta

  uint32_t predPixel = pixel_component_sum(p1, halfDeltaPixel, 3);
  
  return predPixel;
}

// Predict pixels with 2 neighbors

static inline
int CTIPredict2(const uint32_t * const pixelsPtr, int o1, int o2) {
  const bool debug = false;
  
  if (debug) {
    printf("CTIPredict2 %d %d\n", o1, o2);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
#endif // DEBUG
  
  // Lookup color table offset at o1 and o2
  
  uint32_t p1 = pixelsPtr[o1];
  uint32_t p2 = pixelsPtr[o2];
  
  if (debug) {
    printf("image offset %2d -> 0x%08X\n", o1, p1);
    printf("image offset %2d -> 0x%08X\n", o2, p2);
  }
  
  if (p1 == p2) {
    if (debug) {
      printf("comp delta is ZERO special case for 2x duplicate pixel 0x%08X\n", p1);
    }
    
    return 0;
  }
  
  if (debug) {
    printf("image offset %2d -> 0x%08X\n", o1, p1);
    printf("image offset %2d -> 0x%08X\n", o2, p2);
  }
  
  // Component delta is a simple SUB each component from p1 -> p2
  
  if (debug) {
    printf("predict(%2d,%2d) : 0x%08X -> 0x%08X\n", o1, o2, p1, p2);
  }
  
  uint32_t deltaPixel = pixel_component_delta(p1, p2, 3);
  
  if (debug) {
    printf("comp delta     : 0x%08X\n", deltaPixel);
  }
  
//  const unsigned int weight = 8;
  
  unsigned int sum = sum_of_abs_components(deltaPixel, 3);
  
//  return sum * weight;
  
  return sum;
}

// Predict with 3 neighbors

static inline
int CTIPredict3(const uint32_t * const pixelsPtr, int o1, int o2, int o3) {
  const bool debug = false;
  
  if (debug) {
    printf("CTIPredict3 %d %d %d\n", o1, o2, o3);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
  assert(o3 != -1);
#endif // DEBUG
  
  uint32_t p1 = pixelsPtr[o1];
  uint32_t p2 = pixelsPtr[o2];
  uint32_t p3 = pixelsPtr[o3];
  
  if (debug) {
    printf("table offset %4d -> 0x%08X\n", o1, p1);
    printf("table offset %4d -> 0x%08X\n", o2, p2);
    printf("table offset %4d -> 0x%08X\n", o3, p3);
  }
  
  if (p1 == p2 && p1 == p3) {
    if (debug) {
      printf("comp delta is ZERO special case for 3x duplicate table index %d\n", o1);
    }
    
    return 0;
  }
  
  // Component delta from p1 -> p2 (SUB)
  
  if (debug) {
    printf("predict(%2d,%2d) : 0x%08X -> 0x%08X\n", o1, o2, p1, p2);
  }
  
  uint32_t delta1 = pixel_component_delta(p1, p2, 3);
  
  if (debug) {
    printf("comp delta1     : 0x%08X\n", delta1);
  }
  
  // Calcualte second delta from p2 to p3
  
  if (debug) {
    printf("predict(%2d,%2d) : 0x%08X -> 0x%08X\n", o2, o3, p2, p3);
  }
  
  uint32_t delta2 = pixel_component_delta(p2, p3, 3);
  
  if (debug) {
    printf("comp delta2     : 0x%08X\n", delta2);
  }
  
  // Convert an unsigned byte value in the range (0, 255)
  // to a signed int value in the range (-128, 0, 127)
  
  const unsigned int weight1 = 6;
  const unsigned int weight2 = 2;
  
  unsigned int sum1 = sum_of_abs_components(delta1, 3);
  unsigned int sum2 = sum_of_abs_components(delta2, 3);

  if (debug) {
    printf("sum1 %d\n", sum1);
    printf("sum2 %d\n", sum2);
  }
  
  unsigned int sum = (sum1 * weight1) + (2 * sum2 * weight2);

  if (debug) {
    printf("unscaled sum %d\n", sum);
  }
  
  sum >>= 3;

  if (debug) {
    printf("sum %d\n", sum);
  }
  
  return sum;
}

// Predict can do a linear prediction including neighbors in 8 way around
// as long as neighbor is defined.

// FIXME: prediction should consider neighbors if they are defined.

static inline
int CTIPredict(const uint32_t * const pixelsPtr, int o1, int o2, int o3) {
  if (o3 == -1) {
    return CTIPredict2(pixelsPtr, o1, o2);
  } else {
    return CTIPredict3(pixelsPtr, o1, o2, o3);
  }
}

// Simple grayscale delta of 2 grayscale values : abs(dV)

static inline
int CTIGrayDelta(const uint8_t * const grayPtr, int o1, int o2) {
  const bool debug = false;
  
  if (debug) {
    printf("CTIGrayDelta %d %d\n", o1, o2);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
#endif // DEBUG
  
  // Lookup color table offset at o1 and o2
  
  int p1 = grayPtr[o1];
  int p2 = grayPtr[o2];
  
  if (debug) {
    printf("image offset %2d -> %3d\n", o1, p1);
    printf("image offset %2d -> %3d\n", o2, p2);
  }
  
  if (p1 == p2) {
    if (debug) {
      printf("comp delta is ZERO special case for 2x duplicate pixel 0x%08X\n", p1);
    }
    
    return 0;
  }
  
  if (debug) {
    printf("image offset %2d -> 0x%08X\n", o1, p1);
    printf("image offset %2d -> 0x%08X\n", o2, p2);
  }
  
//  if (debug) {
//    printf("predict(%2d,%2d) : 0x%08X -> 0x%08X\n", o1, o2, p1, p2);
//  }

  //int delta = abs(p2 - p1);
  int delta = (p2 - p1);
  
  if (debug) {
    printf("delta = %d : %d - %d\n", delta, p2, p1);
  }
  
  // Convert signed values (-255, 255) to unsigned range (0, 512)
  // where (0 -> 0, 1 -> 2, -1 -> 2 and so on.
  // Note that convertSignedZeroDeltaToUnsignedNbits checking is not needed
  // since -256 is not a valid value.
  
  //uint32_t unDelta = convertSignedZeroDeltaToUnsignedNbits(delta, 9);
  
  uint32_t unDelta = convertSignedZeroDeltaToUnsigned(delta);
  
  if (debug) {
    printf("unsigned delta = %d\n", unDelta);
  }
  
  return unDelta;
}

// Table prediciton with 2 values along same axis

static inline
int CTITablePredict2(const uint8_t * const tableOffsetsPtr, const uint32_t * const colortablePixelsPtr, int o1, int o2, const int N) {
  const bool debug = false;
  
  if (debug) {
    printf("CTITablePredict2 %d %d : N = %d\n", o1, o2, N);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
#endif // DEBUG
  
  // o1 = "from"
  // o2 = "to"
  
  // Lookup color table offset at o1 and o2
  
  int ctableO1 = tableOffsetsPtr[o1];
  int ctableO2 = tableOffsetsPtr[o2];
  
  if (ctableO1 == ctableO2) {
    if (debug) {
      printf("comp delta is ZERO special case for 2 duplicate table offsets %d : at %d and %d\n", ctableO1, o1, o2);
    }
    
    return 0;
  }
  
#if defined(DEBUG)
  if (colortablePixelsPtr) {
    uint32_t p1 = colortablePixelsPtr[ctableO1];
    uint32_t p2 = colortablePixelsPtr[ctableO2];
    
    if (debug) {
      printf("table offset %2d -> 0x%08X\n", ctableO1, p1);
      printf("table offset %2d -> 0x%08X\n", ctableO2, p2);
    }
  }
#endif // DEBUG
  
  int plainDelta = ctableO2 - ctableO1;
  
  if (debug) {
    printf("plain axis delta     : %d\n", plainDelta);
  }
  
  int wrappedDelta = convertToWrappedTableDelta(ctableO1, ctableO2, N);
  unsigned int unsignedDelta = convertSignedZeroDeltaToUnsigned(wrappedDelta);
  
  if (debug) {
    printf("wrapped  table delta     : %d\n", wrappedDelta);
    printf("unsigned table delta     : %d\n", unsignedDelta);
  }
  
  //const int weight1 = 6;
  //const int weight2 = 2;
  //const unsigned int denom = 8;
  //unsigned int sum = (unsignedDelta * denom);
  
  unsigned int sum = unsignedDelta;
  
  if (debug) {
    printf("unweighted sum = (8 * %d) = %d\n", unsignedDelta, sum);
  }
  
  return sum;
}

static inline
int CTITablePredict3(const uint8_t * const tableOffsetsPtr, const uint32_t * const colortablePixelsPtr, int o1, int o2, int o3, const int N) {
  const bool debug = false;
  
  if (debug) {
    printf("CTITablePredict3 %d %d %d : N = %d\n", o1, o2, o3, N);
  }
  
#if defined(DEBUG)
  assert(o1 != o2);
  assert(o3 != -1);
#endif // DEBUG
  
  // o1 = "from"
  // o2 = "to"
  // o3 = "other"
  
  // Lookup color table offset at o1 and o2
  
  int ctableO1 = tableOffsetsPtr[o1];
  int ctableO2 = tableOffsetsPtr[o2];
  int ctableO3 = tableOffsetsPtr[o3];
  
  if ((ctableO1 == ctableO2) && (ctableO2 == ctableO3)) {
    if (debug) {
      printf("comp delta is ZERO special case for duplicate table offsets %d : at %d and %d\n", ctableO1, o1, o2);
    }
    
    return 0;
  }
  
#if defined(DEBUG)
  if (colortablePixelsPtr) {
    uint32_t p1 = colortablePixelsPtr[ctableO1];
    uint32_t p2 = colortablePixelsPtr[ctableO2];
    uint32_t p3 = colortablePixelsPtr[ctableO3];
    
    if (debug) {
      printf("table offset %2d -> 0x%08X\n", ctableO1, p1);
      printf("table offset %2d -> 0x%08X\n", ctableO2, p2);
      printf("table offset %2d -> 0x%08X\n", ctableO3, p3);
    }
  }
#endif // DEBUG
  
  int plainDeltaH = ctableO2 - ctableO1;
  int plainDeltaV = ctableO3 - ctableO2;
  
  if (debug) {
    printf("plain horizontal delta     : %d\n", plainDeltaH);
    printf("plain vertical   delta     : %d\n", plainDeltaV);
  }
  
  int wrappedHDelta = convertToWrappedTableDelta(ctableO1, ctableO2, N);
  unsigned int unsignedHDelta = convertSignedZeroDeltaToUnsigned(wrappedHDelta);
  
  int wrappedVDelta = convertToWrappedTableDelta(ctableO2, ctableO3, N);
  unsigned int unsignedVDelta = convertSignedZeroDeltaToUnsigned(wrappedVDelta);
  
  if (debug) {
    printf("wrapped  H table delta     : %d\n", wrappedHDelta);
    printf("wrapped  V table delta     : %d\n", wrappedVDelta);
    
    printf("unsigned H table delta     : %d\n", unsignedHDelta);
    printf("unsigned V table delta     : %d\n", unsignedVDelta);
  }
  
  const unsigned int weight1 = 6;
  const unsigned int weight2 = 2;
  
#if defined(DEBUG)
  const unsigned int denom = 8;
  
  // (H * 6/8) + (2 * V * 2/8) = weighted average
  //
  // (((H * 6) + (V * 2)) / 8) will round up and shift right by 3 is fast
  
  int ave = ((unsignedHDelta * weight1) + (2 * unsignedVDelta * weight2)) >> 3;
  
  if (debug) {
    float fAve = ((unsignedHDelta * weight1) + (unsignedVDelta * weight2)) / (1.0f * denom);
    
    printf("weighted ave = ((%d * %d) + (2 * %d * %d)) / %d = %.2f = %d\n", unsignedHDelta, weight1, unsignedVDelta, weight2, denom, fAve, ave);
  }
#endif // DEBUG
  
  // Weighted sum
  
  // (H * 0.8) + (2 * V * 0.2)
  
  // Simple weighted sum without turning it into an average
  
  unsigned int sum = ((unsignedHDelta * weight1) + (2 * unsignedVDelta * weight2));
  
  if (debug) {
    printf("weighted sum = ((%d * %d) + (2 * %d * %d)) = %d\n", unsignedHDelta, weight1, unsignedVDelta, weight2, sum);
  }
  
  // Scale sum back own to original range
  
  sum >>= 3; // N / 8

  if (debug) {
    printf("weighted sum = %d\n", sum);
  }
  
  return sum;
}

// Entry point for a table based prediction for a primary axis and an optional other value along the other axis.

static inline
int CTITablePredict(const uint8_t * const tableOffsetsPtr, const uint32_t * const colortablePixelsPtr, int o1, int o2, int o3, const int N) {
  const bool debug = false;
  
  if (debug) {
    printf("CTITablePredict  %d %d %d : N = %d\n", o1, o2, o3, N);
  }
  
  if (o3 == -1) {
    return CTITablePredict2(tableOffsetsPtr, colortablePixelsPtr, o1, o2, N);
  } else {
    return CTITablePredict3(tableOffsetsPtr, colortablePixelsPtr, o1, o2, o3, N);
  }
}

// Multiply by 341 using only addition

static inline
unsigned int fast_mult_341(unsigned int N) {
  const bool debug = false;
  
  if (debug) {
    printf("fast_mult_341 N = %d\n", N);
  }
  
  // N * 341
  // (N * 256) + (N * 64) + (N * 16) + (N * 4) + (N * 1)
  
//  unsigned int sum = N + N;
//  unsigned int sum = N;
  unsigned int sum;

  if ((1)) {
    sum = N * 341;
  } else if ((0)) {
    sum = N;
    sum += (N * 4);
    sum += (N * 16);
    sum += (N * 64);
    sum += (N * 256);
  } else {
    // left shift to multiply
    // mult 2 << 1
    // mult 4 << 2
    // mult 8 << 3
    // mult 16 << 4
    // mult 32 << 5
    // mult 64 << 6
    // mult 128 << 7
    // mult 256 << 8

    sum = N; // (N * 4)
    sum += (N << 2); // (N * 4)
    sum += (N << 4); // (N * 16)
    sum += (N << 6); // (N * 64)
    sum += (N << 8); // (N * 256)
  }
  
  return sum;
}

// Fast divide by 2 using unsigned shift right

static inline
unsigned int fast_div_2(unsigned int N) {
  unsigned int div2 = (N >> 1);
#if defined(DEBUG)
  assert(div2 == (N / 2));
#endif // DEBUG
  return div2;
}

// Fast divide by 4 using unsigned shift right

static inline
unsigned int fast_div_4(unsigned int N) {
  unsigned int div4 = (N >> 2);
#if defined(DEBUG)
  assert(div4 == (N / 4));
#endif // DEBUG
  return div4;
}

// Divide an unsigned int in the range (0, 1023) 2^10 by 3 with chop on the result

static inline
unsigned int fast_div_3(unsigned int N) {
  const bool debug = false;
  
  if (debug) {
    printf("fast_div_3 N = %d\n", N);
  }

//  unsigned int div3 = ((N+1) * 341) / 1024;
  
//  unsigned int div3 = fast_mult_341(N+1) / 1024;
  
  unsigned int div3 = (fast_mult_341(N+1) >> 10);
  
//  unsigned int div3 = ((N+1) * 341) >> 10;
  
  if (debug) {
    printf("div3 = %d\n", div3);
    printf("N / 3 = %d\n", (N / 3));
    printf("N / 3.0 = %0.3f\n", (N / 3.0f));
  }
  
//#if defined(DEBUG)
//  assert(div3 == (N / 3));
//#endif // DEBUG
  
  return div3;
}

// Fast average of 2 unsigned int values via right shift

static inline
unsigned int fast_ave_2(unsigned int v1, unsigned int v2) {
  const bool debug = false;
  if (debug) {
    printf("fast_ave_2(%d,%d)\n", v1, v2);
  }
  
  // AVE = (V1 + V2) / 2
  unsigned int ave = (v1 + v2) >> 1;
  return ave;
}

// Calcualte average of 0,1,2 values. When both v1 and v2
// are both -1 then -1 is returned.

// FIXME: on 64 bit system, can comparing of 2 32 bit numbers to
// a known constant be implemented as a single compare to -1?
// uint64_t c1 = (v1 << 32) | v2;
// if (c1 == -1)

static inline
int average_012(int v1, int v2) {
  int ave;
  if (v1 == -1 && v2 == -1) {
    // No pixels in rows -1 or +1
    ave = -1;
  } else if (v1 != -1 && v2 != -1) {
    // Both rows contain pixels, ave() of 2
    ave = fast_ave_2(v1, v2);
  } else if (v1 != -1) {
    ave = v1;
  } else if (v2 != -1) {
    ave = v2;
  } else {
#if defined(DEBUG)
    assert(0);
#endif // DEBUG
  }
  return ave;
}

// Average 1,2,3 values where v1 is known to always be positive

static inline
int average_123(int v1, int v2, int v3) {
#if defined(DEBUG)
  assert(v1 != -1);
#endif // DEBUG
  
  int ave;
  if (v2 == -1 && v3 == -1) {
    ave = v1;
  } else if (v2 != -1 && v3 != -1) {
    // Both v2 and v3 are defined, ave() of 3 values
    unsigned int sum = v1 + v2 + v3;
    ave = fast_div_3(sum);
  } else if (v2 != -1) {
    // ave2(v1, v2)
    ave = fast_ave_2(v1, v2);
  } else if (v3 != -1) {
    // ave2(v1, v3)
    ave = fast_ave_2(v1, v3);
  } else {
#if defined(DEBUG)
    assert(0);
#endif // DEBUG
  }
  return ave;
}

static inline
void superFastBlur(unsigned char *pix, int w, int h, int radius)
{
  if (radius<1) {
    return;
  }
  int wm=w-1;
  int hm=h-1;
  int wh=w*h;
  int div=radius+radius+1;
  unsigned char *r=new unsigned char[wh];
  unsigned char *g=new unsigned char[wh];
  unsigned char *b=new unsigned char[wh];
  int rsum,gsum,bsum,x,y,i,p,p1,p2,yp,yi,yw;
  int *vMIN = new int[std::max(w,h)];
  int *vMAX = new int[std::max(w,h)];
  
  unsigned char *dv = new unsigned char[256*div];
  for ( i = 0; i < 256 * div; i++) {
    dv[i] = (i/div);
  }
  
  yw = yi = 0;
  
  for ( y = 0; y < h; y++ ) {
    rsum = gsum = bsum = 0;
    for ( i = -radius; i <= radius; i++ ){
      p = (yi + std::min(wm, std::max(i,0))) * 3;
      rsum += pix[p];
      gsum += pix[p+1];
      bsum += pix[p+2];
    }
    for ( x = 0; x < w; x++) {
      r[yi]=dv[rsum];
      g[yi]=dv[gsum];
      b[yi]=dv[bsum];
      
      if (y == 0){
        vMIN[x]=std::min(x+radius+1,wm);
        vMAX[x]=std::max(x-radius,0);
      }
      p1 = (yw+vMIN[x])*3;
      p2 = (yw+vMAX[x])*3;
      
      rsum += pix[p1] - pix[p2];
      gsum += pix[p1+1] - pix[p2+1];
      bsum += pix[p1+2] - pix[p2+2];
      
      yi++;
    }
    yw+=w;
  }
  
  for ( x = 0; x < w; x++ ) {
    rsum = gsum = bsum = 0;
    yp =- radius * w;
    for( i = -radius; i <= radius; i++){
      yi=std::max(0,yp)+x;
      rsum+=r[yi];
      gsum+=g[yi];
      bsum+=b[yi];
      yp+=w;
    }
    yi=x;
    for ( y = 0; y < h; y++ ){
      pix[yi*3] = dv[rsum];
      pix[yi*3 + 1] = dv[gsum];
      pix[yi*3 + 2] = dv[bsum];
      if ( x == 0 ){
        vMIN[y] = std::min(y+radius+1,hm)*w;
        vMAX[y] = std::max(y-radius,0)*w;
      }
      p1=x+vMIN[y];
      p2=x+vMAX[y];
      
      rsum+=r[p1]-r[p2];
      gsum+=g[p1]-g[p2];
      bsum+=b[p1]-b[p2];
      
      yi+=w;
    }
  }
  
  delete [] r;
  delete [] g;
  delete [] b;
  
  delete [] vMIN;
  delete [] vMAX;
  delete [] dv;
}

// gradclamp predictor (MED)

static inline
uint32_t min3ui(uint32_t v1, uint32_t v2, uint32_t v3) {
  uint32_t min = std::min(v1, v2);
  return std::min(min, v3);
}

static inline
uint32_t max3ui(uint32_t v1, uint32_t v2, uint32_t v3) {
  uint32_t max = std::max(v1, v2);
  return std::max(max, v3);
}

// Clamp an integer value to a min and max range.
// This method operates only on signed integer
// values.

static inline
int32_t clampi(int32_t val, int32_t min, int32_t max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  } else {
    return val;
  }
}

// ClampedGradPredictor described here:
// http://cbloomrants.blogspot.com/2010/06/06-20-10-filters-for-png-alike.html
//
// This inlined predictor function operates on 1 component
// of a 4 component word. Note that this method operates
// in terms of 8, 16, or 32 bit integers.

static
inline
uint32_t gradclamp_predict(uint32_t a, uint32_t b, uint32_t c) {
  const bool debug = false;
  
  if (debug) {
    printf("a=%d (left), b=%d (up), c=%d (upleft)\n", a, b, c);
  }
  
  // The next couple of calculations make use of signed integer
  // math, though the returned result will never be smaller
  // than the min or larger than the max, so there should
  // not be an issue with this method returning values
  // outside the byte range if the original input is in the
  // byte range.
  
  int p = a + b - c;
  
  if (debug) {
    printf("p = (a + b - c) = (%d + %d - %d) = %d\n", a, b, c, p);
  }
  
  // Find the min and max value of the 3 input values
  
  uint32_t min = min3ui(a, b, c);
  uint32_t max = max3ui(a, b, c);
  
  // FIXME: currently, there appears to be an issue if the
  // values being predicted are as large as the largest integer,
  // but this is ignored for now since only bytes will be predicted.
  
  // Note that the compare here is done in terms of a signed
  // integer but that the min and max are known to be in terms
  // of an unsigned int, so the result will never be less than
  // min or larger than max.
  
  int32_t clamped = clampi(p, (int32_t)min, (int32_t)max);
  
#ifdef DEBUG
  assert(clamped >= 0);
  assert(clamped >= min);
  assert(clamped <= max);
#endif // DEBUG
  
  if (debug) {
    printf("p = %d\n", p);
    printf("(min, max) = (%d, %d)\n", min, max);
    printf("clamped = %d\n", clamped);
  }
  
  return (uint32_t) clamped;
}

// This gradclamp predictor logic operates in terms of bytes
// and not whole pixels. But for the sake of efficient code,
// the logic executes in terms of blocks of 4 byte elements at
// a time read from samplesPtr. This approach makes it efficient
// to predict RGB or RGBA pixels packed into a word (a pixel).
// The caller must make sure to supply values 4 at a time and
// deal with the results if fewer than 4 are needed at the
// end of the buffer.
//
// The width argument indicates the width of the buffer being
// predicted. The offset argument indicates the ith offset
// in samplesPtr to read from.

static inline
uint32_t gradclamp8by4(uint32_t *samplesPtr, uint32_t width, uint32_t offset) {
  const bool debug = false;
  
  // indexes must be signed so that they can be negative at top or left edge
  
  int upOffset = offset - width;
  int upLeftOffset = upOffset - 1;
  int leftOffset = offset - 1;
  
  if (debug) {
    printf("upi=%d, upLefti=%d, lefti=%d\n", upOffset, upLeftOffset, leftOffset);
  }
  
  uint32_t upSamples;
  uint32_t upLeftSamples;
  uint32_t leftSamples;
  
  // left = a
  
  if (leftOffset < 0) {
    leftSamples = 0x0;
  } else {
    leftSamples = samplesPtr[leftOffset];
  }
  
  // up = b
  
  if (upOffset < 0) {
    upSamples = 0x0;
  } else {
    upSamples = samplesPtr[upOffset];
  }
  
  // upLeft = c
  
  if (upLeftOffset < 0) {
    upLeftSamples = 0x0;
  } else {
    upLeftSamples = samplesPtr[upLeftOffset];
  }
  
  // Execute predictor for each of the 4 components
  
  uint32_t c3 = gradclamp_predict(
                                  RSHIFT_MASK(leftSamples, 24, 0xFF),
                                  RSHIFT_MASK(upSamples, 24, 0xFF),
                                  RSHIFT_MASK(upLeftSamples, 24, 0xFF)
                                  );
  
  uint32_t c2 = gradclamp_predict(
                                  RSHIFT_MASK(leftSamples, 16, 0xFF),
                                  RSHIFT_MASK(upSamples, 16, 0xFF),
                                  RSHIFT_MASK(upLeftSamples, 16, 0xFF)
                                  );
  
  uint32_t c1 = gradclamp_predict(
                                  RSHIFT_MASK(leftSamples, 8, 0xFF),
                                  RSHIFT_MASK(upSamples, 8, 0xFF),
                                  RSHIFT_MASK(upLeftSamples, 8, 0xFF)
                                  );
  uint32_t c0 = gradclamp_predict(
                                  RSHIFT_MASK(leftSamples, 0, 0xFF),
                                  RSHIFT_MASK(upSamples, 0, 0xFF),
                                  RSHIFT_MASK(upLeftSamples, 0, 0xFF)
                                  );
  
  if (debug) {
    printf("c3=%d, c2=%d, c1=%d, c0=%d\n", c3, c2, c1, c0);
  }
  
  // It should not be possible for values larger than a byte range
  // to be returned, since gradclamp_predict() returns one of the 3 passed
  // in values and they are all masked to 0xFF.
  
#if defined(DEBUG)
  assert(c3 <= 0xFF);
  assert(c2 <= 0xFF);
  assert(c1 <= 0xFF);
  assert(c0 <= 0xFF);
#endif // DEBUG
  
  // Mask to 8bit value and shift into component position
  
  uint32_t components =
  MASK_LSHIFT(c3, 0xFF, 24) |
  MASK_LSHIFT(c2, 0xFF, 16) |
  MASK_LSHIFT(c1, 0xFF, 8) |
  MASK_LSHIFT(c0, 0xFF, 0);
  
  return components;
}

// This logic will encode a gradclamp prediction error in terms
// of an 8bit integer value. This calculation must be done
// in terms of a signed 8 bit value which is then converted
// back to an unsigned integer value and returned.

static
inline
uint32_t gradclamp8_encode_predict_error(uint8_t pred, uint8_t sample) {
  const bool debug = false;
  
  if (debug) {
    printf("pred=%d, sample=%d\n", pred, sample);
  }
  
  uint8_t resultByte = sample - pred;
  uint32_t pred_err = ((uint32_t)resultByte) & 0xFF;
  
  if (debug) {
    printf("gradclamp_encode_predict_error : sample %d : prediction %d : prediction_err %d\n", sample, pred, pred_err);
  }
  
  return pred_err;
}

// Given a buffer of input pixel values, do gradclamp prediction and then encode
// the prediction error for each component. While components are stored
// in blocks of 4 in each word, care is taken in this implementation to
// ensure that each value is properly dealt with as an unsigned 8bit value.

static inline
void gradclamp8by4_encode_pred_error(uint32_t *inSamplesPtr,
                                     uint32_t *outPredErrPtr,
                                     uint32_t startSampleIndex,
                                     uint32_t endSampleIndex,
                                     uint32_t width)
{
  const bool debug = false;
  
  if (debug) {
    printf("gradclamp8by4_encode_pred_error() startSampleIndex=%d, endSampleIndex=%d, width=%d\n", startSampleIndex, endSampleIndex, width);
  }
  
  for (int i = startSampleIndex; i < endSampleIndex; i++) {
    uint32_t pred = gradclamp8by4(inSamplesPtr, width, i);
    uint32_t inSamples = inSamplesPtr[i];
    
    uint8_t c3_sample = RSHIFT_MASK(inSamples, 24, 0xFF);
    uint8_t c3_pred = RSHIFT_MASK(pred, 24, 0xFF);
    uint32_t c3_pred_err = gradclamp8_encode_predict_error(c3_pred, c3_sample);
    
    uint8_t c2_sample = RSHIFT_MASK(inSamples, 16, 0xFF);
    uint8_t c2_pred = RSHIFT_MASK(pred, 16, 0xFF);
    uint32_t c2_pred_err = gradclamp8_encode_predict_error(c2_pred, c2_sample);
    
    uint8_t c1_sample = RSHIFT_MASK(inSamples, 8, 0xFF);
    uint8_t c1_pred = RSHIFT_MASK(pred, 8, 0xFF);
    uint32_t c1_pred_err = gradclamp8_encode_predict_error(c1_pred, c1_sample);
    
    uint8_t c0_sample = RSHIFT_MASK(inSamples, 0, 0xFF);
    uint8_t c0_pred = RSHIFT_MASK(pred, 0, 0xFF);
    uint32_t c0_pred_err = gradclamp8_encode_predict_error(c0_pred, c0_sample);
    
    uint32_t outPredErr = (c3_pred_err << 24) | (c2_pred_err << 16) | (c1_pred_err << 8) | (c0_pred_err);
    
    outPredErrPtr[i] = outPredErr;
  }
  
  return;
}
