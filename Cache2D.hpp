//
//  Cache2D.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  A 2D cache represents (X,Y) coordinates in a matrix with a
//  given width and height. This cache logic provides a useful
//  abstraction to store a templated value T for each (X,Y)
//  coordinate in the grid. A cache is either in the horizontal
//  or vertical orientation so that each array access for
//  increasing values like ((0,0) (1,0) (2,0) (3,0)) will access
//  memory in linear order as offsets (0 1 2 3).

#ifndef CACHE_2D_H
#define CACHE_2D_H

#include "assert.h"

#include <vector>
#include <list>
#include <queue>
#include <set>

#include <unordered_map>

#if defined(DEBUG)
#include <iostream>
#endif // DEBUG

#include <sstream>

#include "PredFuncs.hpp"

using namespace std;

template <class T, const bool isHorizontal>
class Cache2D {
public:
  int width;
  int height;
  vector<T> values;

  Cache2D()
  {
  }

  void allocValues(int inWidth, int inHeight, T defaultValue)
  {
    width = inWidth;
    height = inHeight;
    
    const int N = width * height;
    
    values.reserve(N);
    
    for (int i = 0; i < N; i++) {
      values.push_back(defaultValue); // copy of default object state
    }
  }
  
  // DEBUG method used to verify the bounds of an offset
  
  void assertIfInvalidOffset(int offset) const {
    assert(offset >= 0);
    assert(offset < values.size());
  }
  
  // Calculate the cached offset given an (X,Y) coordinate
  // in the grid. In a horizontal grid, the cached offset
  // is ((y * width) + x) while the offset is transposed
  // when in vertical mode.
  
  int cachedOffset(int x, int y) const {
    if (isHorizontal) {
      return (y * width) + x;
    } else {
      return (x * height) + y;
    }
  }
  
  // Clamp a value like (x + n) to the width in horizontal mode
  // or (y + n) to the height in vertical mode. For example,
  // with width = 3 a call to clampMax(3) will return 2
  // since the offset (width - 1) is the largest value offset.

  int clampMax(int colOrRowRel) const {
    int colOrRowMax;
    
    if (isHorizontal) {
      colOrRowMax = width - 1;
    } else {
      colOrRowMax = height - 1;
    }
    
    if (colOrRowRel > colOrRowMax) {
      return colOrRowMax;
    } else {
      return colOrRowRel;
    }
  }
  
#if defined(DEBUG)
  
  // Validate that the sum is in the valid range of (0,1023) (10 bits)
  
  void checkMaxValue(unsigned int sum) {
    if (sum > 1023) {
      assert(0);
    }
  }
  
#endif // DEBUG
  
};

// A 2D cache where each (X,Y) cache value stores an average sum
// of the previous 3 values. Note that the previous values applies
// to a given row, so that at position 0 the cache contains only
// 1 value, at position 1 the cache contains the sum of 2 values,
// and positions 2 -> (width-1) contain the sum of 3 values.

#define Cache2DSum3_Invalid -1
#define Cache2DSum3_ZeroRows -2

template <class T, const bool isHorizontal>
class Cache2DSum3 : public Cache2D<T, isHorizontal> {
public:
  
  Cache2DSum3()
  {
  }
  
  // Invalidate either a H or V cache for the indicate pixel.
  // Note that in the case of a vertical cache the pixel coordinates
  // are transposed.
  
  void invalidate(int x, int y)
  {
    const bool debug = false;
    
    if (debug) {
      printf("Cache2SumD3.invalidate (%d,%d) : isHorizontal %d\n", x, y, isHorizontal);
    }
    
    int offset = this->cachedOffset(x,y);

    int delta = 2; // +2 inclusive covers 3 values
    
    int colOrRow = isHorizontal ? x : y;
    
    int colOrRowPlusDelta = colOrRow + delta;
    
    int colOrRowMax = this->clampMax(colOrRowPlusDelta);
    
    if (debug) {
      printf("colOrRow + delta = %d + %d = %d : colOrRowMax %d : max %d\n", colOrRow, delta, colOrRowPlusDelta, colOrRowMax, isHorizontal ? this->width : this->height);
    }
    
    if (colOrRowPlusDelta > colOrRowMax) {
      delta = colOrRowMax - colOrRow;
      
#if defined(DEBUG)
      assert(delta >= 0);
#endif // DEBUG
      
      if (debug) {
        printf("delta adjusted down to %d : colOrRowMax %d\n", delta, colOrRowMax);
      }
    }
    
#if defined(DEBUG)
    if (isHorizontal) {
      assert((x + delta) < this->width);
    } else {
      assert((y + delta) < this->height);
    }
#endif // DEBUG
    
    int offsetMax = offset + delta;
    
    vector<T> & cachedDeltaRows = this->values;
    
#if defined(DEBUG)
    int i = 0;
#endif // DEBUG
    
    if (debug) {
      printf("invalidateRowCache iter offsets (%d, %d) (inclusive)\n", offset, offsetMax);
    }
    
    if (colOrRow > 0) {
      offset -= 1;
    }
    
    for ( ; offset <= offsetMax; offset++ ) {
#if defined(DEBUG)
      int cacheCol = x;
      int cacheRow = y;
      
      int nextColOrRow;
      if (isHorizontal) {
        nextColOrRow = cacheCol + i;
      } else {
        nextColOrRow = cacheRow + i;
      }
      
      i += 1;
      
      if (debug) {
        int v1 = nextColOrRow;
        int v2 = cacheRow;
        if (!isHorizontal) {
          v1 = cacheCol;
          v2 = nextColOrRow;
        }
        
        printf("invalidate %s (%d,%d) : offset %d\n", isHorizontal ? (char*)"H" : (char*)"V", v1, v2, offset);
      }
#endif // DEBUG
      
      cachedDeltaRows[offset] = -1;
    }
    
    if (debug) {
      printf("Cache2SumD3.invalidate returning\n");
    }
    
    return;
  }
  
  // Query the current cached value for a given (X,Y) slot
  
  T getCachedValue(const Cache2D<T,isHorizontal> & cachedL1, int x, int y)
  {
    const bool debug = false;
    
    const int cacheCol = x;
    const int cacheRow = y;
    
    if (debug) {
      printf("Cache2SumD3.getCachedValue (%d,%d) : isHorizontal %d\n", cacheCol, cacheRow, isHorizontal);
    }
    
    // If the row3 cache is valid, return, this may not be needed later in the case
    // where the lookup function does this checking in a more optimal way.
    
    const int offset = this->cachedOffset(cacheCol, cacheRow);
    
    vector<T> & cachedDeltaRows = this->values;
    
    T val = cachedDeltaRows[offset];
    
    if (val == Cache2DSum3_Invalid) {
      if (debug) {
        printf("cache is invalid at (%d,%d) offset %d : isHorizontal %d\n", cacheCol, cacheRow, offset, isHorizontal);
      }
      
      // update()
      
      {
        // If the row3 cache is valid, return, this may not be needed later in the case
        // where the lookup function does this checking in a more optimal way.
        
        const int endOffset = offset;
        
        // sum the previous 3 deltas read from delta cache
        
        unsigned int sum = 0;
        unsigned int N = 0;
        
        // FIXME: looking back should not skip over rows?
        // FIXME: should this use transposed col in V case?
        
        int off = -2;
        int relColOrRow;
        
        if (isHorizontal) {
          relColOrRow = cacheCol + off;
        } else {
          relColOrRow = cacheRow + off;
        }
        
        if (relColOrRow < 0) {
          off -= relColOrRow;
        }
        
        int startOffset;
        
        startOffset = endOffset + off;
        
        if (debug) {
          if (isHorizontal) {
            int startCol = cacheCol + off;
            printf("updateRowCache iterate H cols (%d,%d) inclusive\n", startCol, cacheCol);
          } else {
            int startRow = cacheRow + off;
            printf("updateRowCache iterate H cols (%d,%d) inclusive\n", startRow, cacheRow);
          }
          
          printf("updateRowCache iterate offsets (%d,%d) inclusive\n", startOffset, endOffset);
        }
        
        // Loop over previous 3 delta values, note that this logic only looks at
        // the deltas, if a delta is invalid then it is ignored.
        
        const auto & cachedL1DeltaVec = cachedL1.values;
        
        for ( int offset = startOffset; offset <= endOffset; offset++ ) {
          int cachedVal = cachedL1DeltaVec[offset];
          
          if (debug) {
            printf("updateRowCache read cached delta at offset %d : delta %d\n", offset, cachedVal);
          }
          
          if (cachedVal != -1) {
            sum += cachedVal;
            N += 1;
            
            if (debug) {
              printf("updateRowCache add cached %s delta at offset %d : sum %d\n", isHorizontal ? (char*)"H" : (char*)"V", offset, sum);
            }
          }
        }
        
        // Calculate ave of 0,1,2,3 values and cache this value
        
        vector<T> & cachedSum3Vec = cachedDeltaRows;
        
#if defined(DEBUG)
        assert(endOffset >= 0);
        assert(endOffset < cachedSum3Vec.size());
        assert(cachedSum3Vec[endOffset] == Cache2DSum3_Invalid);
#endif // DEBUG
        
        // Each cached sum is the collection of the last 4 pixels, component by component
        
        unsigned int ave = -1;
        
        switch (N) {
          case 0: {
            // No cached values in this row
            ave = -1;
            break;
          }
          case 1: {
            // Nop since the sum for a row contains only 1 value
            ave = sum;
            break;
          }
          case 2: {
            // Ave of 2 elements
            ave = fast_div_2(sum);
            break;
          }
          case 3: {
            // Efficient N / 3
            ave = fast_div_3(sum);
            break;
          }
#if defined(DEBUG)
          default: {
            assert(0);
          }
#endif // DEBUG
        }
        
        // FIXME: if the read operation is actually faster without the delta() AND mask, then do not store N
        
        // Note that the delta can be -1 when N == 0, this is a valid cached value that indicated that no
        // values N=0 was properly cached.
        
        if (N == 0) {
          cachedSum3Vec[endOffset] = Cache2DSum3_ZeroRows;
          
#if defined(DEBUG)
          assert((cachedSum3Vec[endOffset] == Cache2DSum3_Invalid) == false);
#endif // DEBUG
        } else {
#if defined(DEBUG)
          this->checkMaxValue(ave);
#endif // DEBUG
          
          cachedSum3Vec[endOffset] = ave;
          
          if (debug) {
            printf("updateRowCache set new non-zero %d sum/ave (%d,%d) : N = %d : ave %d\n", endOffset, cacheCol, cacheRow, N, ave);
          }
          
#if defined(DEBUG)
          assert((cachedSum3Vec[endOffset] == Cache2DSum3_Invalid) == false);
#endif // DEBUG
        }
        
        if (debug) {
          printf("updateRowCache (%d,%d) N = %d : delta sum %d\n", cacheCol, cacheRow, N, ave);
        }
      }
      
      val = cachedDeltaRows[offset];
      
#if defined(DEBUG)
      assert((val == Cache2DSum3_Invalid) == false);
#endif // DEBUG
      
      if (debug) {
        printf("stored updated cached value at (%d,%d) offset %d : delta %d\n", cacheCol, cacheRow, offset, val);
      }
    } else {
      if (debug) {
        printf("cache is valid at (%d,%d) offset %d : isHorizontal %d\n", cacheCol, cacheRow, offset, isHorizontal);
      }
    }
    
    if (val == Cache2DSum3_ZeroRows) {
      // A valid cached value that indicates zero cached values,
      // this is handled by returning -1 as a special case.
      return -1;
    } else {
      return val;
    }
  }
  
};

#endif // CACHE_2D_H
