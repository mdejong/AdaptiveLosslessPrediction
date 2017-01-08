//
//  ColortableIter.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Iteration order over a matrix of values based on
//  differences of known pixels. Note that this approach
//  does calculation based on a colortable lookup.

#include "assert.h"

#include <vector>
#include <list>
#include <queue>
#include <set>

#include <unordered_map>

#define USE_BOX_DELTA

#if defined(DEBUG)
#include <iostream>
#endif // DEBUG

#include <sstream>

#import "PredFuncs.hpp"
#import "Cache2D.hpp"

#import "StaticPrioStack.hpp"

using namespace std;

static inline
int CTIOffset2d(int x, int y, const int width) {
  return (y * width) + x;
}

class CoordDelta {
public:
  // Empty constructor
  
  CoordDelta()
  : _toX(0xFFFF/2), _toY(0xFFFF/2), _isExact(false), _isHorizontal(false)
  {
  }
  
  CoordDelta(int x1, int y1, int x2, int y2, bool inIsHorizontal) {
    _isHorizontal = inIsHorizontal;

    // (x,y)
    
#if defined(DEBUG)
    // Note that 0xFFFF is not a valid value
    assert(x1 >= 0 && x1 < 0xFFFF/2);
    assert(y1 >= 0 && y1 < 0xFFFF/2);
    assert(x2 >= 0 && x2 < 0xFFFF/2);
    assert(y2 >= 0 && y2 < 0xFFFF/2);
    
    assert(sizeof(*this) == 4); // 32 bits
#endif // DEBUG
    
    _toX = x2;
    _toY = y2;
    
#if defined(DEBUG)
    {
      int _fromX = fromX();
      int _fromY = fromY();
      assert(_fromX == x1);
      assert(_fromY == y1);
    }
#endif // DEBUG
  }
  
  bool isHorizontal() const {
    return _isHorizontal;
  }
  
  // Return true for an object created with all zero values. This isEmpty() test assumes
  // that the coord value (0,0) will never be a destination of a delta.
  
  bool isEmpty() const {
    if (toX() == 0xFFFF/2 && toY() == 0xFFFF/2) {
      return true;
    } else {
      return false;
    }
  }
  
  bool isExact() const {
    return _isExact;
  }
  
  string toString() const {
    char buffer[100];
    if (isEmpty()) {
      snprintf(buffer, sizeof(buffer), "( empty )");
    } else {
      snprintf(buffer, sizeof(buffer), "(%2d,%2d) -> (%2d,%2d) : isHorizontal %d", fromX(), fromY(), toX(), toY(), isHorizontal());
    }
    return string(buffer);
  }
  
  // getters
  
  int fromX() const {
    if (isHorizontal()) {
      return toX() - 1;
    } else {
      return toX();
    }
  }
  
  int fromY() const {
    if (isHorizontal()) {
      return toY();
    } else {
      return toY() - 1;
    }
  }
  
  int toX() const {
    return _toX;
  }
  
  int toY() const {
    return _toY;
  }
  
private:
  // The (x,y) coords of the "to" coordinate
  
  unsigned int _toX : 15;
  unsigned int _isExact : 1;
  unsigned int _toY : 15;
  unsigned int _isHorizontal : 1;
};

// Each wait list is represented by a vector<CoordDelta> so that the size
// of an allocation is a multiple of the sizeof(CoordDelta).

class CTI_Struct
{
public:
  // The wait list is a statically defined prio stack
  
  StaticPrioStack<CoordDelta> waitList;

#if defined(USE_BOX_DELTA)
  // Cached H and V delta calculations, these need only
  // be executed once and then they can be reused by
  // multiple pixels.
  
  Cache2D<int16_t, true> cachedHDeltaSums;
  Cache2D<int16_t, false> cachedVDeltaSums;

  // Both H and V cached deltas are stored in memory in
  // the same orientation. This makes it possible to
  // use a single function to read cached values in
  // a memory optimal fasion.
  
  Cache2DSum3<int16_t, true> cachedHDeltaRows;
  Cache2DSum3<int16_t, false> cachedVDeltaRows;
#endif // USE_BOX_DELTA

  // grid of true or false state for each pixel
  
  vector<uint8_t> processedFlags;

#if defined(DEBUG)
  unordered_map<string,int> results;
#endif // DEBUG
  
  int width;
  int height;
  
  uint32_t * pixelsPtr;
  uint32_t * colortablePixelsPtr;
  int colortableNumPixels;
  uint8_t * tableOffsetsPtr;
  
  CTI_Struct()
  {
  }
  
  ~CTI_Struct() {
  }
  
  void initWaitList(int numErrs) {
    waitList.allocateN(numErrs);
  }

  bool isWaitListEmpty() {
    return waitList.isEmpty();
  }
  
  int32_t waitListHead() {
    return waitList.head();
  }
  
  // Pass specific error value, -1 for all values in list
  
  string waitListToString(unsigned int err) {
    stringstream s;
    
    if (err == -1) {
      err = 0;
      for ( vector<CoordDelta> & errTable : waitList.elemTable ) {
        for ( auto it = errTable.rbegin(); it != errTable.rend(); it++ ) {
          //const CoordDelta & cd = *it;
          s << err << ",";
        }
        err += 1;
      }
    } else {
      vector<CoordDelta> & errTable = waitList.elemTable[err];
      
      for ( auto it = errTable.rbegin(); it != errTable.rend(); it++ ) {
        //const CoordDelta & cd = *it;
        s << err << ",";
      }
    }
    
    return s.str();
  }
  
  // Clear all entries out of wait list and the waitListErrTable
  
  void clearWaitList() {
    waitList.clear();
  }

  // Util to insert a node before indicated err slot, note that
  // this method depends on an existing node to insert before.

  void insertWaitListNode(int err, int insertBeforeErr) {
    waitList.insertNode(err, insertBeforeErr);
  }
  
  // When a value must be appened to the end of the list, there is no
  // node to insert before. This logic handles the special case of
  // an append to the end of the list.
  
  void appendWaitListNode(int err, int appendAfterErr) {
    waitList.appendNode(err, appendAfterErr);
  }
  
  // When a specific node becomes empty, remove it from the wait list
  
  void unlinkWaitListNode(int err) {
    waitList.unlinkNode(err);
  }
  
  // FILO push to front of list for a specific err level
  
  void addToWaitList(const CoordDelta & cd, unsigned int err) {
    waitList.push(cd, err);
  }
  
  // Grab the first element in the wait list, note that this
  // method returns a zero value when the wait list is empty
  
  CoordDelta firstOnWaitList(int * errPtr) {
    return waitList.first(errPtr);
  }
  
  // Return true if the given coord has been processed, false if not
  
  bool wasProcessed(int x, int y)
  {
#if defined(DEBUG)
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
#endif // DEBUG
    
    int offset = CTIOffset2d(x, y, width);
#if defined(DEBUG)
    assert(offset < processedFlags.size());
#endif // DEBUG
    bool wasTargetPixelProcessed = (processedFlags[offset] != 0);
    return wasTargetPixelProcessed;
  }
  
  // Set processed flag for specific (X, Y) coordinate
  
  void setProcessed(int x, int y)
  {
#if defined(DEBUG)
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
#endif // DEBUG

    
    int offset = CTIOffset2d(x, y, width);
    processedFlags[offset] = 1;
  }
  
  // Faster version of setProcessed in the case where the offset was already computed
  
  void setProcessed(int offset)
  {
    processedFlags[offset] = 1;
  }

  // Count number of H or V deltas on wait list.
  
  int countDeltasTypes(bool countH, bool countV) {
    int countHSum = 0;
    int countVSum = 0;

    for ( auto err = waitListHead(); err != -1; ) {
      auto & node = waitList.nodeTable[err];
      vector<CoordDelta> & errTable = waitList.elemTable[err];
      
      for ( CoordDelta cd : errTable ) {
        if (cd.isHorizontal()) {
          if (countH) {
            countHSum += 1;
          }
        } else {
          if (countV) {
            countVSum += 1;
          }
        }
      }
      
      err = node.next;
    }
    
    return countHSum + countVSum;
  }
  
  // Simple pixel lookup, given the offset into the image.
  
  uint32_t pixelLookup(
                  const bool tablePred,
                  int offset)
  {
    uint32_t pixel;
    if (tablePred) {
      pixel = colortablePixelsPtr[tableOffsetsPtr[offset]];
    } else {
      pixel = pixelsPtr[offset];
    }
#if defined(DEBUG)
    pixel = pixel & 0x00FFFFFF;
#endif // DEBUG
    return pixel;
  }
  
  // Determine a delta given 2 offsets, can be either H or V
  
  int simpleDelta(
                  const bool tablePred,
                  int fromOffset,
                  int toOffset)
  {
    int delta;
    
    if (tablePred) {
      //delta = CTITablePredict(tableOffsetsPtr, colortablePixelsPtr, fromOffset, toOffset, otherOffset, colortableNumPixels);
      delta = CTITablePredict2(tableOffsetsPtr, colortablePixelsPtr, fromOffset, toOffset, colortableNumPixels);
    } else {
      //delta = CTIPredict(pixelsPtr, fromOffset, toOffset, otherOffset);
      delta = CTIPredict2(pixelsPtr, fromOffset, toOffset);
    }
    
    return delta;
  }

  // Calculate delta between 2 pixels

  unsigned int componentDelta(
                          unsigned int p1,
                          unsigned int p2)
  {
    const bool debug = false;
    
    if (p1 == p2) {
      return 0;
    }
    
    // Component delta is a simple SUB each component from p1 -> p2
    
    if (debug) {
      printf("predict : 0x%08X -> 0x%08X\n", p1, p2);
    }
    
    uint32_t deltaPixel = pixel_component_delta(p1, p2, 3);
    
    if (debug) {
      printf("comp delta     : 0x%08X\n", deltaPixel);
    }
    
    return deltaPixel;
  }
  
#if defined(USE_BOX_DELTA)

  // Update cached delta values, this method is invoked after a pixel has been processed
  
  void updateCache(
                   const bool tablePred,
                   const int cacheCol,
                   const int cacheRow)
  {
    const bool debug = false;
    
    if (debug) {
      printf("updateCache (%d,%d)\n", cacheCol, cacheRow);
    }
    
    // FIXME: In each check, avoid doing a calculation of the
    // offset using a multiply when possible. For example, it
    // may not be required to do the mult when the wasProcessed()
    // returns false for a specific (X,Y) pair.
    
    // C offset in pixels table is common to each calculation
    
    int centerOffset = CTIOffset2d(cacheCol, cacheRow, width);
    
    // L -> C is H cache for (-1, 0)
    
    {
      int prevCol = cacheCol - 1;
      
      if (prevCol >= 0) {
        int leftOffset = CTIOffset2d(prevCol, cacheRow, width);

#if defined(DEBUG)
        cachedHDeltaSums.assertIfInvalidOffset(leftOffset);
#endif // DEBUG
        
#if defined(DEBUG)
        assert(cachedHDeltaSums.values[leftOffset] == -1);
#endif // DEBUG
        
        // FIXME: If L has been cached, then it would be possible to know
        // the (LL -> L) is already valid and for that reason a read
        // of wasProcessed() is not needed. (optimization)
        
        // Hold ref to L
        
        auto & cachedDelta = cachedHDeltaSums.values[leftOffset];
        
        bool pixelWasProcessed = wasProcessed(prevCol, cacheRow);
        if (pixelWasProcessed) {
          // Invoking CTI_PredictDelta3 w other=-1 returns an unscaled delta + abs sum
          
          int delta = simpleDelta(tablePred, leftOffset, centerOffset);
          
#if defined(DEBUG)
          assert(delta < 0xFFFF);
#endif // DEBUG
          
          cachedDelta = delta;
          
          if (debug) {
            printf("calc L->C H cache : (%d, %d) -> (%d, %d) = delta %d : offset %d\n", prevCol, cacheRow, cacheCol, cacheRow, (int)cachedDelta, leftOffset);
          }
        }
      }
    }

    // C -> R is H cache for (+0, +0)
    
    {
      int nextCol = cacheCol + 1;
      
      if (nextCol < width) {
        int rightOffset = CTIOffset2d(nextCol, cacheRow, width);
        
#if defined(DEBUG)
        cachedHDeltaSums.assertIfInvalidOffset(centerOffset);
        assert(cachedHDeltaSums.values[centerOffset] == -1);
#endif // DEBUG
        
        // FIXME: If RR has been cached, then (R -> RR) R is processed without invoking wasProcessed().
        
        auto & cachedDelta = cachedHDeltaSums.values[centerOffset];
        
        bool pixelWasProcessed = wasProcessed(nextCol, cacheRow);
        if (pixelWasProcessed) {
          // Invoking CTI_PredictDelta3 w other=-1 returns an unscaled delta + abs sum
          
          int delta = simpleDelta(tablePred, centerOffset, rightOffset);
          
#if defined(DEBUG)
          assert(delta < 0xFFFF);
#endif // DEBUG
          
          cachedDelta = delta;
          
          if (debug) {
            printf("calc C->R H cache : (%d, %d) -> (%d, %d) = delta %d : offset %d\n", cacheCol, cacheRow, nextCol, cacheRow, (int)cachedDelta, centerOffset);
          }
        }
      }
    }
    
    // U -> C is V cache for (0, -1) (transposed)
    
    {
      int prevRow = cacheRow - 1;
      
      if (prevRow >= 0) {
        // Transposed offset calculation
        int upOffset = CTIOffset2d(cacheCol, prevRow, width);
        int upOffsetT = CTIOffset2d(prevRow, cacheCol, height);
        
#if defined(DEBUG)
        cachedHDeltaSums.assertIfInvalidOffset(upOffsetT);
        assert(cachedVDeltaSums.values[upOffsetT] == -1);
#endif // DEBUG
        
        auto & cachedDelta = cachedVDeltaSums.values[upOffsetT];
        
        bool pixelWasProcessed = wasProcessed(cacheCol, prevRow);
        if (pixelWasProcessed) {
          // Invoking CTI_PredictDelta3 w other=-1 returns an unscaled delta + abs sum
          
          int delta = simpleDelta(tablePred, upOffset, centerOffset);
          
#if defined(DEBUG)
          assert(delta < 0xFFFF);
#endif // DEBUG
          
          cachedDelta = delta;
          
          if (debug) {
            printf("calc U->C V cache : (%d, %d) -> (%d, %d) = delta %d : offset %d\n", cacheCol, prevRow, cacheCol, cacheRow, (int)cachedDelta, upOffsetT);
          }
        }
      }
    }

    // C -> D is V cache for (+0, +0) (transposed)
    
    {
      int nextRow = cacheRow + 1;
      
      if (nextRow < height) {
        // Transposed offset calculation
        int downOffset = CTIOffset2d(cacheCol, nextRow, width);
        //int downOffsetT = CTIOffset2d(nextRow, cacheCol, height);

        int centerOffsetT = CTIOffset2d(cacheRow, cacheCol, height);
        
#if defined(DEBUG)
        cachedHDeltaSums.assertIfInvalidOffset(centerOffsetT);
        assert(cachedVDeltaSums.values[centerOffsetT] == -1);
#endif // DEBUG
        
        auto & cachedDelta = cachedVDeltaSums.values[centerOffsetT];
        
        bool pixelWasProcessed = wasProcessed(cacheCol, nextRow);
        if (pixelWasProcessed) {
          // Invoking CTI_PredictDelta3 w other=-1 returns an unscaled delta + abs sum
          
          int delta = simpleDelta(tablePred, centerOffset, downOffset);
          
#if defined(DEBUG)
          assert(delta < 0xFFFF);
#endif // DEBUG
          
          cachedDelta = delta;
          
          if (debug) {
            printf("calc C->D V cache : (%d, %d) -> (%d, %d) = delta %d : offset %d\n", cacheCol, cacheRow, cacheCol, nextRow, (int)cachedDelta, centerOffsetT);
          }
        }
      }
    }
    
    if (debug) {
      printf("done updateCache (%d,%d)\n", cacheCol, cacheRow);
    }
    
    return;
  }

  // Invalidate either a H or V cache for the indicate pixel.
  // Note that in the case of a vertical cache the pixel coordinates
  // are transposed.
  
  void invalidateRowCache(
                          const bool isHorizontal,
                          const int cacheCol,
                          const int cacheRow)
  {
    const bool debug = false;
    
    if (debug) {
      printf("invalidateRowCache (%d,%d) : isHorizontal %d\n", cacheCol, cacheRow, isHorizontal);
    }

    if (isHorizontal) {
      cachedHDeltaRows.invalidate(cacheCol, cacheRow);
    } else {
      cachedVDeltaRows.invalidate(cacheCol, cacheRow);
    }
    
    return;
  }
  
  // Get the current cached value at an offset
  
  int16_t getRow3CacheValue(
                            const bool isHorizontal,
                            const int cacheCol,
                            const int cacheRow)
  {
    const bool debug = false;
    
    if (debug) {
      printf("getRow3CacheValue (%d,%d) : isHorizontal %d\n", cacheCol, cacheRow, isHorizontal);
    }
    
    if (isHorizontal) {
      return cachedHDeltaRows.getCachedValue(cachedHDeltaSums, cacheCol, cacheRow);
    } else {
      return cachedVDeltaRows.getCachedValue(cachedVDeltaSums, cacheCol, cacheRow);
    }
  }
  
#endif // USE_BOX_DELTA
};

// Given the from and to coordinates, determine the prediction delta value
// based on which neighbor pixels are currently defined

static inline
int CTI_PredictDelta3(
                     const uint32_t * const pixelsPtr,
                     const uint32_t * const colortablePixelsPtr,
                     const int colortableNumPixels,
                     const uint8_t * const tableOffsetsPtr,
                     const bool tablePred,
                     const int regionWidth,
                     const int regionHeight,
                      CTI_Struct & ctiStruct,
                     int fromOffset,
                     int toOffset,
                     int otherX,
                     int otherY)
{
  const bool debug = false;
  
  int otherOffset = -1;
  
  // Determine if other is set by looking at the horizontal
  // and vertical counters that correspond to (x,y)
  
  if (otherX >= 0 && otherY >= 0) {
    bool otherPixelWasProcessed = ctiStruct.wasProcessed(otherX, otherY);
    
    if (otherPixelWasProcessed) {
      otherOffset = CTIOffset2d(otherX, otherY, regionWidth);
    }
  }
  
  int delta;
  
  if (debug) {
    printf("CTI_PredictDelta3\n");
  }
  
  if (tablePred) {
    delta = CTITablePredict(tableOffsetsPtr, colortablePixelsPtr, fromOffset, toOffset, otherOffset, colortableNumPixels);
  } else {
    delta = CTIPredict(pixelsPtr, fromOffset, toOffset, otherOffset);
  }
  
  return delta;
}

// Arrow Prediction (Horizontal)
//  x - -
//  - > x
//  x - -
//
// UL U UR
//  L C  R
// DL D DR
//
//  x U UR
//  L C  x
//  x D DR
//
// U  = (0,-1)
// UR = (+1,-1)
// L  = (-1,0)
// C  = (0,0)
// D  = (0,+1)
// DR = (+1,+1)
//
// Arrow Prediction (Vertical)
//  x || x
//  | \/ |
//  | x  |
//
//  x L  x
//  D C  U
// DR x UR
//

// U  = (+1, 0)
// UR = (+1, +1)
// L  = (0, -1)
// C  = (0,  0)
// D  = (-1, 0)
// DR = (-1, +1)

// FIXME: could this logic hold on to a bit pattern that would know
// which bits were defined the last time a recalc was done? That way
// a new reclac would only be done when more neighbors have been defined?

template <const bool isHorizontal>
static inline
int CTI_PredictDeltaArrow(
                          CTI_Struct & ctiStruct,
                          const bool tablePred,
                          int fromOffset,
                          int toOffset,
                          int centerX,
                          int centerY)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_PredictDeltaArrow(%d,%d) : isHorizontal %d\n", centerX, centerY, isHorizontal);
  }
  
  // FIXME: rename later
  const int regionWidth = ctiStruct.width;
  const int regionHeight = ctiStruct.height;
  
#if defined(DEBUG)
  if (isHorizontal) {
    // Must be horizontal delta
    assert((fromOffset + 1) == toOffset);
  } else {
    // Must be vertical delta
    assert((fromOffset + regionWidth) == toOffset);
  }
#endif // DEBUG
  
  // Horizontal L  = (-1,0)
  // Vertical   L  = (0,-1)
  
  const int xL = isHorizontal ? (centerX - 1) : (centerX + 0);
  const int yL = isHorizontal ? (centerY + 0) : (centerY - 1);
  
  // Horizontal U = (0,-1)
  // Vertical   U = (+1,0)

  const int xU = isHorizontal ? (centerX + 0) : (centerX + 1);
  const int yU = isHorizontal ? (centerY - 1) : (centerY + 0);
  
  // Horizontal UR = (+1,-1)
  // Vertical   UR = (+1, +1)

  const int xUR = isHorizontal ? (centerX + 1) : (centerX + 1);
  const int yUR = isHorizontal ? (centerY - 1) : (centerY + 1);

  // Horizontal D = (0,+1)
  // Vertical   D = (-1,0)

  const int xD = isHorizontal ? (centerX + 0) : (centerX - 1);
  const int yD = isHorizontal ? (centerY + 1) : (centerY + 0);

  // Horizontal DR = (+1,+1)
  // Vertical   DR = (-1,+1)

  const int xDR = isHorizontal ? (centerX + 1) : (centerX - 1);
  const int yDR = isHorizontal ? (centerY + 1) : (centerY + 1);
  
#if defined(DEBUG)
  // The L and C coordinates must be defined
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed);
  }
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(xL, yL);
    assert(pixelWasProcessed);
  }
#endif // DEBUG
  
  if (debug) {
    printf("U  (%d,%d) %d\n", xU, yU, CTIOffset2d(xU, yU, regionWidth));
    printf("UR (%d,%d) %d\n", xUR, yUR, CTIOffset2d(xUR, yUR, regionWidth));
    printf("L  (%d,%d) %d\n", xL, yL, fromOffset);
    printf("C  (%d,%d) %d\n", centerX, centerY, toOffset);
    printf("D  (%d,%d) %d\n", xD, yD, CTIOffset2d(xD, yD, regionWidth));
    printf("DR (%d,%d) %d\n", xDR, yDR, CTIOffset2d(xDR, yDR, regionWidth));
  }
  
  // Test to see if pixels above and below are defined
  
  bool upWasProcessed = false;
  
  // Horizontal U = (0,-1)
  // Vertical   U = (+1,0)
  
  if ((isHorizontal && (yU >= 0)) ||
      (!isHorizontal && (xU < regionWidth))) {
    upWasProcessed = ctiStruct.wasProcessed(xU, yU);
  }
  
  bool upRWasProcessed = false;
  
  // Horizontal UR = (+1,-1)
  // Vertical   UR = (+1, +1)
  
  if ((isHorizontal && (yUR >= 0 && xUR < regionWidth)) ||
      (!isHorizontal && (yUR < regionHeight && xUR < regionWidth))) {
    upRWasProcessed = ctiStruct.wasProcessed(xUR, yUR);
  }
  
  bool downWasProcessed = false;
  
  // Horizontal D = (0,+1)
  // Vertical   D = (-1,0)
  
  if ((isHorizontal && (yD < regionHeight)) ||
      (!isHorizontal && (xD >= 0))) {
    downWasProcessed = ctiStruct.wasProcessed(xD, yD);
  }
  
  bool downRWasProcessed = false;
  
  // Horizontal DR = (+1,+1)
  // Vertical   DR = (-1,+1)
  
  if ((isHorizontal && (xDR < regionWidth && yDR < regionHeight)) ||
      (!isHorizontal && (xDR >= 0 && yDR < regionHeight))) {
    downRWasProcessed = ctiStruct.wasProcessed(xDR, yDR);
  }

  // Logic that depends on which pixels are defined at this point

  bool sameUD = false;
  bool sameURDR = false;
  
  // If U is not defined but D is, then use the value for D
  // instead of U (mirror). Otherwise use C.

  int downOffset = -1;
  
  if (downWasProcessed) {
    downOffset = CTIOffset2d(xD, yD, regionWidth);
  }
  
  int upOffset;
  
  if (upWasProcessed) {
    upOffset = CTIOffset2d(xU, yU, regionWidth);
  } else {
    // If U is not defined, but UU is, then use UU to deal with
    // a prediction hole caused by a strong vertical edge.
    
    // Horizontal UU = (0,-2)
    // Vertical   UU = (+2,0)
    
    const int xUU = isHorizontal ? (centerX + 0) : (centerX + 2);
    const int yUU = isHorizontal ? (centerY - 2) : (centerY + 0);
    
    bool upUpWasProcessed = false;
    
    if ((isHorizontal && (yUU >= 0)) ||
        (!isHorizontal && (xUU < regionWidth))) {
      upUpWasProcessed = ctiStruct.wasProcessed(xUU, yUU);
    }
    
    if (upUpWasProcessed) {
      int upUpOffset = CTIOffset2d(xUU, yUU, regionWidth);
      upOffset = upUpOffset;
    } else if (downWasProcessed) {
      upOffset = downOffset;
      sameUD = true;
    } else {
      upOffset = toOffset;
    }
    
    if (debug) {
      printf("U not processed\n");
    }
  }
  
  // If UR is not defined at this point, then read UR from U.
  // In the case that U was not defined, then UR defaults to C.
  
  int downROffset = -1;
  
  if (downRWasProcessed) {
    downROffset = CTIOffset2d(xDR, yDR, regionWidth);
  }
  
  int upROffset;
  
  if (upRWasProcessed) {
    upROffset = CTIOffset2d(xUR, yUR, regionWidth);
  } else {
    if (downRWasProcessed) {
      upROffset = downROffset;
      sameURDR = true;
    } else {
      upROffset = upOffset;
    }
    
    if (debug) {
      printf("UR not processed\n");
    }
  }
  
  // If D is not defined at this point
  
  if (!downWasProcessed) {
    // If D is not defined, but DD is, then use DD to deal with
    // a prediction hole caused by a strong vertical edge.
    
    // Horizontal DD = (0,+2)
    // Vertical   DD = (-2,0)
    
    const int xDD = isHorizontal ? (centerX + 0) : (centerX - 2);
    const int yDD = isHorizontal ? (centerY + 2) : (centerY + 0);
    
    bool downDownWasProcessed = false;
    
    if ((isHorizontal && (yDD < regionHeight)) ||
        (!isHorizontal && (xDD >= 0))) {
      downDownWasProcessed = ctiStruct.wasProcessed(xDD, yDD);
    }
    
    if (downDownWasProcessed) {
      int downDownOffset = CTIOffset2d(xDD, yDD, regionWidth);
      downOffset = downDownOffset;
    } else {
      // read D from U (mirror)
      
      downOffset = upOffset;
      sameUD = true;
    }
    
    if (debug) {
      printf("D not processed\n");
    }
  }
  
  // If DR is not defined at this point, then read DR from UR. (mirror)
  
  if (!downRWasProcessed) {
    downROffset = upROffset;
    sameURDR = true;
    
    if (debug) {
      printf("DR not processed\n");
    }
  }
  
  // Pixel access order U, UR, L, C, D, DR

#if defined(DEBUG)
  assert(upOffset >= 0);
  assert(upROffset >= 0);
  assert(fromOffset >= 0);
  assert(toOffset >= 0);
  assert(downOffset >= 0);
  assert(downROffset >= 0);
#endif

  const uint32_t * const pixelsPtr = ctiStruct.pixelsPtr;
  const uint8_t * const tableOffsetsPtr = ctiStruct.tableOffsetsPtr;
  const uint32_t * const colortablePixelsPtr = ctiStruct.colortablePixelsPtr;
  const int colortableNumPixels = ctiStruct.colortableNumPixels;
  
  if (debug) {
    uint32_t upPixel;
    uint32_t upRPixel;
    uint32_t leftPixel;
    uint32_t centerPixel;
    uint32_t downPixel;
    uint32_t downRPixel;
    
    if (tablePred) {
      int upColortableIndex = tableOffsetsPtr[upOffset];
      int upRColortableIndex = tableOffsetsPtr[upROffset];
      int leftColortableIndex = tableOffsetsPtr[fromOffset];
      int centerColortableIndex = tableOffsetsPtr[toOffset];
      
      upPixel = colortablePixelsPtr[upColortableIndex];
      upRPixel = colortablePixelsPtr[upRColortableIndex];
      leftPixel = colortablePixelsPtr[leftColortableIndex];
      centerPixel = colortablePixelsPtr[centerColortableIndex];
    } else {
      upPixel = pixelsPtr[upOffset];
      upRPixel = pixelsPtr[upROffset];
      leftPixel = pixelsPtr[fromOffset];
      centerPixel = pixelsPtr[toOffset];
    }
    
    printf("U      0x%08X\n", upPixel);
    printf("UR     0x%08X\n", upRPixel);
    
    printf("L      0x%08X\n", leftPixel);
    printf("center 0x%08X\n", centerPixel);
    
    if (tablePred) {
      int downColortableIndex = tableOffsetsPtr[downOffset];
      int downRColortableIndex = tableOffsetsPtr[downROffset];
      
      downPixel = colortablePixelsPtr[downColortableIndex];
      downRPixel = colortablePixelsPtr[downRColortableIndex];
    } else {
      downPixel = pixelsPtr[downOffset];
      downRPixel = pixelsPtr[downROffset];
    }
    
    printf("D      0x%08X\n", downPixel);
    printf("DR     0x%08X\n", downRPixel);
  }
  
  // Common

  uint32_t centerPixelOrTableIndex;
  
  if (tablePred) {
    centerPixelOrTableIndex = tableOffsetsPtr[toOffset];
  } else {
    centerPixelOrTableIndex = pixelsPtr[toOffset];
  }
  
  // Delta L -> C
  
  unsigned int centerDeltaSum;
  
  if (tablePred) {
    int leftColortableIndex = tableOffsetsPtr[fromOffset];
    
    int wrappedDelta = convertToWrappedTableDelta(leftColortableIndex, centerPixelOrTableIndex, colortableNumPixels);
    centerDeltaSum = convertSignedZeroDeltaToUnsigned(wrappedDelta);
  } else {
    uint32_t leftPixel = pixelsPtr[fromOffset];
    uint32_t centerDeltaPixel = pixel_component_delta(leftPixel, centerPixelOrTableIndex, 3);
    
    if (debug) {
      printf("center delta        : 0x%08X\n", centerDeltaPixel);
    }
    
    centerDeltaSum = sum_of_abs_components(centerDeltaPixel, 3);
  }
    
  // Delta C -> U
  
  unsigned int centerUSum;
  
  if (tablePred) {
    int upColortableIndex = tableOffsetsPtr[upOffset];
    
    int wrappedDelta = convertToWrappedTableDelta(centerPixelOrTableIndex, upColortableIndex, colortableNumPixels);
    centerUSum = convertSignedZeroDeltaToUnsigned(wrappedDelta);
  } else {
    uint32_t upPixel = pixelsPtr[upOffset];
    uint32_t centerUDeltaPixel = pixel_component_delta(centerPixelOrTableIndex, upPixel, 3);
    
    if (debug) {
      printf("center -> U delta   : 0x%08X\n", centerUDeltaPixel);
    }
    
    centerUSum = sum_of_abs_components(centerUDeltaPixel, 3);
  }
    
  // Delta C -> UR

  unsigned int centerURSum;
  
  if (tablePred) {
    int upRColortableIndex = tableOffsetsPtr[upROffset];
    
    int wrappedDelta = convertToWrappedTableDelta(centerPixelOrTableIndex, upRColortableIndex, colortableNumPixels);
    centerURSum = convertSignedZeroDeltaToUnsigned(wrappedDelta);
  } else {
    uint32_t upRPixel = pixelsPtr[upROffset];
    uint32_t centerURDeltaPixel = pixel_component_delta(centerPixelOrTableIndex, upRPixel, 3);
    
    if (debug) {
      printf("center -> UR delta  : 0x%08X\n", centerURDeltaPixel);
    }
    
    centerURSum = sum_of_abs_components(centerURDeltaPixel, 3);
    
    if (debug) {
      printf("center -> UR sum  : %d\n", centerURSum);
    }
  }
    
  // C -> D
  
  unsigned int centerDSum;
  
  if (sameUD) {
    if (debug) {
      printf("center -> D (dup of C -> U)\n");
    }
    
    centerDSum = centerUSum;
  } else {
    if (tablePred) {
      int downColortableIndex = tableOffsetsPtr[downOffset];
      
      int wrappedDelta = convertToWrappedTableDelta(centerPixelOrTableIndex, downColortableIndex, colortableNumPixels);
      centerDSum = convertSignedZeroDeltaToUnsigned(wrappedDelta);
    } else {
      uint32_t downPixel = pixelsPtr[downOffset];
      uint32_t centerDDeltaPixel = pixel_component_delta(centerPixelOrTableIndex, downPixel, 3);
      
      if (debug) {
        printf("center -> D delta   : 0x%08X\n", centerDDeltaPixel);
      }
      
      centerDSum = sum_of_abs_components(centerDDeltaPixel, 3);
    }
  }
  
  // C -> DR

  unsigned int centerDRSum;
  
  if (sameURDR) {
    if (debug) {
      printf("center -> DR (dup of C -> UR)\n");
    }
    
    centerDRSum = centerURSum;
  } else {
    if (tablePred) {
      int downRColortableIndex = tableOffsetsPtr[downROffset];
      
      int wrappedDelta = convertToWrappedTableDelta(centerPixelOrTableIndex, downRColortableIndex, colortableNumPixels);
      centerDRSum = convertSignedZeroDeltaToUnsigned(wrappedDelta);
    } else {
      uint32_t downRPixel = pixelsPtr[downROffset];
      uint32_t centerDRDeltaPixel = pixel_component_delta(centerPixelOrTableIndex, downRPixel, 3);
      
      if (debug) {
        printf("center -> DR delta  : 0x%08X\n", centerDRDeltaPixel);
      }
      
      centerDRSum = sum_of_abs_components(centerDRDeltaPixel, 3);
    }
  }
  
  // Print the sum values in arrow pattern with the int values filled in
  
  if (debug) {
    // x - -
    // - > x
    // x - -

    if (isHorizontal) {
      printf("____________\n");
      printf("x %3d  %3d\n", centerUSum, centerURSum);
      printf("- %3d    x\n", centerDeltaSum);
      printf("x %3d  %3d\n", centerDSum, centerDRSum);
      printf("____________\n");
    } else {
      printf("|    x    -    x |\n");
      printf("| %4d %4d %4d |\n", centerDSum, centerDeltaSum, centerUSum);
      printf("| %4d    x %4d |\n", centerDRSum, centerURSum);
    }
    
    printf("U  sum : %d\n", centerUSum);
    printf("UR sum : %d\n", centerURSum);
    printf("C  sum : %d\n", centerDeltaSum);
    printf("D  sum : %d\n", centerDSum);
    printf("DR sum : %d\n", centerDRSum);
  }
  
  // FIXME: might be better to use (2*6)/16 and then
  // avoid the shift right average here.
  
  // Total weights 6/8 + 1/8 + 1/8 = 8/8

  unsigned int upDownSum = (centerUSum + centerDSum);
  unsigned int diagSum = (centerURSum + centerDRSum);
  
  if (debug) {
    printf("UD   ave sum : %4d\n", upDownSum);
    printf("URDR ave sum : %4d\n", diagSum);
  }
  
  // FIXME: impl mult 6 and (mult 4 + mult 2) + ave(up, diag)
  
  const unsigned int centerDeltaWeight = 6; // 6 of 8
  
  unsigned int sum = (centerDeltaSum * centerDeltaWeight) + ((upDownSum + diagSum) >> 1);

  if (debug) {
    printf("unscaled sum    : %4d\n", sum);
  }
  
  // Scale sum back own to original range
  
  sum >>= 3; // N / 8
  
  if (debug) {
    printf("total sum    : %4d\n", sum);
  }
  
  // FIXME: need way to return the number of neighbors seen, so that
  // if put back into the tree, another recalc is not needed.
  
  return sum;
}

#if defined(USE_BOX_DELTA)

// Predict a RGB value by looking only at the direct 4 neighbor pixels (N S E W)

//static inline
static __attribute__ ((noinline))
uint32_t CTI_NeighborPredict(
                         CTI_Struct & ctiStruct,
                         const bool tablePred,
                         int centerX,
                         int centerY)
{
  const bool debug = false;
  
  const int width = ctiStruct.width;
  const int height = ctiStruct.height;
  
  const uint32_t * const pixelsPtr = ctiStruct.pixelsPtr;
  const uint32_t * const colortablePixelsPtr = ctiStruct.colortablePixelsPtr;
  const uint8_t * const tableOffsetsPtr = ctiStruct.tableOffsetsPtr;
  
  if (debug) {
    printf("CTI_NeighborPredict(%d,%d)\n", centerX, centerY);
  }

#if defined(DEBUG)
  assert(centerX >= 0);
  assert(centerX < width);
  
  assert(centerY >= 0);
  assert(centerY < height);
#endif // DEBUG
  
  int centerOffset = CTIOffset2d(centerX, centerY, width);
  
#if defined(DEBUG)
  // Note that C processed flag is ignored. It can be marked as
  // processed but that would be ignored.
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
#endif // DEBUG

  int sumHR = 0, sumHG = 0, sumHB = 0;
  int numH = 0;
  int sumVR = 0, sumVG = 0, sumVB = 0;
  int numV = 0;
  
  // U
  
  {
    int col = centerX;
    int row = centerY - 1;
    
    bool pixelWasProcessed = false;
    if (row >= 0) {
      pixelWasProcessed = ctiStruct.wasProcessed(col, row);
    }
    
    if (pixelWasProcessed) {
      int offset = centerOffset - width;
      
      uint32_t pixel;
      if (tablePred) {
        pixel = colortablePixelsPtr[tableOffsetsPtr[offset]];
      } else {
        pixel = pixelsPtr[offset];
      }
      
      if (debug) {
        printf("U pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
      }
      
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      numV += 1;
      
      sumVB += B;
      sumVG += G;
      sumVR += R;
    }
  }
  
  // L
  
  {
    int col = centerX - 1;
    int row = centerY;
    
    bool pixelWasProcessed = false;
    if (col >= 0) {
      pixelWasProcessed = ctiStruct.wasProcessed(col, row);
    }

    if (pixelWasProcessed) {
      int offset = centerOffset - 1;
      
      uint32_t pixel;
      if (tablePred) {
        pixel = colortablePixelsPtr[tableOffsetsPtr[offset]];
      } else {
        pixel = pixelsPtr[offset];
      }
      
      if (debug) {
        printf("L pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
      }
      
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      numH += 1;
      
      sumHB += B;
      sumHG += G;
      sumHR += R;
    }
  }

  // R
  
  {
    int col = centerX + 1;
    int row = centerY;
    
    bool pixelWasProcessed = false;
    if (col < width) {
      pixelWasProcessed = ctiStruct.wasProcessed(col, row);
    }
    
    if (pixelWasProcessed) {
      int offset = centerOffset + 1;
      
      uint32_t pixel;
      if (tablePred) {
        pixel = colortablePixelsPtr[tableOffsetsPtr[offset]];
      } else {
        pixel = pixelsPtr[offset];
      }
      
      if (debug) {
        printf("R pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
      }
      
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      numH += 1;
      
      sumHB += B;
      sumHG += G;
      sumHR += R;
    }
  }

  // D
  
  {
    int col = centerX;
    int row = centerY + 1;
    
    bool pixelWasProcessed = false;
    if (row < height) {
      pixelWasProcessed = ctiStruct.wasProcessed(col, row);
    }
    
    if (pixelWasProcessed) {
      int offset = centerOffset + width;
      
      uint32_t pixel;
      if (tablePred) {
        pixel = colortablePixelsPtr[tableOffsetsPtr[offset]];
      } else {
        pixel = pixelsPtr[offset];
      }
      
      if (debug) {
        printf("D pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
      }
      
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      numV += 1;
      
      sumVB += B;
      sumVG += G;
      sumVR += R;
    }
  }

#if defined(DEBUG)
  if (numH == 0 && numV == 0) {
    assert(0);
  }
  assert(numH <= 2);
  assert(numV <= 2);
#endif // DEBUG
  
  // H is 0, 1, or 2 values
  
  if (numH == 0) {
  } else if (numH == 1) {
    // nop
  } else if (numH == 2) {
    // ave
    sumHR = fast_div_2(sumHR);
    sumHG = fast_div_2(sumHG);
    sumHB = fast_div_2(sumHB);
  }
  
  // V is 0, 1, or 2 values

  if (numV == 0) {
  } else if (numV == 1) {
    // nop
  } else if (numV == 2) {
    // ave
    sumVR = fast_div_2(sumVR);
    sumVG = fast_div_2(sumVG);
    sumVB = fast_div_2(sumVB);
  }

  uint32_t R, G, B;
  
  if (numH == 0) {
    // Use just the V pred
    R = sumVR;
    G = sumVG;
    B = sumVB;
  } else if (numV == 0) {
    // Use just the H pred
    R = sumHR;
    G = sumHG;
    B = sumHB;
  } else {
    // Combine ave for H and V
#if defined(DEBUG)
    assert(sumHR != -1);
    assert(sumVR != -1);
    assert(sumHG != -1);
    assert(sumVG != -1);
    assert(sumHB != -1);
    assert(sumVB != -1);
#endif // DEBUG
    
    R = fast_ave_2(sumHR, sumVR);
    G = fast_ave_2(sumHG, sumVG);
    B = fast_ave_2(sumHB, sumVB);
  }
  
#if defined(DEBUG)
  assert(R <= 0xFF);
  assert(G <= 0xFF);
  assert(B <= 0xFF);
#endif // DEBUG
  
  return (R << 16) | (G << 8) | B;
}

// Predict a RGB value by looking at the direct 4 neighbor pixels (N S E W)
// and the pred errors for these pixels

//static inline
static __attribute__ ((noinline))
uint32_t CTI_NeighborPredict2(
                              CTI_Struct & ctiStruct,
                              const bool tablePred,
                              uint32_t * const predErrPtr,
                              int centerX,
                              int centerY)
{
  const bool debug = false;
  
  const int width = ctiStruct.width;
  const int height = ctiStruct.height;
  
  if (debug) {
    printf("CTI_NeighborPredict2(%d,%d)\n", centerX, centerY);
  }
  
#if defined(DEBUG)
  assert(centerX >= 0);
  assert(centerX < width);
  
  assert(centerY >= 0);
  assert(centerY < height);
#endif // DEBUG
  
  int centerOffset = CTIOffset2d(centerX, centerY, width);
  
#if defined(DEBUG)
  // Note that C processed flag is ignored. It can be marked as
  // processed but that would be ignored.
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
#endif // DEBUG
  
  // Gather info about which of neighbors is set and save
  // into an unsigned register so that it is easy to
  // access the specific state as a bit.
  
  typedef struct {
    unsigned int UL : 1;
    unsigned int U : 1;
    unsigned int UR : 1;
    unsigned int L : 1;
    unsigned int R : 1;
    unsigned int DL : 1;
    unsigned int D : 1;
    unsigned int DR : 1;
  } NeighborBits;
  
  NeighborBits nBits;
  memset(&nBits, 0, sizeof(NeighborBits));
  
  // UL
  
  {
    int col = centerX - 1;
    int row = centerY - 1;
    
    if ((col >= 0) && (row >= 0)) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.UL = true;
      }
    }
  }

  // U
  
  {
    int col = centerX;
    int row = centerY - 1;
    
    if (row >= 0) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.U = true;
      }
    }
  }

  // UR
  
  {
    int col = centerX + 1;
    int row = centerY - 1;
    
    if ((col < width) && (row >= 0)) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.UR = true;
      }
    }
  }

  // L
  
  {
    int col = centerX - 1;
    int row = centerY;
    
    if (col >= 0) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.L = true;
      }
    }
  }

  // R
  
  {
    int col = centerX + 1;
    int row = centerY;
    
    if (col < width) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.R = true;
      }
    }
  }
  
  // DL
  
  {
    int col = centerX - 1;
    int row = centerY + 1;
    
    if ((col >= 0) && (row < height)) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.DL = true;
      }
    }
  }
  
  // D
  
  {
    int col = centerX;
    int row = centerY + 1;
    
    if (row < height) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.D = true;
      }
    }
  }
  
  // DR
  
  {
    int col = centerX + 1;
    int row = centerY + 1;
    
    if ((col < width) && (row < height)) {
      if (ctiStruct.wasProcessed(col, row)) {
        nBits.DR = true;
      }
    }
  }
  
  if (debug) {
    // Print direction was processed flags in logical way
    
    printf("NeighborBits:\n");
    printf("%d %d %d\n", nBits.UL, nBits.U, nBits.UR);
    printf("%d x %d\n", nBits.L, nBits.R);
    printf("%d %d %d\n", nBits.DL, nBits.D, nBits.DR);
    
    if ((1)) {
      // Print the pixel at each NeighborBits that is defined

      unsigned int pUL = 0xFFFFFFFF, pU = 0xFFFFFFFF, pUR = 0xFFFFFFFF, pL = 0xFFFFFFFF, pR = 0xFFFFFFFF, pDL = 0xFFFFFFFF, pD = 0xFFFFFFFF, pDR = 0xFFFFFFFF;
      
      if (nBits.UL) {
      pUL = ctiStruct.pixelLookup(tablePred, centerOffset - width - 1);
      }

      if (nBits.U) {
      pU = ctiStruct.pixelLookup(tablePred, centerOffset - width);
      }
      
      if (nBits.UR) {
      pUR = ctiStruct.pixelLookup(tablePred, centerOffset - width + 1);
      }
      
      if (nBits.L) {
      pL = ctiStruct.pixelLookup(tablePred, centerOffset - 1);
      }

      if (nBits.R) {
      pR = ctiStruct.pixelLookup(tablePred, centerOffset + 1);
      }
      
      if (nBits.DL) {
      pDL = ctiStruct.pixelLookup(tablePred, centerOffset + width - 1);
      }
      
      if (nBits.D) {
      pD = ctiStruct.pixelLookup(tablePred, centerOffset + width);
      }
      
      if (nBits.DR) {
      pDR = ctiStruct.pixelLookup(tablePred, centerOffset + width + 1);
      }
      
      printf("0x%08X 0x%08X 0x%08X\n", pUL, pU, pUR);
      printf("0x%08X 0xFFFFFFFF 0x%08X\n", pL, pR);
      printf("0x%08X 0x%08X 0x%08X\n", pDL, pD, pDR);
      printf("\n");
    }
  }

  // Combine components into a pixel and check valid range
  
  auto combinePixelComponents = [](unsigned int R, unsigned int G, unsigned int B)->unsigned int
  {
#if defined(DEBUG)
    assert(R <= 0xFF);
    assert(G <= 0xFF);
    assert(B <= 0xFF);
#endif // DEBUG
    
    return (R << 16) | (G << 8) | B;
  };

  // Return the pixel component that has the smaller delta.
  
  auto aveComponent = [](
                                  unsigned int p1,
                                  unsigned int p2,
                                  const int shiftR)->unsigned int
  {
    const bool debug = false;
    
    if (debug) {
      unsigned int c1 = (p1 >> shiftR) & 0xFF;
      unsigned int c2 = (p2 >> shiftR) & 0xFF;
      
      printf("aveComponent c1 c2 : %d %d\n", c1, c2);
    }
    
    unsigned int c1 = (p1 >> shiftR) & 0xFF;
    unsigned int c2 = (p2 >> shiftR) & 0xFF;
    
    return fast_ave_2(c1, c2);
  };
  
  // Return the pixel component that has the smaller delta.
  
  auto smallerDeltaComponent = [](unsigned int dH,
                                  unsigned int dV,
                                  unsigned int pU,
                                  unsigned int pD,
                                  unsigned int pL,
                                  unsigned int pR,
                                  const int shiftR)->unsigned int
  {
    const bool debug = false;
    
    if (debug) {
      unsigned int cL = (pL >> shiftR) & 0xFF;
      unsigned int cR = (pR >> shiftR) & 0xFF;
      
      unsigned int cU = (pU >> shiftR) & 0xFF;
      unsigned int cD = (pD >> shiftR) & 0xFF;
      
      printf("smallerDeltaComponent dH dV (%d %d) : cU cD cL cR : %d %d %d %d\n", dH, dV, cU, cD, cL, cR);
    
      int dHS = unsigned_byte_to_signed(dH);
      int dVS = unsigned_byte_to_signed(dV);
      
      printf("dH dV (%d <= %d) : (%d <= %d) : (%d <= %d) : cond %d\n", dH, dV, dHS, dVS, abs(dHS), abs(dVS), (abs(dHS) <= abs(dVS)) ? true : false);
      
      printf("dHS dVS %d %d\n", dHS, dVS);
    }
    
#if defined(DEBUG)
    {
      int dHS = unsigned_byte_to_signed(dH);
      int dVS = unsigned_byte_to_signed(dV);
      
      int absH = abs((int8_t)dH);
      int absV = abs((int8_t)dV);
      
      int cond1 = (absH <= absV);
      int cond2 = (abs(dHS) <= abs(dVS));
      assert(cond1 == cond2);      
    }
#endif // DEBUG
    
    if (abs((int8_t)dH) <= abs((int8_t)dV)) {
      // H delta is smaller (or equal to) than V delta, so return ave(L, R)
      
      unsigned int cL = (pL >> shiftR) & 0xFF;
      unsigned int cR = (pR >> shiftR) & 0xFF;
      
      if (debug) {
        printf("H delta is smaller (%d %d) -> ave %d\n", cL, cR, fast_ave_2(cL, cR));
      }
      
      return fast_ave_2(cL, cR);
    } else {
      // V delta is smaller H delta, so return ave(U, D)
      
      unsigned int cU = (pU >> shiftR) & 0xFF;
      unsigned int cD = (pD >> shiftR) & 0xFF;
      
      if (debug) {
        printf("V delta is smaller (%d %d) -> ave %d\n", cU, cD, fast_ave_2(cU, cD));
      }
      
      return fast_ave_2(cU, cD);
    }
  };
  
  // Choose component with the smaller delta and return (R G B) that is the min
  
  auto chooseSmallerComponent = [smallerDeltaComponent](
                          unsigned int pU,
                          unsigned int pD,
                          unsigned int pL,
                          unsigned int pR,
                          unsigned int dV,
                          unsigned int dH)->unsigned int {
    const bool debug = false;
    
    if (debug) {
      printf("chooseSmallerComponent (dV -> dH) 0x%08X -> 0x%08X\n", dV, dH);
    }
    
    unsigned int dVB = dV & 0xFF;
    unsigned int dVG = (dV >> 8) & 0xFF;
    unsigned int dVR = (dV >> 16) & 0xFF;
    
    unsigned int dHB = dH & 0xFF;
    unsigned int dHG = (dH >> 8) & 0xFF;
    unsigned int dHR = (dH >> 16) & 0xFF;
    
    if (debug) {
      printf("H (dR, dG, dB) = (%d %d %d)\n", dHR, dHG, dHB);
      printf("V (dR, dG, dB) = (%d %d %d)\n", dVR, dVG, dVB);
    }
    
    // Choose the component with the smaller delta
    
    unsigned int B = smallerDeltaComponent(dHB, dVB, pU, pD, pL, pR, 0);
    unsigned int G = smallerDeltaComponent(dHG, dVG, pU, pD, pL, pR, 8);
    unsigned int R = smallerDeltaComponent(dHR, dVR, pU, pD, pL, pR, 16);

    if (debug) {
      printf("min (dV -> dH) : %d %d %d\n", R, G, B);
    }
    
#if defined(DEBUG)
    assert(R <= 0xFF);
    assert(G <= 0xFF);
    assert(B <= 0xFF);
#endif // DEBUG
    
    unsigned int minPixel = (R << 16) | (G << 8) | B;
    
    if (debug) {
      printf("min (dV -> dH) : 0x%08X\n", minPixel);
    }
    
    return minPixel;
  };
  
  unsigned int retPixel = 0;
    
  // If both an H prediction and a V prediction is available, then
  // compute the deltas and use the smallest delta.
  
  bool hasH = (nBits.L && nBits.R);
  bool hasV = (nBits.U && nBits.D);
  
  if (hasH && hasV) {
    // Compute the delta between L and R and choose component ave that is the smallest

    unsigned int pU = ctiStruct.pixelLookup(tablePred, centerOffset - width);
    unsigned int pL = ctiStruct.pixelLookup(tablePred, centerOffset - 1);
    unsigned int pR = ctiStruct.pixelLookup(tablePred, centerOffset + 1);
    unsigned int pD = ctiStruct.pixelLookup(tablePred, centerOffset + width);

    if (debug) {
      printf("L -> R : 0x%08X -> 0x%08X\n", pL, pR);
      printf("U -> D : 0x%08X -> 0x%08X\n", pU, pD);
    }
    
    unsigned int deltaUD = ctiStruct.componentDelta(pU, pD);
    unsigned int deltaLR = ctiStruct.componentDelta(pL, pR);
    
    if (debug) {
      printf("L - R delta : 0x%08X\n", deltaLR);
      printf("U - D delta : 0x%08X\n", deltaUD);
    }
    
    // Choose between H or V delta
    
    retPixel = chooseSmallerComponent(pU, pD, pL, pR, deltaUD, deltaLR);
  } else if (hasH) {
    // H
    
    unsigned int pL = ctiStruct.pixelLookup(tablePred, centerOffset - 1);
    unsigned int pR = ctiStruct.pixelLookup(tablePred, centerOffset + 1);
    
    if (debug) {
      printf("L -> R : 0x%08X -> 0x%08X\n", pL, pR);
    }
    
    unsigned int B = aveComponent(pL, pR, 0);
    unsigned int G = aveComponent(pL, pR, 8);
    unsigned int R = aveComponent(pL, pR, 16);
    
    if (debug) {
      printf("H ave (R G B) (%d %d %d)\n", R, G, B);
    }
    
    retPixel = combinePixelComponents(R, G, B);
  } else if (hasV) {
    // V
    
    unsigned int pU = ctiStruct.pixelLookup(tablePred, centerOffset - width);
    unsigned int pD = ctiStruct.pixelLookup(tablePred, centerOffset + width);
    
    if (debug) {
      printf("U -> D : 0x%08X -> 0x%08X\n", pU, pD);
    }
    
    unsigned int B = aveComponent(pU, pD, 0);
    unsigned int G = aveComponent(pU, pD, 8);
    unsigned int R = aveComponent(pU, pD, 16);
    
    if (debug) {
      printf("V ave (R G B) (%d %d %d)\n", R, G, B);
    }
    
    retPixel = combinePixelComponents(R, G, B);
  } else if (nBits.L && nBits.U && nBits.UL) {
    // Trivial gradclamp prediction case like:
    //
    // 0 10 ?
    // 0  X ?
    // ?  ? ?
    
    unsigned int pUL = ctiStruct.pixelLookup(tablePred, centerOffset - width - 1);
    unsigned int pU = ctiStruct.pixelLookup(tablePred, centerOffset - width);
    unsigned int pL = ctiStruct.pixelLookup(tablePred, centerOffset - 1);
    
    uint32_t samples[4];
    
    samples[0] = pUL;
    samples[1] = pU;
    samples[2] = pL;
    samples[3] = 0; // use zero so that pred err will always be the pred value
    
    if (debug) {
      printf("gradclamp:\n");
      printf("0x%08X 0x%08X:\n", pUL, pU);
      printf("0x%08X X:\n", pL);
    }
    
    uint32_t predPixel = gradclamp8by4(samples, 2, 3);
    
    if (debug) {
      printf("predPixel 0x%08X X:\n", predPixel);
    }
    
    retPixel = predPixel;
  } else if (nBits.L && nBits.D && nBits.DL) {
    // Trivial gradclamp prediction case like:
    //
    // ?  ? ?
    // 0  X ?
    // 0 10 ?
    
    unsigned int pL = ctiStruct.pixelLookup(tablePred, centerOffset - 1);
    unsigned int pDL = ctiStruct.pixelLookup(tablePred, centerOffset + width - 1);
    unsigned int pD = ctiStruct.pixelLookup(tablePred, centerOffset + width);
    
    uint32_t samples[4];
    
    // Input:
    // C B
    // A X
    
    samples[0] = pDL;
    samples[1] = pD;
    samples[2] = pL;
    samples[3] = 0;
    
    if (debug) {
      printf("gradclamp:\n");
      printf("0x%08X 0x%08X:\n", samples[0], samples[1]);
      printf("0x%08X X:\n", samples[2]);
    }
    
    uint32_t predPixel = gradclamp8by4(samples, 2, 3);
    
    if (debug) {
      printf("predPixel 0x%08X X:\n", predPixel);
    }
    
    retPixel = predPixel;
  } else if (nBits.R && nBits.U && nBits.UR) {
    // Trivial gradclamp prediction case like:
    //
    // ?  0  0
    // ?  X 10
    // ?  ?  ?
    
    unsigned int pU = ctiStruct.pixelLookup(tablePred, centerOffset - width);
    unsigned int pUR = ctiStruct.pixelLookup(tablePred, centerOffset - width + 1);
    unsigned int pR = ctiStruct.pixelLookup(tablePred, centerOffset + 1);
    
    uint32_t samples[4];
    
    // Input:
    // C B
    // A X
    
    samples[0] = pUR;
    samples[1] = pU;
    samples[2] = pR;
    samples[3] = 0;
    
    if (debug) {
      printf("gradclamp:\n");
      printf("0x%08X 0x%08X:\n", samples[0], samples[1]);
      printf("0x%08X X:\n", samples[2]);
    }
    
    uint32_t predPixel = gradclamp8by4(samples, 2, 3);
    
    if (debug) {
      printf("predPixel 0x%08X X:\n", predPixel);
    }
    
    retPixel = predPixel;
  } else if (nBits.R && nBits.D && nBits.DR) {
    // Trivial gradclamp prediction case like:
    //
    // ?  ?  ?
    // ?  X 10
    // ?  0  0
    
    unsigned int pR = ctiStruct.pixelLookup(tablePred, centerOffset + 1);
    unsigned int pD = ctiStruct.pixelLookup(tablePred, centerOffset + width);
    unsigned int pDR = ctiStruct.pixelLookup(tablePred, centerOffset + width + 1);
    
    uint32_t samples[4];
    
    // Input:
    // C B
    // A X
    
    samples[0] = pDR;
    samples[1] = pD;
    samples[2] = pR;
    samples[3] = 0;
    
    if (debug) {
      printf("gradclamp:\n");
      printf("0x%08X 0x%08X:\n", samples[0], samples[1]);
      printf("0x%08X X:\n", samples[2]);
    }
    
    uint32_t predPixel = gradclamp8by4(samples, 2, 3);
    
    if (debug) {
      printf("predPixel 0x%08X X:\n", predPixel);
    }
    
    retPixel = predPixel;
  } else {
    // No H or V primary, calculate ave of H and V
    
    return CTI_NeighborPredict(ctiStruct, tablePred, centerX, centerY);
  }
  
  return retPixel;
}

// Predict a RGB value for a pixel given an input box
// that is oriented so that the horizontal delta through
// the values is summed a row at a time. This logic assumes
// that the caller has defined the inputValues pointer
// so that a fixed maximum "width" is defined as the first
// int value. After that, a N that indicates how many
// values appear in a specific row where N < width. The
// number (rowOffEnd-rowOffStart) value indicates how many
// row appear in the input values.

//static inline
static __attribute__ ((noinline))
uint32_t CTI_BoxPredictPixel(
                             CTI_Struct & ctiStruct,
                             const bool tablePred,
                             int centerX,
                             int centerY,
                             uint32_t * inputValues,
                             int rowOffStart,
                             int rowOffEnd)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_BoxPredictPixel (%d,%d) with rowOff (%d,%d)\n", centerX, centerY, rowOffStart, rowOffEnd);
  }
  
  // Horizontal 4 x 5 box where the center coordinate
  // is the pixel to be predicted.
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 2 2 2 (sumU2)
  // 1 1 1 1 (sumU1)
  // 0 0 X 0 (sum0)
  // 1 1 1 1 (sumD1)
  // 2 2 2 2 (sumD2)
  
  int sumU2R = -1, sumU2G = -1, sumU2B = -1;
  int sumU1R = -1, sumU1G = -1, sumU1B = -1;
  int sum0R  = -1, sum0G  = -1, sum0B  = -1;
  int sumD1R = -1, sumD1G = -1, sumD1B = -1;
  int sumD2R = -1, sumD2G = -1, sumD2B = -1;
  
  // Read width of each row (in pixels)
  uint32_t * inputValuesPtr = (uint32_t*) &inputValues[0];
  const int rowWidth = *inputValuesPtr++;
  
  for ( int rowOff = rowOffStart; rowOff < rowOffEnd; rowOff++ ) {
    int N = *inputValuesPtr++; // num elements to read in row
    
#if defined(DEBUG)
    assert(N <= (rowWidth-1));
#endif // DEBUG
    
#if defined(DEBUG)
    if (debug) {
      printf("rowOff %d contains = %d pixels\n", rowOff, N);
    }
#endif // DEBUG
    
    int rowR = 0;
    int rowG = 0;
    int rowB = 0;
    
    for ( int col = 0; col < N; col++ ) {
      uint32_t pixel = *inputValuesPtr++;

#if defined(DEBUG)
      assert(pixel != 0xFFFFFFFF);
#endif // DEBUG
      
#if defined(DEBUG)
      if (debug) {
        printf("pixel (%4d,%4d) = 0x%08X\n", col, rowOff, pixel);
      }
#endif // DEBUG

      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      if (debug) {
        printf("pixel = (R G B) (%d %d %d)\n", R, G, B);
      }
      
      rowB += B;
      rowG += G;
      rowR += R;
    }
    
    // Advance inputValuesPtr to end of row
    inputValuesPtr += (rowWidth - 1 - N);
    
    if (N == 0) {
      // Skip row processing when no values in this row. All values -1
      
      continue; // go to next row
    }
    
    // Generate ave for 1,2,3,4 pixels in this row
    
    switch (N) {
      case 1: {
        // Nop
        break;
      }
      case 2: {
        // Ave of 2 elements
        rowR = fast_div_2(rowR);
        rowG = fast_div_2(rowG);
        rowB = fast_div_2(rowB);
        break;
      }
      case 3: {
        // Efficient N / 3
        rowR = fast_div_3(rowR);
        rowG = fast_div_3(rowG);
        rowB = fast_div_3(rowB);
        break;
      }
      case 4: {
        // Efficient N / 4
        rowR = fast_div_4(rowR);
        rowG = fast_div_4(rowG);
        rowB = fast_div_4(rowB);
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    // Set ave value for each elem in this row
    
    switch (rowOff) {
      case -2: {
        sumU2R = rowR;
        sumU2G = rowG;
        sumU2B = rowB;
        break;
      }
      case -1: {
        sumU1R = rowR;
        sumU1G = rowG;
        sumU1B = rowB;
        break;
      }
      case 0: {
        sum0R = rowR;
        sum0G = rowG;
        sum0B = rowB;
        break;
      }
      case 1: {
        sumD1R = rowR;
        sumD1G = rowG;
        sumD1B = rowB;
        break;
      }
      case 2: {
        sumD2R = rowR;
        sumD2G = rowG;
        sumD2B = rowB;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
  }
  
  if (debug) {
    printf("cached sumU2 = (R G B) (%d %d %d)\n", sumU2R, sumU2G, sumU2B);
    printf("cached sumU1 = (R G B) (%d %d %d)\n", sumU1R, sumU1G, sumU1B);
    printf("cached sum0  = (R G B) (%d %d %d)\n", sum0R, sum0G, sum0B);
    printf("cached sumD1 = (R G B) (%d %d %d)\n", sumD1R, sumD1G, sumD1B);
    printf("cached sumD2 = (R G B) (%d %d %d)\n", sumD2R, sumD2G, sumD2B);
  }
  
  // Calc ave for D1 and U1
  
  int ave1R = average_012(sumD1R, sumU1R);
  int ave1G = average_012(sumD1G, sumU1G);
  int ave1B = average_012(sumD1B, sumU1B);
  
  // Calc ave for D2 and U2
  
  int ave2R = average_012(sumD2R, sumU2R);
  int ave2G = average_012(sumD2G, sumU2G);
  int ave2B = average_012(sumD2B, sumU2B);
  
  if (debug) {
    printf("ave sum0 = (R G B) (%d %d %d)\n", sum0R, sum0G, sum0B);
    printf("ave sum1 = (R G B) (%d %d %d)\n", ave1R, ave1G, ave1B);
    printf("ave sum2 = (R G B) (%d %d %d)\n", ave2R, ave2G, ave2B);
  }
  
#if defined(DEBUG)
  assert(sum0R != -1);
#endif // DEBUG
  
  // The final average is a weighted sum of the 3 row sums.
  // Approx: sum(100% + 50% + 25%)
  
  unsigned int wR;
  unsigned int wG;
  unsigned int wB;
  
  if (ave1R == -1 && ave2R == -1) {
    // no average needed
    
    if (debug) {
      printf("no weighted calc\n");
    }
    
    wR = sum0R;
    wG = sum0G;
    wB = sum0B;
  } else if (ave1R != -1 && ave2R != -1) {
    // Weighted Ave (0,1,2) = 16/32 (50%) + 10/32 (31%) + 6/32 (19%)
    
    wR = (sum0R * 16) + (ave1R * 10) + (ave2R * 6);
    wG = (sum0G * 16) + (ave1G * 10) + (ave2G * 6);
    wB = (sum0B * 16) + (ave1B * 10) + (ave2B * 6);
    
    if (debug) {
      printf("weighted (0,1,2) calc\n");
    }
    
    // FIXME: translate to shifts
    
    wR /= 32;
    wG /= 32;
    wB /= 32;
  } else if (ave1R == -1) {
    // Weighted Ave (0,X,2) = 24/32 (75%) + 8/32 (25%)
    
    wR = (sum0R * 24) + (ave2R * 8);
    wG = (sum0G * 24) + (ave2G * 8);
    wB = (sum0B * 24) + (ave2B * 8);
    
    if (debug) {
      printf("weighted (0,X,2) calc\n");
    }
    
    wR /= 32;
    wG /= 32;
    wB /= 32;
  } else if (ave2R == -1) {
    // Weighted Ave (0,1,X) = 21/32 (65%) + 11/32 (35%)
    
    if (debug) {
      printf("weighted (0,1,X) calc\n");
    }
    
    wR = (sum0R * 21) + (ave1R * 11);
    wG = (sum0G * 21) + (ave1G * 11);
    wB = (sum0B * 21) + (ave1B * 11);
    
    // FIXME: translate to shifts
    
    wR /= 32;
    wG /= 32;
    wB /= 32;
  } else {
    assert(0);
  }
  
  if (debug) {
    printf("weighted sum (R G B) (%d %d %d)\n", wR, wG, wB);
  }
  
#if defined(DEBUG)
  assert(wR <= 0xFF);
  assert(wG <= 0xFF);
  assert(wB <= 0xFF);
#endif // DEBUG
  
  return (wR << 16) | (wG << 8) | (wB);
}

// Predict the RGB value of a pixel by generating an average
// of the nearby pixels in a 4 x 5 box around the center pixel.

//static inline
static __attribute__ ((noinline))
uint32_t CTI_BoxPredictH(
                                  CTI_Struct & ctiStruct,
                                  const bool tablePred,
                                  int centerX,
                                  int centerY)
{
  const bool debug = false;
  const bool debugDumpInputPixels = false;
 
  const int width = ctiStruct.width;
  const int height = ctiStruct.height;
  
  if (debug) {
    printf("CTI_BoxPredictH(%d,%d)\n", centerX, centerY);
  }
  
  int centerOffset = CTIOffset2d(centerX, centerY, width);

#if defined(DEBUG)
  // C must not be processed
  
  assert(centerX >= 0);
  assert(centerX < width);
  
  assert(centerY >= 0);
  assert(centerY < height);

  // Note that the center coord is marked as processed when this
  // method is invoked, but it should not be seen as processed,
  // by the logic that loops over pixel values, so turn it off
  // for the moment and then turn it back on before returning.
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed == false);
  }
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
  
  // The 2 pixels to the left of (cx, cy) must be defined
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX - 1, centerY);
    assert(pixelWasProcessed == true);
  }
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX - 2, centerY);
    assert(pixelWasProcessed == true);
  }
#endif // DEBUG
  
  // Horizontal 4 x 5 box where the center coordinate
  // is the pixel to be predicted.

  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 2 2 2 (sumU2)
  // 1 1 1 1 (sumU1)
  // 0 0 X 0 (sum0)
  // 1 1 1 1 (sumD1)
  // 2 2 2 2 (sumD2)
  
  // Upper left corner of box is at (-2, -2). Note that since deltas are
  // calculated in terms of pairs of pixels the row 0 corresponds to
  // the difference between rows 0 and 1.
  
  const int boxWidth = 4;
  const int boxHalfWidth = (boxWidth - 1) / 2;
  
  const int boxHeight = 5;
  const int boxHalfHeight = (boxHeight - 1) / 2;
  
  int originX = (centerX - boxHalfWidth);
  int originY = (centerY - boxHalfHeight);
  
  // Adjust X origin to the left to account for center
  originX -= 1;
  
  int maxX = originX + (boxWidth - 1); // X + 2 inclusive indicates width of 5
  int maxY = originY + (boxHeight - 1); // Y + 4 inclusive indicates height of 3
  
  // rowOff is a counter that indicates where the row is relative to the
  // center coordinate. This is used to determine the pixel diff weight.
  
  int rowOff = -2;
  
  if (originX < 0) {
    originX = 0;
  }
  if (originY < 0) {
    if (originY == -2) {
      rowOff = 0;
    } else if (originY == -1) {
      rowOff = -1;
    }
    
    originY = 0;
  }
  
  if (maxX < (width-1)) {
    // nop
  } else {
    maxX = (width-1);
  }
  if (maxY < (height-1)) {
    // nop
  } else {
    maxY = (height-1);
  }
  
  int originOffset = CTIOffset2d(originX, originY, width);
  
  if (debug) {
    printf("ORIGIN  (%4d,%4d) %d\n", originX, originY, originOffset);
    printf("W x H  : %d x %d\n", maxX-originX+1, maxY-originY+1);
    printf("MAX     (%4d,%4d) %d\n", maxX, maxY, CTIOffset2d(maxX, maxY, width));
    printf("rowOff   %d\n", rowOff);
  }
  
  int rowOffStart = rowOff;
  
#if defined(DEBUG)
  int inputWidth = maxX-originX+1;
  int inputHeight = maxY-originY+1;
  vector<vector<uint32_t> > inputPixels;

  if (debugDumpInputPixels) {
    inputPixels = vector<vector<uint32_t> >(inputHeight);
    
    for (int i = 0; i < inputHeight; i++ ) {
      inputPixels[i] = vector<uint32_t>(inputWidth);
      
      for (int j = 0; j < inputWidth; j++ ) {
        inputPixels[i][j] = 0xFFFFFFFF;
      }
    }
  }
#endif // DEBUG
  
  // Stack allocated buffer of known fixed size
  
  const int rowWidth = 4 + 1; // N + 4 pixels
  const int maxNumRows = 5;
  uint32_t inputPixelsBuffer[1 + maxNumRows * rowWidth];
  memset(inputPixelsBuffer, 0, sizeof(inputPixelsBuffer));
  
  uint32_t *inputPixelsPtr = &inputPixelsBuffer[0];
  *inputPixelsPtr++ = rowWidth;
  
  const unsigned int widthMinusX = width - (maxX - originX + 1);
  
  unsigned int offset = originOffset;
  
  for ( int row = originY; row <= maxY; row++ ) {
    uint32_t *nPtr = inputPixelsPtr++;
    
    unsigned int N = 0;
    
    for ( int col = originX; col <= maxX; col++ ) {
      
      if (0 && debug) {
        printf("cache coord (%4d,%4d) = %d\n", col, row, offset);
      }
      
#if defined(DEBUG)
      {
        int calcOffset = CTIOffset2d(col, row, width);
        assert(offset == calcOffset);
      }
#endif // DEBUG
      
      bool pixelWasProcessed = ctiStruct.wasProcessed(col, row);
      
      if (pixelWasProcessed) {
#if defined(DEBUG)
        assert(offset != centerOffset);
#endif // DEBUG
        
        uint32_t pixel = ctiStruct.pixelLookup(tablePred, offset);
        
        if (debug) {
          printf("pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
        }
        
#if defined(DEBUG)
        // Place in vector location so that default value is used
        // for a slot that is not set.
        if (debugDumpInputPixels) {
          assert((row-originY) < inputPixels.size());
          vector<uint32_t> &rowVec = inputPixels[row-originY];
          assert((col-originX) < rowVec.size());
          rowVec[col-originX] = pixel;
        }
#endif // DEBUG
        
        *inputPixelsPtr++ = pixel & 0x00FFFFFF;
        N++;
      }
      
      offset += 1;
    } // end foreach col in current row
    
    if (debug) {
      printf("write N = %d for row %d\n", N, rowOff);
    }
    *nPtr = N;
    
    // Skip to start of next row
    inputPixelsPtr += (rowWidth - 1 - N);
    
    offset += widthMinusX;
    rowOff += 1;
  }
  
  // Debug input pixel values from row and col read logic
  
#if defined(DEBUG)
  if (debugDumpInputPixels) {
    for ( auto & rowVec : inputPixels ) {
      for ( auto pixel : rowVec ) {
        printf("0x%08X ", pixel);
      }
      printf("\n");
    }
  }
#endif // DEBUG
  
  return CTI_BoxPredictPixel(ctiStruct,
                             tablePred,
                             centerX,
                             centerY,
                             inputPixelsBuffer,
                             rowOffStart,
                             rowOff);
}

// Predict the RGB value of a pixel by generating an average
// of the nearby pixels in a 4 x 5 box around the center pixel.

//static inline
static __attribute__ ((noinline))
uint32_t CTI_BoxPredictV(
                         CTI_Struct & ctiStruct,
                         const bool tablePred,
                         int centerX,
                         int centerY)
{
  const bool debug = false;
  const bool debugDumpInputPixels = false;
  
  const int width = ctiStruct.width;
  const int height = ctiStruct.height;
  
  if (debug) {
    printf("CTI_BoxPredictV(%d,%d)\n", centerX, centerY);
  }
  
  int centerOffset = CTIOffset2d(centerX, centerY, width);
  
#if defined(DEBUG)
  // C must not be processed
  
  assert(centerX >= 0);
  assert(centerX < width);
  
  assert(centerY >= 0);
  assert(centerY < height);
  
  // Note that the center coord is marked as processed when this
  // method is invoked, but it should not be seen as processed,
  // by the logic that loops over pixel values, so turn it off
  // for the moment and then turn it back on before returning.
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed == false);
  }
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
  
  // The 2 pixels above of (cx, cy) must be defined
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 1);
    assert(pixelWasProcessed == true);
  }
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 2);
    assert(pixelWasProcessed == true);
  }
#endif // DEBUG
  
  // Horizontal 4 x 5 box where the center coordinate
  // is the pixel to be predicted.
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // sumU2 - sumU1 - sum0 - sumD1 - sumD2
  //
  // 2 1 0 1 2
  // 2 1 0 1 2
  // 2 1 X 1 2
  // 2 1 0 1 2
  
  // Upper left corner of box is at (-2, -2). Note that since deltas are
  // calculated in terms of pairs of pixels the row 0 corresponds to
  // the difference between rows 0 and 1.
  
  const int boxWidth = 5;
  const int boxHalfWidth = (boxWidth - 1) / 2;
  
  const int boxHeight = 4;
  const int boxHalfHeight = (boxHeight - 1) / 2;
  
  int originX = (centerX - boxHalfWidth);
  int originY = (centerY - boxHalfHeight);
  
  // Adjust Y origin up to account for center
  originY -= 1;
  
  int maxX = originX + (boxWidth - 1); // X + 2 inclusive indicates width of 5
  int maxY = originY + (boxHeight - 1); // Y + 4 inclusive indicates height of 3
  
  // rowOff is a counter that indicates where the row is relative to the
  // center coordinate. This is used to determine the pixel diff weight.
  
  int colOff = -2;
  
  if (originX < 0) {
    if (originX == -2) {
      colOff = 0;
    } else if (originX == -1) {
      colOff = -1;
    }
    
    originX = 0;
  }
  if (originY < 0) {
    originY = 0;
  }
  
  if (maxX < (width-1)) {
    // nop
  } else {
    maxX = (width-1);
  }
  if (maxY < (height-1)) {
    // nop
  } else {
    maxY = (height-1);
  }
  
  int originOffset = CTIOffset2d(originX, originY, width);
  
  if (debug) {
    printf("ORIGIN  (%4d,%4d) %d\n", originX, originY, originOffset);
    printf("W x H  : %d x %d\n", maxX-originX+1, maxY-originY+1);
    printf("MAX     (%4d,%4d) %d\n", maxX, maxY, CTIOffset2d(maxX, maxY, width));
    printf("colOff   %d\n", colOff);
  }
  
  // Allocate H x W sized vector that will store column by column pixel data
  
  int colOffStart = colOff;
  
#if defined(DEBUG)
  int inputWidth = maxX-originX+1;
  int inputHeight = maxY-originY+1;
  vector<vector<uint32_t> > inputPixels;
  
  if (debugDumpInputPixels) {
    inputPixels = vector<vector<uint32_t> >(inputWidth);
    
    for (int i = 0; i < inputWidth; i++ ) {
      inputPixels[i] = vector<uint32_t>(inputHeight);
      
      for (int j = 0; j < inputHeight; j++ ) {
        inputPixels[i][j] = 0xFFFFFFFF; // Invalid since alpha is not supported
      }
    }
  }
#endif // DEBUG
  
  // Stack allocated buffer of known fixed size
  
  const int rowWidth = 4 + 1; // N + 4 pixels
  const int maxNumRows = 5;
  uint32_t inputPixelsBuffer[1 + maxNumRows * rowWidth];
  memset(inputPixelsBuffer, 0, sizeof(inputPixelsBuffer));
  
  uint32_t *inputPixelsRowPtrs[maxNumRows];
  uint32_t inputPixelsRowN[maxNumRows];
  
  memset(inputPixelsRowN, 0, sizeof(inputPixelsRowN));
  
  {
    uint32_t *bufferPtr = &inputPixelsBuffer[0];
    
    *bufferPtr++ = rowWidth;
    
    // Advance past N
    
    bufferPtr += 1;
    
    inputPixelsRowPtrs[0] = bufferPtr;
    bufferPtr += rowWidth;
    
    inputPixelsRowPtrs[1] = bufferPtr;
    bufferPtr += rowWidth;
    
    inputPixelsRowPtrs[2] = bufferPtr;
    bufferPtr += rowWidth;
    
    inputPixelsRowPtrs[3] = bufferPtr;
    bufferPtr += rowWidth;
    
    inputPixelsRowPtrs[4] = bufferPtr;
    bufferPtr += rowWidth;
  }
  
  for ( int col = originX; col <= maxX; col++ ) {
    
    // Transposed input means write all the values for
    // a specific column to the same output row.
    
#if defined(DEBUG)
    vector<uint32_t> *rowVecPtr = nullptr;
    if (debugDumpInputPixels) {
      assert((col-originX) < inputPixels.size());
      rowVecPtr = &inputPixels[col-originX];
    }
#endif // DEBUG
    
    int rowT0 = col-originX;
    
    for ( int row = originY; row <= maxY; row++ ) {
      unsigned int offset = CTIOffset2d(col, row, width);;
      
      if (1 && debug) {
        printf("cache coord (%4d,%4d) = %d\n", col, row, offset);
      }
      
      bool pixelWasProcessed = ctiStruct.wasProcessed(col, row);
      
      if (pixelWasProcessed) {
#if defined(DEBUG)
        assert(offset != centerOffset);
#endif // DEBUG
        
        uint32_t pixel = ctiStruct.pixelLookup(tablePred, offset);
        
        if (debug) {
          printf("pixel (%4d,%4d) = 0x%08X\n", col, row, pixel);
        }
        
        // Write row num to output col (transposed)

#if defined(DEBUG)
        if (debugDumpInputPixels) {
          const int row0 = row-originY;
          (*rowVecPtr)[row0] = pixel;
          assert(rowVecPtr->size() <= (rowWidth-1));
        }
#endif // DEBUG
        
        inputPixelsRowN[rowT0] += 1;
        
        if (debug) {
          printf("update N for row %d to %d\n", rowT0, inputPixelsRowN[rowT0]);
        }
        
        uint32_t *rowPtr = inputPixelsRowPtrs[rowT0];
        *rowPtr = pixel & 0x00FFFFFF;
        inputPixelsRowPtrs[rowT0] = rowPtr + 1;
        
        if (debug) {
          printf("append pixel to row %d\n", rowT0);
        }
        
      }
    } // end foreach row in current col
    
    colOff += 1;
  }
  
  // Write N for each row
  
  {
    uint32_t *bufferPtr = &inputPixelsBuffer[0];
    
    // Advance past row width
    
    bufferPtr += 1;
    
    int N;

    N = inputPixelsRowN[0];
#if defined(DEBUG)
    assert(N < rowWidth);
#endif // DEBUG
    *bufferPtr = N;
    bufferPtr += rowWidth;

    N = inputPixelsRowN[1];
#if defined(DEBUG)
    assert(N < rowWidth);
#endif // DEBUG
    *bufferPtr = N;
    bufferPtr += rowWidth;
    
    N = inputPixelsRowN[2];
#if defined(DEBUG)
    assert(N < rowWidth);
#endif // DEBUG
    *bufferPtr = N;
    bufferPtr += rowWidth;
    
    N = inputPixelsRowN[3];
#if defined(DEBUG)
    assert(N < rowWidth);
#endif // DEBUG
    *bufferPtr = N;
    bufferPtr += rowWidth;

    N = inputPixelsRowN[4];
#if defined(DEBUG)
    assert(N < rowWidth);
#endif // DEBUG
    *bufferPtr = N;
    bufferPtr += rowWidth;
  }
  
  // FIXME: when trailing N = 0, then reduce the colOff
  // so that CTI_BoxPredictPixel() need not read zero values
  
  // Debug input pixel values from row and col read logic
  
#if defined(DEBUG)
  if (debugDumpInputPixels) {
    for ( auto & rowVec : inputPixels ) {
      for ( auto pixel : rowVec ) {
        printf("0x%08X ", pixel);
      }
      printf("\n");
    }
  }
#endif // DEBUG
  
  // Predict RGB from transposed input pixels
  
  return CTI_BoxPredictPixel(ctiStruct,
                             tablePred,
                             centerX,
                             centerY,
                             inputPixelsBuffer,
                             colOffStart,
                             colOff);
}

// Calculate weighted sum of 3 values

static inline
unsigned int CTI_WeightedSum(int sum0, int sum1, int sum2)
{
  const bool debug = false;
  
  if ((0)) {
    // ave = sum/3 = ave3(sum0, sum1, sum2)
    
    int scaledSum0 = sum0;
    int scaledSum1 = -1;
    int scaledSum2 = -1;
    
    if (sum1 != -1) {
      scaledSum1 = fast_div_2(sum1); // (sum1 * 50%)
    }
    
    if (sum2 != -1) {
      scaledSum2 = fast_div_4(sum2); // (sum2 * 0.25%)
    }
    
    if (debug) {
      printf("ave scaled sum (0 1 2) = (%d + %d + %d)\n", scaledSum0, scaledSum1, scaledSum2);
    }
    
    if (debug) {
      printf("sum %d\n", scaledSum0 + ((scaledSum1 == -1) ? 0 : scaledSum1) + ((scaledSum2 == -1) ? 0 : scaledSum2));
    }
    
    // Divide final sum by 3 to get a weighted average of 3 inputs
    
    unsigned int ave3 = average_123(scaledSum0, scaledSum1, scaledSum2);
    
    if (debug) {
      printf("ave3 -> %d\n", ave3);
    }
    
#if defined(DEBUG)
    assert(ave3 != -1);
#endif // DEBUG
    
    return ave3;
  } else {
    // Weighted sum
    
    // The final average is a weighted sum of the 3 row sums.
    // Approx: sum(100% + 50% + 25%)
    
    int ave1 = sum1;
    int ave2 = sum2;
    
    unsigned int wSum;
    
    if (debug) {
      printf("ave scaled sum (0 1 2) = (%d + %d + %d)\n", sum0, ave1, ave2);
    }
    
    if (ave1 == -1 && ave2 == -1) {
      // no average needed
      
      if (debug) {
        printf("no weighted calc\n");
      }
      
      wSum = sum0;
    } else if (ave1 != -1 && ave2 != -1) {
      // Weighted Ave (0,1,2) = 16/32 (50%) + 10/32 (31%) + 6/32 (19%)
      
      wSum = (sum0 * 16) + (ave1 * 10) + (ave2 * 6);
      
      if (debug) {
        printf("weighted (0,1,2) calc\n");
      }
      
      // FIXME: translate to shifts
      
      wSum /= 32;
    } else if (ave1 == -1) {
      // Weighted Ave (0,X,2) = 24/32 (75%) + 8/32 (25%)
      
      wSum = (sum0 * 24) + (ave2 * 8);
      
      if (debug) {
        printf("weighted (0,X,2) calc\n");
      }
      
      wSum /= 32;
    } else if (ave2 == -1) {
      // Weighted Ave (0,1,X) = 21/32 (65%) + 11/32 (35%)
      
      if (debug) {
        printf("weighted (0,1,X) calc\n");
      }
      
      wSum = (sum0 * 21) + (ave1 * 11);
      
      // FIXME: translate to shifts
      
      wSum /= 32;
    } else {
      assert(0);
    }
    
    if (debug) {
      printf("weighted sum %d\n", wSum);
    }
    
    return wSum;
  }
}

// Given the (X1,Y1) and (X2,Y2) coordinates of a rectangular region,
// sum the values in each row and return a weighted sum.

//#define BOX_DELTA_SUM_WITH_CACHE

//static inline
static __attribute__ ((noinline))
unsigned int CTI_BoxDeltaSum(
                             const vector<int16_t> & deltaVec,
                             int numCols,
                             int numRows,
                             int originOffset,
                             int widthMinusX,
                             int rowOff
)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_BoxDeltaSum (cols,rows) (%d,%d) : origin offset %d : widthMinusX %d : rowOff %d\n", numRows, numCols, originOffset, widthMinusX, rowOff);
  }
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 2 2 (sumU2)
  // 1 1 1 (sumU1)
  // 0 X 0 (sum0)
  // 1 1 1 (sumD1)
  // 2 2 2 (sumD2)
  
  int sumU2 = -1; // W = 0.25
  int sumU1 = -1; // W = 0.5
  int sum0  = -1; // W = 1.0
  int sumD1 = -1; // W = 0.5
  int sumD2 = -1; // W = 0.25
  
  if (debug) {
    printf("ORIGIN OFFSET %d\n", originOffset);
    printf("W x H  : %d x %d\n", numCols, numRows);
    printf("rowOff   %d\n", rowOff);
  }
  
  // Gather 1 -> 15 cached values and count number of pixels that are cached
  
  unsigned int offset = originOffset;
  
  // Read 1,2,3 values from a non-center row
  
  auto ncRead = [&offset, widthMinusX](const vector<int16_t> & deltaVec, const int numCols)->int {
    int sumForRow = 0;
    int N = 0;
    
    for ( int col = 0; col < numCols; col++ ) {
      int cachedVal = deltaVec[offset];
      
      if (debug) {
        printf("cached col %d : offset %d = %d\n", col, offset, cachedVal);
      }
      
      if (cachedVal != -1) {
        // Add value if cached
        
        sumForRow += cachedVal;
        N += 1;
      }
      
      offset += 1;
    } // end foreach col in row
    
    // end of row
    offset += widthMinusX;
    
    switch (N) {
      case 0: {
        // No cached values in this row
        sumForRow = -1;
        break;
      }
      case 1: {
        // Nop since the sum for a row contains only 1 value
        break;
      }
      case 2: {
        // Ave of 2 elements
        sumForRow = fast_div_2(sumForRow);
        break;
      }
      case 3: {
        // Efficient N / 3
        sumForRow = fast_div_3(sumForRow);
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    return sumForRow;
  };

  // Read 1 or 2 values from a center row. The second column in a center row is never read since
  // it is known to always be invalid. Note that numCols could be 1,2,3 but there will only
  // ever be 1 or 2 valid cached values.
  
  auto cRead = [&offset, widthMinusX](const vector<int16_t> & deltaVec, const int numCols)->int {
    int sumForRow = 0;
    int N = 0;
    
#if defined(DEBUG)
    assert(numCols == 1 || numCols == 2 || numCols == 3);
#endif // DEBUG

    // Read column 0 which is known to always contain a valid cached value

#if defined(DEBUG)
    assert(deltaVec[offset] != -1);
#endif // DEBUG
    
    {
      int cachedVal = deltaVec[offset];
      
      if (debug) {
        int col = 0;
        printf("cached col %d : offset %d = %d\n", col, offset, cachedVal);
      }
      
      if (1) {
        // Add value if cached
        
        sumForRow += cachedVal;
        N += 1;
      }
      
      offset += 1;
    } // end foreach col in row
    
    // Skip column 1 which is always invalid center pixel
    
    {
#if defined(DEBUG)
      assert(deltaVec[offset] == -1);
#endif // DEBUG
      
      offset += 1;
    }
    
    // Read column 2 which may or may not contain a valid value
    
    if (numCols == 3) {
      int cachedVal = deltaVec[offset];
      
      if (debug) {
        int col = 2;
        printf("cached col %d : offset %d = %d\n", col, offset, cachedVal);
      }
      
      if (cachedVal != -1) {
        // Add value if cached
        
        sumForRow += cachedVal;
        N += 1;
      }
      
      offset += 1;
    }
    
    // end of row
    offset += widthMinusX;

#if defined(DEBUG)
    assert(N == 1 || N == 2);
#endif // DEBUG
    
    if (N == 2) {
      sumForRow = fast_div_2(sumForRow);
    }
    
    return sumForRow;
  };
  
  int rowsLeft = numRows;
  
  if (rowOff == -2) {
    rowOff += 1;
    rowsLeft -= 1;
    sumU2 = ncRead(deltaVec, numCols);
  }
  if (rowOff == -1) {
    rowOff += 1;
    rowsLeft -= 1;
    sumU1 = ncRead(deltaVec, numCols);
  }

  rowOff += 1;
  rowsLeft -= 1;
  sum0 = cRead(deltaVec, numCols);

  if (rowsLeft > 0) {
    rowOff += 1;
    rowsLeft -= 1;
    sumD1 = ncRead(deltaVec, numCols);
  }
  
  if (rowsLeft > 0) {
    rowOff += 1;
    rowsLeft -= 1;
    sumD2 = ncRead(deltaVec, numCols);
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  return CTI_WeightedSum(sum0, sum1, sum2);
}

// Predict in a horizontal 3x5 box around the unknown pixel
// by reading from neighbors and generating a weighted average.

//static inline
static __attribute__ ((noinline))
unsigned int CTI_BoxDeltaPredictH(
                                  CTI_Struct & ctiStruct,
                                  const bool tablePred,
                                  int centerX,
                                  int centerY)
{
  const bool debug = false;
  
  const auto & cachedHDeltaSums = ctiStruct.cachedHDeltaSums;
  
  // FIXME: rename later
  const int regionWidth = ctiStruct.width;
  const int regionHeight = ctiStruct.height;
  
  if (debug) {
    printf("CTI_BoxPredictH(%d,%d)\n", centerX, centerY);
  }
  
  if (debug) {
    printf("Cached H\n");
    
    for ( int row = 0; row < regionHeight; row++ ) {
      for ( int col = 0; col < regionWidth; col++ ) {
        int cacheOffset = CTIOffset2d(col, row, regionWidth);
        const int16_t cachedHDeltaSums = ctiStruct.cachedHDeltaSums.values[cacheOffset];
        
        printf("%4d ", cachedHDeltaSums);
      }
      
      printf("\n");
    }
  }

  // Print the L2 cache values
  
#if defined(BOX_DELTA_SUM_WITH_CACHE)
  if (debug) {
    printf("Cached H sum3\n");
    
    for ( int row = 0; row < regionHeight; row++ ) {
      for ( int col = 0; col < regionWidth; col++ ) {
        int cacheOffset = CTIOffset2d(col, row, regionWidth);
        const int16_t val = ctiStruct.cachedHDeltaRows.values[cacheOffset];
        
        printf("%4d ", val);
      }
      
      printf("\n");
    }
  }
#endif // BOX_DELTA_SUM_WITH_CACHE
  
  // Horizontal 3 x 5 box where the center coordinate
  // is the pixel to be predicted.
  
#if defined(DEBUG)
  // C must not be processed
  
  assert(centerX >= 0);
  assert(centerX < regionWidth);
  
  assert(centerY >= 0);
  assert(centerY < regionHeight);
  
  int centerOffset = CTIOffset2d(centerX, centerY, regionWidth);
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed == false);
  }
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
  
  // The 2 pixels to the left of (cx, cy) must be defined
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX - 1, centerY);
    assert(pixelWasProcessed == true);
  }
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX - 2, centerY);
    assert(pixelWasProcessed == true);
  }
#endif // DEBUG
  
  // FIXME: original paper uses 1 for the forward slot used with sum0.
  // Is this because a delta for those 2 pixels is actually farther away from
  // the pred function?
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 2 2 (sumU2)
  // 1 1 1 (sumU1)
  // 0 X 0 (sum0)
  // 1 1 1 (sumD1)
  // 2 2 2 (sumD2)
  
  int sumU2 = -1; // W = 0.25
  int sumU1 = -1; // W = 0.5
  int sum0  = -1; // W = 1.0
  int sumD1 = -1; // W = 0.5
  int sumD2 = -1; // W = 0.25
  
  //unsigned int N = 0;
  
  // Upper left corner of box is at (-1, -2). Note that since deltas are
  // calculated in terms of pairs of pixels the row 0 corresponds to
  // the difference between rows 0 and 1.
  
  const int boxWidth = 3;
  const int boxHalfWidth = (boxWidth - 1) / 2;
  
  const int boxHeight = 5;
  const int boxHalfHeight = (boxHeight - 1) / 2;
  
  int originX = (centerX - boxHalfWidth);
  int originY = (centerY - boxHalfHeight);
  
  // Adjust X origin to the left to account for center
  originX -= 1;
  
  int maxX = originX + (boxWidth - 1); // X + 2 inclusive indicates width of 5
  int maxY = originY + (boxHeight - 1); // Y + 4 inclusive indicates height of 3
  
  // rowOff is a counter that indicates where the row is relative to the
  // center coordinate. This is used to determine the pixel diff weight.
  
  int rowOff = -2;
  
  if (originX < 0) {
    originX = 0;
  }
  if (originY < 0) {
    if (originY == -2) {
      rowOff = 0;
    } else if (originY == -1) {
      rowOff = -1;
    }
    
    originY = 0;
  }
  
  if (maxX < (regionWidth-1)) {
    // nop
  } else {
    maxX = (regionWidth-1);
  }
  if (maxY < (regionHeight-1)) {
    // nop
  } else {
    maxY = (regionHeight-1);
  }
  
  int originOffset = CTIOffset2d(originX, originY, regionWidth);
  
  if (debug) {
    printf("ORIGIN  (%4d,%4d) %d\n", originX, originY, originOffset);
    printf("W x H  : %d x %d\n", maxX-originX+1, maxY-originY+1);
    printf("MAX     (%4d,%4d) %d\n", maxX, maxY, CTIOffset2d(maxX, maxY, regionWidth));
    printf("rowOff   %d\n", rowOff);
  }
  
  int rowOffInit = rowOff;
  
  // Gather 15 cached values and count number of pixels that are cached
  
  const unsigned int widthMinusX = regionWidth - (maxX - originX + 1);
  
  // Invoke optimized method to read row values
  
  const int numCols = maxX-originX+1;
  const int numRows = maxY-originY+1;
  
#if defined(BOX_DELTA_SUM_WITH_CACHE)
  const int col = maxX;

  auto & cachedHDeltaRows = ctiStruct.cachedHDeltaRows;
  //auto & cachedHDeltaSums = ctiStruct.cachedHDeltaSums;
  
  for ( int row = originY; row <= maxY; row++ ) {
    
    //int sumForRow = ctiStruct.getRow3CacheValue(true, col, row);
    
    int sumForRow = cachedHDeltaRows.getCachedValue(cachedHDeltaSums, col, row);
    
    if (debug) {
      printf("cached (%d,%d) = sumForRow %d\n", col, row, sumForRow);
    }
    
    // Set ave slot for this row
    
    switch (rowOff) {
      case -2: {
        sumU2 = sumForRow;
        break;
      }
      case -1: {
        sumU1 = sumForRow;
        break;
      }
      case 0: {
        sum0 = sumForRow;
        break;
      }
      case 1: {
        sumD1 = sumForRow;
        break;
      }
      case 2: {
        sumD2 = sumForRow;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    rowOff += 1;
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  unsigned int result = CTI_WeightedSum(sum0, sum1, sum2);
#else
  unsigned int result = CTI_BoxDeltaSum(cachedHDeltaSums.values,
                                        numCols,
                                        numRows,
                                        originOffset,
                                        widthMinusX,
                                        rowOffInit);
#endif // BOX_DELTA_SUM_WITH_CACHE
  
#if defined(DEBUG)
  const bool debugDoSumLoop = true;
#else
  const bool debugDoSumLoop = false;
#endif // DEBUG
  
  if (debugDoSumLoop) {
  
    rowOff = rowOffInit;
    
    unsigned int offset = originOffset;
    
  for ( int row = originY; row <= maxY; row++ ) {
    int N = 0; // reset num elements counter for each row
    
    int sumForRow = 0;
    
    for ( int col = originX; col <= maxX; col++ ) {
      
      if (1 && debug) {
        printf("cache coord (%4d,%4d) = %d\n", col, row, offset);
      }
      
#if defined(DEBUG)
      {
        // offset in deltas table, 0 corresponds to delta between 0 and 1
        
        int cacheOffset = CTIOffset2d(col, row, regionWidth);
        
        if (offset != cacheOffset) {
          assert(offset == cacheOffset);
        }
        
        cachedHDeltaSums.assertIfInvalidOffset(cacheOffset);
      }
#endif // DEBUG
      
      const int16_t cachedHDeltaSum = cachedHDeltaSums.values[offset];
      
      int cachedVal = cachedHDeltaSum;
      
      if (debug) {
        printf("cached (%4d,%4d) offset %d = %d\n", col, row, offset, cachedVal);
      }
      
      // Verify that the cached value exactly matches the value generated
      // by explicitly calculating the coordinates at (X-1,Y) -> (X,Y)
      
#if defined(DEBUG)
      {
        // Explicitly calc H delta
        
        assert(col >= 0);
        assert(col < regionWidth);
        
        bool pixel1WasProcessed;
        bool pixel2WasProcessed;
        
        if ((col+1) < regionWidth) {
          pixel1WasProcessed = ctiStruct.wasProcessed(col, row);
          pixel2WasProcessed = ctiStruct.wasProcessed(col+1, row);
        } else {
          pixel1WasProcessed = true;
          pixel2WasProcessed = false;
        }
        
        if (cachedVal == -1) {
          assert(!pixel1WasProcessed || !pixel2WasProcessed);
        } else {
          assert(pixel1WasProcessed && pixel2WasProcessed);
        }
        
        if (cachedVal != -1) {
          // Explicitly calculate delta and compare to cached value
          
//          if (debug) {
//            printf("checking cached value for H delta for coords (%d,%d) -> (%d,%d)\n", col, row, col+1, row);
//          }
          
          // FIXME: call simpleDelta() here
          
          int offset1 = CTIOffset2d(col, row, regionWidth);
          int offset2 = CTIOffset2d(col+1, row, regionWidth);
          
//          if (debug) {
//            printf("H delta offsets %d -> %d\n", offset1, offset2);
//          }
          
          int delta = ctiStruct.simpleDelta(tablePred, offset1, offset2);
          
          assert(delta == cachedVal);
        }
      }
#endif // DEBUG
      
      if (cachedVal != -1) {
        // Add value when value is cached
        
        sumForRow += cachedVal;
        N += 1;
      }
      
      offset += 1;
    } // end foreach col in current row
    
    // Generate ave for 1,2,3 deltas in this row
    
    if (debug) {
      printf("rowOff %d contains = %d deltas\n", rowOff, N);
    }
    
    switch (N) {
      case 0: {
        // No cached values in this row
        sumForRow = -1;
        break;
      }
      case 1: {
        // Nop since the sum for a row contains only 1 value
        break;
      }
      case 2: {
        // Ave of 2 elements
        sumForRow = fast_div_2(sumForRow);
        break;
      }
      case 3: {
        // Efficient N / 3
        sumForRow = fast_div_3(sumForRow);
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
#if defined(BOX_DELTA_SUM_WITH_CACHE)
    
    // Check that the row3 cached value for this row matches the one we just calculated
    
    if ((1)) {
      int col = maxX;
      
//      if (col == 6 && row == 9) {
//        printf("target\n");
//      }
      
      int16_t val = ctiStruct.getRow3CacheValue(true, col, row);
      
      if (debug) {
        printf("cached (%d,%d) = %d : sumForRow %d\n", col, row, val, sumForRow);
      }
      
      if (N == 0) {
        // getRow3CacheValue returns -1 in this special case where
        // the cache is valid but no pixels were processed in this row
        assert(val == -1);
      } else {
        assert(val == sumForRow);
      }
    }
    
#endif // BOX_DELTA_SUM_WITH_CACHE
    
    // Set ave slot for this row
    
    switch (rowOff) {
      case -2: {
        sumU2 = sumForRow;
        break;
      }
      case -1: {
        sumU1 = sumForRow;
        break;
      }
      case 0: {
        sum0 = sumForRow;
        break;
      }
      case 1: {
        sumD1 = sumForRow;
        break;
      }
      case 2: {
        sumD2 = sumForRow;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    offset += widthMinusX;
    rowOff += 1;
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  unsigned int result2 = CTI_WeightedSum(sum0, sum1, sum2);
    
  assert(result == result2);
  }
  
  return result;
}

/*

// Vertical weight prediction, same as CTI_BoxDeltaPredictH except that
// the input coords differ and offsets need to be calculated transposed.

//static inline
static __attribute__ ((noinline))
unsigned int CTI_BoxDeltaPredictV(
                                  CTI_Struct & ctiStruct,
                                  const bool tablePred,
                                  int centerX,
                                  int centerY)
{
  const bool debug = false;
  
  const auto & cachedVDeltaSums = ctiStruct.cachedVDeltaSums;
  
  // FIXME: rename later
  const int regionWidth = ctiStruct.width;
  const int regionHeight = ctiStruct.height;
  
  if (debug) {
    printf("CTI_BoxDeltaPredictV(%d,%d)\n", centerX, centerY);
  }
  
  if (debug) {
    printf("Cached V\n");
    
    for ( int row = 0; row < regionHeight; row++ ) {
      for ( int col = 0; col < regionWidth; col++ ) {
        int cacheOffset = CTIOffset2d(col, row, regionWidth);
        const CachedDelta cachedVDeltaSums = ctiStruct.cachedVDeltaSums[cacheOffset];
        
        printf("%4d ", cachedVDeltaSums.d);
      }
      
      printf("\n");
    }
  }
  
  // Horizontal 3 x 5 box where the center coordinate
  // is the pixel to be predicted.
  
#if defined(DEBUG)
  // C must not be processed
  
  assert(centerX >= 0);
  assert(centerX < regionWidth);
  
  assert(centerY >= 0);
  assert(centerY < regionHeight);
  
  // FIXME: center offset should be transposed
  int centerOffset = CTIOffset2d(centerX, centerY, regionWidth);
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed == false);
  }
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
  
  // The 2 pixels above (cx, cy) must have been processed already
  
  {
    assert((centerY - 1) >= 0);
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 1);
    assert(pixelWasProcessed == true);
  }
  {
    assert((centerY - 2) >= 0);
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 2);
    assert(pixelWasProcessed == true);
  }
#endif // DEBUG
  
  // FIXME: original paper uses 1 for the forward slot used with sum0.
  // Is this because a delta for those 2 pixels is actually farther away from
  // the pred function?
  
  // Note that this pattern is transposed
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 2 2 (sumU2)
  // 1 1 1 (sumU1)
  // 0 X 0 (sum0)
  // 1 1 1 (sumD1)
  // 2 2 2 (sumD2)
  
  int sumU2 = -1; // W = 0.25
  int sumU1 = -1; // W = 0.5
  int sum0  = -1; // W = 1.0
  int sumD1 = -1; // W = 0.5
  int sumD2 = -1; // W = 0.25
  
  //unsigned int N = 0;
  
  // Upper left corner of box is at (-1, -2). Note that since deltas are
  // calculated in terms of pairs of pixels the row 0 corresponds to
  // the difference between rows 0 and 1.
  
  const int boxWidth = 3;
  const int boxHalfWidth = (boxWidth - 1) / 2;
  
  const int boxHeight = 5;
  const int boxHalfHeight = (boxHeight - 1) / 2;
  
  int originX = (centerX - boxHalfWidth);
  int originY = (centerY - boxHalfHeight);
  
  // Adjust X origin to the left to account for center
  originX -= 1;
  
  int maxX = originX + (boxWidth - 1); // X + 2 inclusive indicates width of 5
  int maxY = originY + (boxHeight - 1); // Y + 4 inclusive indicates height of 3
  
  // rowOff is a counter that indicates where the row is relative to the
  // center coordinate. This is used to determine the pixel diff weight.
  
  int rowOff = -2;
  
  if (originX < 0) {
    originX = 0;
  }
  if (originY < 0) {
    if (originY == -2) {
      rowOff = 0;
    } else if (originY == -1) {
      rowOff = -1;
    }
    
    originY = 0;
  }
  
  if (maxX < (regionWidth-1)) {
    // nop
  } else {
    maxX = (regionWidth-1);
  }
  if (maxY < (regionHeight-1)) {
    // nop
  } else {
    maxY = (regionHeight-1);
  }
  
  int originOffset = CTIOffset2d(originY, originX, regionHeight);
  
  if (debug) {
    printf("ORIGIN  (%4d,%4d) %d\n", originX, originY, originOffset);
    printf("W x H  : %d x %d\n", maxX-originX+1, maxY-originY+1);
    printf("MAX     (%4d,%4d) %d\n", maxX, maxY, CTIOffset2d(maxX, maxY, regionWidth));
    printf("rowOff   %d\n", rowOff);
  }
  
  // Gather 15 cached values and count number of pixels that are cached
  
  const unsigned int widthMinusX = regionWidth - (maxX - originX + 1);
  
  unsigned int offset = originOffset;
  
  for ( int row = originY; row <= maxY; row++ ) {
    int N = 0; // reset num elements counter for each row
    
    int sumForRow = 0;
    
    for ( int col = originX; col <= maxX; col++ ) {
      
      if (1 && debug) {
        printf("cache coord (%4d,%4d) = %d\n", col, row, offset);
      }
      
#if defined(DEBUG)
      {
        // Note that offset is transposed here
        
        int cacheOffset = CTIOffset2d(row, col, regionHeight);
        
        if (offset != cacheOffset) {
          assert(offset == cacheOffset);
        }
        
        assert(cacheOffset >= 0);
        assert(cacheOffset < cachedVDeltaSums.size());
      }
#endif // DEBUG
      
      const CachedDelta cachedVDeltaSum = cachedVDeltaSums[offset];
      
      int cachedVal = cachedVDeltaSum.d;
      
      if (debug) {
        printf("cached (%4d,%4d) = %d : offset %d\n", col, row, cachedVal, offset);
      }
      
      // Verify that the cached value exactly matches the value generated
      // by explicitly calculating the coordinates at (X-1,Y) -> (X,Y)
      
#if defined(DEBUG)
      {
        // Explicitly calc V delta
        
        assert(col >= 0);
        assert(col < regionWidth);
        
        bool pixel1WasProcessed;
        bool pixel2WasProcessed;
        
        if ((col+1) < regionWidth) {
          pixel1WasProcessed = ctiStruct.wasProcessed(col, row);
          pixel2WasProcessed = ctiStruct.wasProcessed(col+1, row);
        } else {
          pixel1WasProcessed = true;
          pixel2WasProcessed = false;
        }
        
        if (cachedVal == -1) {
          assert(!pixel1WasProcessed || !pixel2WasProcessed);
        } else {
          assert(pixel1WasProcessed && pixel2WasProcessed);
        }
        
        if (cachedVal != -1) {
          // Explicitly calculate delta and compare to cached value
          
          // FIXME: call simpleDelta() here
          
          int offset1 = CTIOffset2d(col, row, regionWidth);
          int offset2 = CTIOffset2d(col, row+1, regionWidth);
          
          int delta = CTI_PredictDelta3(ctiStruct.pixelsPtr,
                                        ctiStruct.colortablePixelsPtr,
                                        ctiStruct.colortableNumPixels,
                                        ctiStruct.tableOffsetsPtr,
                                        tablePred,
                                        ctiStruct.width,
                                        ctiStruct.height,
                                        ctiStruct,
                                        offset1,
                                        offset2,
                                        -1,
                                        -1);
          
          assert(delta == cachedVal);
        }
      }
#endif // DEBUG
      
      if (cachedVal != -1) {
        // Add value when value is cached
        
        sumForRow += cachedVal;
        N += 1;
      }
      
      offset += 1;
    } // end foreach col in current row
    
    // Generate ave for 1,2,3 deltas in this row
    
    if (debug) {
      printf("rowOff %d contains = %d deltas\n", rowOff, N);
    }
    
    switch (N) {
      case 0: {
        // No cached values in this row
        sumForRow = -1;
        break;
      }
      case 1: {
        // Nop since the sum for a row contains only 1 value
        break;
      }
      case 2: {
        // Ave of 2 elements
        sumForRow = fast_div_2(sumForRow);
        break;
      }
      case 3: {
        // Efficient N / 3
        sumForRow = fast_div_3(sumForRow);
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    // Set ave slot for this row
    
    switch (rowOff) {
      case -2: {
        sumU2 = sumForRow;
        break;
      }
      case -1: {
        sumU1 = sumForRow;
        break;
      }
      case 0: {
        sum0 = sumForRow;
        break;
      }
      case 1: {
        sumD1 = sumForRow;
        break;
      }
      case 2: {
        sumD2 = sumForRow;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    offset += widthMinusX;
    rowOff += 1;
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  return CTI_WeightedSum(sum0, sum1, sum2);
}

*/

// Predict in a horizontal 5x3 box around the unknown pixel
// by reading from neighbors and generating a weighted average.

//static inline
static __attribute__ ((noinline))
unsigned int CTI_BoxDeltaPredictV(
                                  CTI_Struct & ctiStruct,
                                  const bool tablePred,
                                  int centerX,
                                  int centerY)
{
  const bool debug = false;
  
  const auto & cachedVDeltaSums = ctiStruct.cachedVDeltaSums;
  
  // FIXME: rename later
  const int regionWidth = ctiStruct.width;
  const int regionHeight = ctiStruct.height;
  
  if (debug) {
    printf("CTI_BoxPredictV(%d,%d)\n", centerX, centerY);
  }
  
  if (debug) {
    printf("Cached V (not transposed)\n");
    
    int height = regionHeight;
    int width = regionWidth;
    
    for ( int row = 0; row < height; row++ ) {
      for ( int col = 0; col < width; col++ ) {
        int cacheOffset = CTIOffset2d(row, col, regionHeight);
        const int16_t cachedVDeltaSum = ctiStruct.cachedVDeltaSums.values[cacheOffset];
        printf("%4d ", cachedVDeltaSum);
      }
      
      printf("\n");
    }
  }

  if (debug) {
    printf("Cached V (transposed)\n");
    
    int heightT = regionWidth;
    int widthT = regionHeight;
    
    int cacheOffset = 0;
    
    for ( int row = 0; row < heightT; row++ ) {
      for ( int col = 0; col < widthT; col++ ) {
        const int16_t cachedVDeltaSum = ctiStruct.cachedVDeltaSums.values[cacheOffset];
        printf("%4d ", cachedVDeltaSum);
        cacheOffset += 1;
      }
      
      printf("\n");
    }
  }
  
  // Horizontal 5 x 3 box where the center coordinate
  // is the pixel to be predicted.
  
#if defined(DEBUG)
  // C must not be processed
  
  assert(centerX >= 0);
  assert(centerX < regionWidth);
  
  assert(centerY >= 0);
  assert(centerY < regionHeight);
  
  int centerOffset = CTIOffset2d(centerX, centerY, regionWidth);
  
  {
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY);
    assert(pixelWasProcessed == false);
  }
  
  if (debug) {
    printf("C  (%d,%d) %d\n", centerX, centerY, centerOffset);
  }
  
  // The 2 pixels to the above of (cx, cy) must be defined
  
  {
    assert((centerY - 1) >= 0);
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 1);
    assert(pixelWasProcessed == true);
  }
  {
    assert((centerY - 2) >= 0);
    bool pixelWasProcessed = ctiStruct.wasProcessed(centerX, centerY - 2);
    assert(pixelWasProcessed == true);
  }
#endif // DEBUG
  
  // Scale 0 = 100%
  // Scale 1 = 50%
  // Scale 2 = 25%
  //
  // 2 1 0 1 2
  // 2 1 X 1 2
  // 2 1 0 1 2
  
  int sumU2 = -1; // W = 0.25
  int sumU1 = -1; // W = 0.5
  int sum0  = -1; // W = 1.0
  int sumD1 = -1; // W = 0.5
  int sumD2 = -1; // W = 0.25
  
  // Upper left corner of box is at (-2, -1). Note that since deltas are
  // calculated in terms of pairs of pixels the row 0 corresponds to
  // the difference between rows 0 and 1.
  
  const int boxWidth = 5;
  const int boxHalfWidth = (boxWidth - 1) / 2;
  
  const int boxHeight = 3;
  const int boxHalfHeight = (boxHeight - 1) / 2;
  
  int originX = (centerX - boxHalfWidth);
  int originY = (centerY - boxHalfHeight);
  
  // Adjust Y origin to the left to account for center
  originY -= 1;
  
  int maxX = originX + (boxWidth - 1); // X + 4 inclusive indicates width of 5
  int maxY = originY + (boxHeight - 1); // Y + 2 inclusive indicates height of 3
  
  // colOff is a counter that indicates where the column is relative to the
  // center coordinate. This is used to determine the pixel diff weight.
  
  int colOff = -2;
  
  if (originX < 0) {
    if (originX == -2) {
      colOff = 0;
    } else if (originX == -1) {
      colOff = -1;
    }
    
    originX = 0;
  }
  if (originY < 0) {
    originY = 0;
  }
  
  if (maxX <= (regionWidth-1)) {
    // nop
  } else {
    maxX = regionWidth-1;
  }
  if (maxY <= (regionHeight-1)) {
    // nop
  } else {
    maxY = regionHeight-1;
  }
  
  // FIXME: in the event that maxY == (regionHeight-1) then there is
  // no way that vertical deltas will indicate a useful cached value.
  // Reduce the maxY when on the final row.
  
  // Note that originOffset is transposed
  
  int originOffset = CTIOffset2d(originY, originX, regionHeight);
  
  if (debug) {
    printf("ORIGIN  (%4d,%4d) %d\n", originX, originY, originOffset);
    printf("MAX     (%4d,%4d) %d\n", maxX, maxY, CTIOffset2d(maxX, maxY, regionWidth));
    printf("W x H  : %d x %d\n", maxX-originX+1, maxY-originY+1);
    printf("colOff   %d\n", colOff);
  }

  // Note that value to add is transposed
  
  const unsigned int widthMinusX = regionHeight - (maxY - originY + 1);
  
  unsigned int offsetT = originOffset;
  
  int colOffInit = colOff;
  
  // Invoke optimized method to read row values, note that
  // numCols and numRows are swapped here (transposed)
  
  const int numCols = maxX-originX+1;
  const int numRows = maxY-originY+1;
  
#if defined(BOX_DELTA_SUM_WITH_CACHE)
  
  auto & cachedVDeltaRows = ctiStruct.cachedVDeltaRows;
  
  for ( int col = originX; col <= maxX; col++ ) {
    //int N = 0; // reset num elements counter for each row
    
    int row = maxY;
    
    //int sumForCol = ctiStruct.getRow3CacheValue(false, col, row);
    
    int sumForCol = cachedVDeltaRows.getCachedValue(cachedVDeltaSums, col, row);
    
    if (debug) {
      printf("cached (%d,%d) = sumForCol %d\n", col, row, sumForCol);
    }
    
    if (debug) {
      printf("row %d : colOff %d\n", col, colOff);
    }
    
    // Set ave slot for this row
    
    switch (colOff) {
      case -2: {
        sumU2 = sumForCol;
        break;
      }
      case -1: {
        sumU1 = sumForCol;
        break;
      }
      case 0: {
        sum0 = sumForCol;
        break;
      }
      case 1: {
        sumD1 = sumForCol;
        break;
      }
      case 2: {
        sumD2 = sumForCol;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
//    offsetT += widthMinusX;
    colOff += 1;
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  unsigned int result = CTI_WeightedSum(sum0, sum1, sum2);
#else // BOX_DELTA_SUM_WITH_CACHE
  unsigned int result = CTI_BoxDeltaSum(cachedVDeltaSums.values,
                                        numRows,
                                        numCols,
                                        originOffset,
                                        widthMinusX,
                                        colOffInit);
#endif // BOX_DELTA_SUM_WITH_CACHE
  
#if defined(DEBUG)
  const bool debugDoSumLoop = true;
#else
  const bool debugDoSumLoop = false;
#endif // DEBUG
  
  if (debugDoSumLoop) {
  
  colOff = colOffInit;
    
  // Always include (Y - 2) in the V prediction since it
  // is possible for all other caches are unset.
  
  for ( int col = originX; col <= maxX; col++ ) {
    int N = 0; // reset num elements counter for each row
    
    int sumForCol = 0;
    
    for ( int row = originY; row <= maxY; row++ ) {
    
      if (0 && debug) {
        printf("cache coord (%4d,%4d) = %d\n", col, row, offsetT);
        printf("colOff %d\n", colOff);
      }
      
#if defined(DEBUG)
      {
        //int cacheOffset = CTIOffset2d(col, row, regionWidth);
        int cacheOffset = CTIOffset2d(row, col, regionHeight);
        
        if (offsetT != cacheOffset) {
          assert(offsetT == cacheOffset);
        }
        
        cachedVDeltaSums.assertIfInvalidOffset(offsetT);
      }
#endif // DEBUG
      
      const int16_t cachedDeltaSum = cachedVDeltaSums.values[offsetT];
      
      int cachedVal = cachedDeltaSum;
      
      if (debug) {
        printf("cached (%4d,%4d) %d = %d\n", col, row, offsetT, cachedVal);
      }
      
      // Verify that the cached value exactly matches the value generated
      // by explicitly calculating the coordinates at (X,Y) -> (X+1,Y)
      
#if defined(DEBUG)
      {
        // Explicitly calc V delta
        
        assert(row >= 0);
        assert(row < regionHeight);
        
        bool pixel1WasProcessed;
        bool pixel2WasProcessed;
        
//        if (centerX == 2 && centerY == 3) {
//          printf("target\n");
//        }
        
        if ((row+1) < regionHeight) {
          pixel1WasProcessed = ctiStruct.wasProcessed(col, row);
          pixel2WasProcessed = ctiStruct.wasProcessed(col, row+1);
        } else {
          pixel1WasProcessed = true;
          pixel2WasProcessed = false;
        }
        
        if (cachedVal == -1) {
          assert(!pixel1WasProcessed || !pixel2WasProcessed);
        } else {
          assert(pixel1WasProcessed && pixel2WasProcessed);
        }
        
        if (cachedVal != -1) {
          // Explicitly calculate delta and compare to cached value
          
          int offset1 = CTIOffset2d(col, row, regionWidth);
          int offset2 = CTIOffset2d(col, row+1, regionWidth);
          
          int delta = ctiStruct.simpleDelta(tablePred, offset1, offset2);
          
          assert(delta == cachedVal);
        }
      }
#endif // DEBUG
      
      if (cachedVal != -1) {
        // Add value when value is cached
        
        sumForCol += cachedVal;
        N += 1;
      }
      
      offsetT += 1;
    } // end for col
    
    if (debug) {
      printf("row N = %d for colOff %d\n", N, colOff);
    }
    
    switch (N) {
      case 0: {
        // No cached values in this row
        sumForCol = -1;
        break;
      }
      case 1: {
        // Nop since the sum for a row contains only 1 value
        break;
      }
      case 2: {
        // Ave of 2 elements
        sumForCol = fast_div_2(sumForCol);
        break;
      }
      case 3: {
        // Efficient N / 3
        sumForCol = fast_div_3(sumForCol);
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
#if defined(BOX_DELTA_SUM_WITH_CACHE)
    
    // Check that the row3 cached value for this row matches the one we just calculated
    
    if ((1)) {
      int row = maxY;
      
      int16_t val = ctiStruct.getRow3CacheValue(false, col, row);
      
      if (debug) {
      printf("cached (%d,%d) = %d : sumForCol %d\n", col, row, val, sumForCol);
      }
      
      if (N == 0) {
        // getRow3CacheValue returns -1 in this special case where
        // the cache is valid but no pixels were processed in this row
        assert(val == -1);
      } else {
        assert(val == sumForCol);
      }
    }
    
#endif // BOX_DELTA_SUM_WITH_CACHE
    
    // Set ave slot for this row
    
    switch (colOff) {
      case -2: {
        sumU2 = sumForCol;
        break;
      }
      case -1: {
        sumU1 = sumForCol;
        break;
      }
      case 0: {
        sum0 = sumForCol;
        break;
      }
      case 1: {
        sumD1 = sumForCol;
        break;
      }
      case 2: {
        sumD2 = sumForCol;
        break;
      }
#if defined(DEBUG)
      default: {
        assert(0);
      }
#endif // DEBUG
    }
    
    offsetT += widthMinusX;
    colOff += 1;
  }
  
  if (debug) {
    printf("cached sumU2 = %d\n", sumU2);
    printf("cached sumU1 = %d\n", sumU1);
    printf("cached sum0  = %d\n", sum0);
    printf("cached sumD1 = %d\n", sumD1);
    printf("cached sumD2 = %d\n", sumD2);
  }
  
  // 0 sum0 = sum0
  // 1 sum1 = ave(sumD1 + sumU1) with 0,1,2 inputs
  // 2 sum2 = ave(sumD2 + sumU2) with 0,1,2 inputs
  
  //  sum1 = (sumD1 + sumU1) / 2;
  //  sum2 = (sumD2 + sumU2) / 2;
  
  int sum1 = average_012(sumD1, sumU1);
  int sum2 = average_012(sumD2, sumU2);
  
  if (debug) {
    printf("ave sum (0 1 2) = (%d + %d + %d)\n", sum0, sum1, sum2);
  }
  
#if defined(DEBUG)
  assert(sum0 != -1);
#endif // DEBUG
  
  unsigned int result2 = CTI_WeightedSum(sum0, sum1, sum2);
  
  assert(result == result2);
  }

  return result;
}

#endif // USE_BOX_DELTA

/*

// Invalidate a specific cached row during an interation

static inline
bool CTI_InvalidateRowCache(
                            const uint32_t * const pixelsPtr,
                            const uint32_t * const colortablePixelsPtr,
                            const int colortableNumPixels,
                            const uint8_t * const tableOffsetsPtr,
                            const bool tablePred,
                            const int regionWidth,
                            const int regionHeight,
                            CTI_Struct & ctiStruct,
                            const int cacheCol,
                            const int cacheRow,
                            const bool checkNext,
                            const bool assumeOtherIsSet,
                            const bool markDead)
{
  const bool debug = false;
  
  auto & hCounters = ctiStruct.hCounters;
  auto & vCounters = ctiStruct.vCounters;
  auto & hCache = ctiStruct.hCache;
//  auto & vCache = ctiStruct.vCache;
  
#if defined(DEBUG)
  auto & processedFlags = ctiStruct.processedFlags;
  auto & results = ctiStruct.results;
#endif // DEBUG
  
#if defined(DEBUG)
  {
    assert(cacheRow >= 0);
    assert(cacheRow < regionHeight);
    
    assert(cacheCol >= 0);
    assert(cacheCol < regionWidth);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not process any of the first 4
  if (cacheCol < 2 && cacheRow < 2) {
    assert(0);
  }
#endif // DEBUG
  
  // When advancing to the final column, clear the cache
  // and then do not recalculate a new cached value.
  
  if (cacheCol == (regionWidth-1)) {
    ctiStruct.clearRowCache(cacheRow, markDead);
    return false;
  }
  
#if defined(DEBUG)
  assert(cacheCol < (regionWidth-1));
#endif // DEBUG
  
  // Columns (0, W-1) (not including final column)
  
  auto endIter = end(ctiStruct.waitList);
  auto hCacheIter = hCache[cacheRow];
  if (hCacheIter != endIter) {
    if (markDead) {
      CoordDelta & cd = *hCacheIter;
      cd.setDead();
    }
    hCache[cacheRow] = endIter;
    
    if (debug) {
      printf("cleared delta cache for row %d\n", cacheRow);
    }
    
#if defined(DEBUG)
    assert((cacheCol-1) >= 0);
#endif // DEBUG
    
    int fromX = cacheCol-1;
    int toX = cacheCol;
    int y = cacheRow;
    
    int fromOffset = CTIOffset2d(fromX, y, regionWidth);
    int toOffset = CTIOffset2d(toX, y, regionWidth);
    
    // Determine if the pixel in the next column has already been
    // processed by looking at the counter for that column and
    // comparing it to the current row.

    if (checkNext == false) {
      // Ignore next pixel when invalidating from other
    } else {
      bool wasTargetPixelProcessed = CTI_WasColProcessed(
#if defined(DEBUG)
                                                         regionWidth,
                                                         regionHeight,
                                                         processedFlags,
#endif // DEBUG
                                                         vCounters,
                                                         cacheCol + 1,
                                                         cacheRow);
      
      // No need to recalculate the cached delta when pixel at next col has already been processed
      
      if (wasTargetPixelProcessed) {
        if (debug) {
          printf("no need to reinit cache for row %d\n", cacheRow);
        }
        
        return true;
      }
    }
    
    {
      // Generate a new cached value that takes (row-1,col) into
      // account as the other offset.
      
      int otherRow = -1;
      
      if (y == 0) {
        // Ignore other
      } else {
        // Other is (row - 1) for all other rows
        otherRow = y - 1;
      }
      
      // In the case where pixel otherOffset has not been set yet, ignore in delta calc
      
      int delta = CTI_PredictDelta3(pixelsPtr,
                                   colortablePixelsPtr,
                                   colortableNumPixels,
                                   tableOffsetsPtr,
                                   tablePred,
                                   regionWidth,
                                   regionHeight,
                                    ctiStruct,
                                   fromOffset,
                                   toOffset,
                                   toX,
                                   otherRow);
      
      bool isHorizontal = true;
      bool isIdentical = (delta == 0);
      
      CoordDelta coordDelta = CoordDelta(fromX, y, toX, y, regionWidth, isHorizontal, isIdentical, delta);
      
      hCache[y] = ctiStruct.addToWaitList(coordDelta);
      
      if (debug) {
        printf("recalc delta cache for row %d : (%d,%d)\n", y, toX, y);
      }
      
#if defined(DEBUG)
      results["initCalcRow3"] += 1;
#endif // DEBUG
    }
  } else {
    if (debug) {
      printf("delta cache not set for row %d\n", cacheRow);
    }
  }
  
  return false;
};

// Invalidate a specific cached row during an interation

static inline
bool CTI_InvalidateColCache(
                            const uint32_t * const pixelsPtr,
                            const uint32_t * const colortablePixelsPtr,
                            const int colortableNumPixels,
                            const uint8_t * const tableOffsetsPtr,
                            const bool tablePred,
                            const int regionWidth,
                            const int regionHeight,
                            CTI_Struct & ctiStruct,
                            const int cacheCol,
                            const int cacheRow,
                            const bool checkNext,
                            const bool assumeOtherIsSet,
                            const bool markDead)
{
  const bool debug = false;
  
  auto & hCounters = ctiStruct.hCounters;
  auto & vCounters = ctiStruct.vCounters;
//  auto & hCache = ctiStruct.hCache;
  auto & vCache = ctiStruct.vCache;
  
#if defined(DEBUG)
  auto & processedFlags = ctiStruct.processedFlags;
  auto & results = ctiStruct.results;
#endif // DEBUG
  
#if defined(DEBUG)
  {
    assert(cacheRow >= 0);
    assert(cacheRow < regionHeight);
    
    assert(cacheCol >= 0);
    assert(cacheCol < regionWidth);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not process any of the first 4
  if (cacheCol < 2 && cacheRow < 2) {
    assert(0);
  }
#endif // DEBUG
  
  // When advancing to the final row, clear the cache
  // and then do not recalculate a new cached value.
  
  if (cacheRow == (regionHeight-1)) {
    ctiStruct.clearColCache(cacheCol, markDead);
    return false;
  }
  
#if defined(DEBUG)
  assert(cacheRow < (regionHeight-1));
#endif // DEBUG
  
  auto endIter = end(ctiStruct.waitList);
  auto vCacheIter = vCache[cacheCol];
  if (vCacheIter != endIter) {
    // Ignore the iter ref since it is now invalid
    
    if (markDead) {
      CoordDelta & cd = *vCacheIter;
      cd.setDead();
    }

    vCache[cacheCol] = endIter;
    
    if (debug) {
      printf("cleared delta cache for col %d\n", cacheCol);
    }
    
#if defined(DEBUG)
    assert((cacheRow-2) >= 0);
#endif // DEBUG
    
    int fromY = cacheRow - 1;
    int toY = cacheRow;
    int x = cacheCol;
    
    int fromOffset = CTIOffset2d(x, fromY, regionWidth);
    int toOffset = CTIOffset2d(x, toY, regionWidth);
    
    // Determine if the pixel in the next row has already been
    // processed by looking at the counter for that row.
    
    if (checkNext == false) {
      // Ignore next pixel when invalidating from other
    } else {
      bool wasTargetPixelProcessed = CTI_WasRowProcessed(
#if defined(DEBUG)
                                                    regionWidth,
                                                    regionHeight,
                                                    processedFlags,
#endif // DEBUG
                                                    hCounters,
                                                    x,
                                                    toY + 1);
      
      if (wasTargetPixelProcessed) {
        if (debug) {
          printf("no need to reinit cache for col %d\n", x);
        }
        
        return true;
      }
    }
    
    {
      // Generate a new cached value that takes (col-1,row) into
      // account as the other offset.
      
      int otherCol = -1;
      
      if (x == 0) {
        // Ignore other
      } else {
        // Other is (col - 1) for all other rows
        otherCol = x - 1;
      }
      
      // In the case where pixel otherOffset has not been set yet, ignore in delta calc
      
      int delta = CTI_PredictDelta3(pixelsPtr,
                                   colortablePixelsPtr,
                                   colortableNumPixels,
                                   tableOffsetsPtr,
                                   tablePred,
                                   regionWidth,
                                   regionHeight,
                                    ctiStruct,
                                   fromOffset,
                                   toOffset,
                                   otherCol,
                                   toY);
      
      bool isHorizontal = false;
      bool isIdentical = (delta == 0);
      
      CoordDelta coordDelta = CoordDelta(x, fromY, x, toY, regionWidth, isHorizontal, isIdentical, delta);
      
      vCache[x] = ctiStruct.addToWaitList(coordDelta);
      
      if (debug) {
        printf("recalc delta cache for col %d : (%d,%d)\n", x, x, toY);
      }
      
#if defined(DEBUG)
      results["initCalcCol3"] += 1;
#endif // DEBUG
    }
  } else {
    if (debug) {
      printf("delta cache not set for col %d\n", cacheCol);
    }
  }
  
  return false;
}

static inline
void CTI_InitRowCache(
                      const uint32_t * const pixelsPtr,
                      const uint32_t * const colortablePixelsPtr,
                      const int colortableNumPixels,
                      const uint8_t * const tableOffsetsPtr,
                      const bool tablePred,
                      const int regionWidth,
                      const int regionHeight,
                      CTI_Struct & ctiStruct,
                      const int cacheCol,
                      const int cacheRow)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_InitRowCache(%d,%d)\n", cacheCol, cacheRow);
  }
  
  auto & hCounters = ctiStruct.hCounters;
  auto & vCounters = ctiStruct.vCounters;
  auto & hCache = ctiStruct.hCache;
  //auto & vCache = ctiStruct.vCache;
  
#if defined(DEBUG)
  auto & processedFlags = ctiStruct.processedFlags;
  auto & results = ctiStruct.results;
#endif // DEBUG
  
#if defined(DEBUG)
  {
    assert(cacheRow >= 0);
    assert(cacheRow < regionHeight);
    
    assert(cacheCol >= 0);
    assert(cacheCol < regionWidth);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not process any of the first 4
  if (cacheCol < 2 && cacheRow < 2) {
    assert(0);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not be a cached row at this point
  auto endIter = end(ctiStruct.waitList);
  auto hCacheIter = hCache[cacheRow];
  if (hCacheIter != endIter) {
    assert(0);
  }
#endif // DEBUG
  
  if (1) {
    if (1) {
      int fromX = cacheCol - 2;
      int toX = cacheCol - 1;
      int y = cacheRow;
      
      int fromOffset = CTIOffset2d(fromX, y, regionWidth);
      int toOffset = CTIOffset2d(toX, y, regionWidth);
      
      // Generate a new cached value that takes (row-1,col) into
      // account as the other offset.
      
      int otherOffset;
      int otherRow = -1;
      
      if (y == 0) {
        // Ignore other
        otherOffset = -1;
      } else {
        // Other is (row - 1) for all other rows
        otherRow = y - 1;
        otherOffset = CTIOffset2d(toX, otherRow, regionWidth);
      }
      
      int delta = CTI_PredictDelta3(pixelsPtr,
                                   colortablePixelsPtr,
                                   colortableNumPixels,
                                   tableOffsetsPtr,
                                   tablePred,
                                   regionWidth,
                                   regionHeight,
                                    ctiStruct,
                                   fromOffset,
                                   toOffset,
                                   toX,
                                   otherRow);
      
      bool isHorizontal = true;
      bool isIdentical = (delta == 0);
      
      CoordDelta coordDelta = CoordDelta(fromX, y, toX, y, regionWidth, isHorizontal, isIdentical, delta);
      
      hCache[y] = ctiStruct.addToWaitList(coordDelta);
      
      if (debug) {
        printf("init delta cache for row %d : (%d,%d) -> (%d,%d)\n", y, fromX, y, toX, y);
      }
      
#if defined(DEBUG)
      results["reinitCalcRow3"] += 1;
#endif // DEBUG
    }
  }
}

// Check to see if a specific row or column is currently cached
// and clear the cache if it is.

static inline
void CTI_InitColCache(
                      const uint32_t * const pixelsPtr,
                      const uint32_t * const colortablePixelsPtr,
                      const int colortableNumPixels,
                      const uint8_t * const tableOffsetsPtr,
                      const bool tablePred,
                      const int regionWidth,
                      const int regionHeight,
                      CTI_Struct & ctiStruct,
                      const int cacheCol,
                      const int cacheRow)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_InitColCache(%d,%d)\n", cacheCol, cacheRow);
  }
  
  auto & hCounters = ctiStruct.hCounters;
  auto & vCounters = ctiStruct.vCounters;
  //auto & hCache = ctiStruct.hCache;
  auto & vCache = ctiStruct.vCache;
  
#if defined(DEBUG)
  auto & processedFlags = ctiStruct.processedFlags;
  auto & results = ctiStruct.results;
#endif // DEBUG
  
#if defined(DEBUG)
  {
    assert(cacheRow >= 0);
    assert(cacheRow < regionHeight);
    
    assert(cacheCol >= 0);
    assert(cacheCol < regionWidth);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not process any of the first 4
  if (cacheCol < 2 && cacheRow < 2) {
    assert(0);
  }
#endif // DEBUG
  
#if defined(DEBUG)
  // Should not be a cached row at this point
  auto endIter = end(ctiStruct.waitList);
  auto vCacheIter = vCache[cacheCol];
  if (vCacheIter != endIter) {
    assert(0);
  }
#endif // DEBUG
  
  if (1) {
    if (1) {
      int fromY = cacheRow - 2;
      int toY = cacheRow - 1;
      int x = cacheCol;
      
      int fromOffset = CTIOffset2d(x, fromY, regionWidth);
      int toOffset = CTIOffset2d(x, toY, regionWidth);
      
      // Generate a new cached value that takes (row,col-1) into
      // account as the other offset.
      
      int otherOffset;
      int otherCol = -1;
      
      if (x == 0) {
        // Ignore other
        otherOffset = -1;
      } else {
        // Other is (col - 1) for all other rows
        otherCol = x-1;
        otherOffset = CTIOffset2d(otherCol, toY, regionWidth);
      }
      
      int delta = CTI_PredictDelta3(pixelsPtr,
                                   colortablePixelsPtr,
                                   colortableNumPixels,
                                   tableOffsetsPtr,
                                   tablePred,
                                   regionWidth,
                                   regionHeight,
                                    ctiStruct,
                                   fromOffset,
                                   toOffset,
                                   otherCol,
                                   toY);
      
      bool isHorizontal = false;
      bool isIdentical = (delta == 0);
      
      CoordDelta coordDelta = CoordDelta(x, fromY, x, toY, regionWidth, isHorizontal, isIdentical, delta);
      
      vCache[x] = ctiStruct.addToWaitList(coordDelta);
      
      if (debug) {
        printf("init delta cache for col %d : (%d,%d) -> (%d,%d)\n", x, x, fromY, x, toY);
      }
      
#if defined(DEBUG)
      results["reinitCalcCol3"] += 1;
#endif // DEBUG
    }
  }
};

*/
 
// Find the minimum delta in the horizontal or vertical trees

static inline
void CTI_MinimumSearch(
                       CTI_Struct & ctiStruct,
                       const bool tablePred,
                       CoordDelta *smallestPtr)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_MinimumSearch\n");
  }
  
#if defined(DEBUG)
  auto & results = ctiStruct.results;
#endif // DEBUG
  
  int regionWidth = ctiStruct.width;
  
  // Grab smallest delta with O(1) query
  
  while (1) {
    if (debug) {
      // Iterate over the first couple of deltas and print them out
      
      int err = ctiStruct.waitListHead();
      
      for (int i = 0; err != -1 && i < 5; i++ ) {
        auto & node = ctiStruct.waitList.nodeTable[err];
        vector<CoordDelta> & errTable = ctiStruct.waitList.elemTable[err];
        
        CoordDelta cd = errTable.back();
        printf("min[%d] (%d) %s\n", i, err, cd.toString().c_str());
        
        err = node.next;
      }
    }
    
    int minErr;
    CoordDelta minCD = ctiStruct.firstOnWaitList(&minErr);
    
#if defined(DEBUG)
    results["minRemove"] += 1;
#endif // DEBUG
    
  if (debug) {
    if (!minCD.isEmpty()) {
      printf("search min %s\n", minCD.toString().c_str());
    }
  }
  
  if (minCD.isEmpty()) {
    if (debug) {
      printf("search min empty condition detected\n");
    }
    
    *smallestPtr = minCD;
    break;
  } else {
    // If the delta is marked as dead, then ignore it since this
    // indicates a coordinate that was processed by the other axis.
    
    // The min delta would not need to be recalculated for
    // row 0 and col 0, since these values explicitly do not
    // depend on values other than the previous one.
    
    bool isHorizontal = minCD.isHorizontal();
    int fromX = minCD.fromX();
    int fromY = minCD.fromY();
    int toX = minCD.toX();
    int toY = minCD.toY();
    
    // Calculate prediction pixel
    
    int predX;
    int predY;
    
    if (isHorizontal) {
      predX = toX + 1;
      predY = toY;
    } else {
      predX = toX;
      predY = toY + 1;
    }
    
    // Pred (x,y), ignore this min if pixel was already processed
    
    bool nextPixelWasProcessed = ctiStruct.wasProcessed(predX, predY);
    if (nextPixelWasProcessed) {
#if defined(DEBUG)
      results["minWasProcessed"] += 1;
#endif // DEBUG
      continue;
    }

    // The top left corner pixels should never be recalculated
    
#if defined(DEBUG)
    if (predX < 2 && predY < 2) {
      // The upper left init set of 4 should never be recalculated
      assert(0);
    }
#endif // DEBUG
    
    {
      int oDelta = minErr;
      
      int nDelta;
      
      int fromOffset = CTIOffset2d(fromX, fromY, regionWidth);
      int toOffset = CTIOffset2d(toX, toY, regionWidth);
      
      int otherX;
      int otherY;
      
      // Note that row and col zero have been ignored at this point
      
      if (isHorizontal) {
        otherX = toX;
        otherY = toY - 1;
      } else {
        otherX = toX - 1;
        otherY = toY;
      }
      
      if (debug) {
        printf("CTI_MinimumSearch recalculating : %s\n", minCD.toString().c_str());
      }
      
      if (isHorizontal) {
#if defined(DEBUG)
# if defined(USE_BOX_DELTA)
        results["recalcBoxPredictH"] += 1;
# else
        results["recalcArrow"] += 1;
# endif
#endif // DEBUG

#if defined(USE_BOX_DELTA)
        nDelta = CTI_BoxDeltaPredictH(ctiStruct,
                                 tablePred,
                                 toX + 1,
                                 toY);
#else
        nDelta = CTI_PredictDeltaArrow<true>(ctiStruct,
                                             tablePred,
                                             fromOffset,
                                             toOffset,
                                             toX,
                                             toY);
#endif // USE_BOX_DELTA
      } else {
#if defined(DEBUG)
# if defined(USE_BOX_DELTA)
        results["recalcBoxPredictV"] += 1;
# else
        results["recalcArrow"] += 1;
# endif
#endif // DEBUG
        
#if defined(USE_BOX_DELTA)
        nDelta = CTI_BoxDeltaPredictV(ctiStruct,
                                 tablePred,
                                 toX,
                                 toY + 1);
#else
        nDelta = CTI_PredictDeltaArrow<false>(ctiStruct,
                                              tablePred,
                                              fromOffset,
                                              toOffset,
                                              toX,
                                              toY);
#endif // USE_BOX_DELTA
      }
      
      if (debug) {
        printf("CTI_MinimumSearch recalculated delta : %d\n", nDelta);
      }

#if defined(DEBUG)
      if (results.count("recalcDeltaIsSmaller") == 0) {
        results["recalcDeltaIsSmaller"] = 0;
      }
      if (results.count("recalcDeltaIsEqual") == 0) {
        results["recalcDeltaIsEqual"] = 0;
      }
      if (results.count("recalcDeltaIsLarger") == 0) {
        results["recalcDeltaIsLarger"] = 0;
      }
      if (results.count("recalcAndReinsert") == 0) {
        results["recalcAndReinsert"] = 0;
      }
#endif // DEBUG
      
#if defined(DEBUG)
      if (nDelta < oDelta) {
        results["recalcDeltaIsSmaller"] += 1;
      } else if (nDelta == oDelta) {
        results["recalcDeltaIsEqual"] += 1;
      } else if (nDelta > oDelta) {
        results["recalcDeltaIsLarger"] += 1;
      }
#endif // DEBUG
      
      bool isIdentical = (nDelta == 0);
      
      if (nDelta > oDelta) {
#if defined(DEBUG)
        results["recalcAndReinsert"] += 1;
#endif // DEBUG
        
        CoordDelta recalcDelta = CoordDelta(fromX, fromY, toX, toY, isHorizontal);
        
        ctiStruct.addToWaitList(recalcDelta, nDelta);
        
        // Continue to restart the min search loop
        
        continue;
      }
    }
    
    *smallestPtr = minCD;
    break;
  }
  } // while find min loop
  
  if (debug) {
    printf("CTI_MinimumSearch returning : %s\n", smallestPtr->toString().c_str());
  }
  
  return;
}

/*

// Util function that takes care of counter updates

static inline
void
CTI_UpdateCounters(
                   vector<uint32_t> & hCounters,
                   vector<uint32_t> & vCounters,
                   const int hCounterForCacheRow,
                   const int vCounterForCacheCol,
                   const int col,
                   const int row,
                   const int regionWidth,
                   const int regionHeight,
#if defined(DEBUG)
                   vector<uint8_t> & processedFlags,
#endif // DEBUG
                   const bool debug
                   )

{
  // Update horizontal counter based on vertical counters
  
  int counter;
  
  counter = hCounterForCacheRow;
  
  if (counter == col) {
    // Keep moving the horizontal counter up as long as the specific
    // pixel has been processed to consume any already processed
    // elements.
    
    hCounters[row] += 1;
    
    if (debug) {
      printf("update counter for row %d to %d\n", row, hCounters[row]);
    }
    
    for ( int targetCol = col + 1; targetCol < regionWidth; targetCol++ ) {
      bool wasTargetPixelProcessed = CTI_WasColProcessed(
#if defined(DEBUG)
                                                         regionWidth,
                                                         regionHeight,
                                                         processedFlags,
#endif // DEBUG
                                                         vCounters,
                                                         targetCol,
                                                         row);
      
      if (wasTargetPixelProcessed == false) {
        // Stop when an unprocessed element is found
        break;
      } else {
        hCounters[row] += 1;
        
        if (debug) {
          printf("update counter for row %d to %d\n", row, hCounters[row]);
        }
      }
    }
    
  } else {
    if (debug) {
      printf("did not update counter for row %d (was %d)\n", row, counter);
    }
  }
  
  // Update vertical counter based on horizontal counters
  
  counter = vCounterForCacheCol;
  
  if (counter == row) {
    // Keep moving the vertical counter up as long as the specific
    // pixel has been processed to consume any already processed
    // elements.
    
    vCounters[col] += 1;
    
    if (debug) {
      printf("update counter for col %d to %d\n", col, vCounters[col]);
    }
    
    for ( int targetRow = row + 1; targetRow < regionHeight; targetRow++ ) {
      bool wasTargetPixelProcessed = CTI_WasRowProcessed(
#if defined(DEBUG)
                                                         regionWidth,
                                                         regionHeight,
                                                         processedFlags,
#endif // DEBUG
                                                         hCounters,
                                                         col,
                                                         targetRow);
      
      if (wasTargetPixelProcessed == false) {
        // Stop when an unprocessed element is found
        break;
      } else {
        vCounters[col] += 1;
        
        if (debug) {
          printf("update counter for col %d to %d\n", col, vCounters[col]);
        }
      }
    }
  } else {
    if (debug) {
      printf("did not update counter for col %d (was %d)\n", col, counter);
    }
  }
  
  return;
}

 */
 
// One step of the iteration logic, each step will lookup
// the min delta and then process that min delta.

static inline
bool CTI_IterateStep(CTI_Struct & ctiStruct,
                     const bool tablePred,
                     vector<uint32_t> & iterOrder,
                     uint32_t * const deltasPtr)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_IterateStep %d\n", (int)iterOrder.size());
  }
  
  int regionWidth = ctiStruct.width;
  int regionHeight = ctiStruct.height;
  
  if (debug) {
    // Print matrix of processed flags
    printf("processed:\n");
    for ( int y = 0; y < regionHeight; y++ ) {
      for ( int x = 0; x < regionWidth; x++ ) {
        uint8_t flag = ctiStruct.wasProcessed(x, y);
        printf("%3d", (int)flag);
        assert(flag == 0 || flag == 1);
      }
      printf("\n");
    }
  }
  
  // Grab smallest delta
  
  CoordDelta minDelta;
  
  CTI_MinimumSearch(ctiStruct,
                    tablePred,
                    &minDelta);
  
  if (minDelta.isEmpty()) {
    return false;
  }
  
  if ((1)) {
    // FIXME: print processed at start of each loop?
    
    int iterOffset = CTIOffset2d(minDelta.toX(), minDelta.toY(), ctiStruct.width);
    
    if (debug) {
      printf("CTI_IterateStep starting point %d : (x,y) (%d,%d)\n", iterOffset, minDelta.toX(), minDelta.toY());
    }
    
    if (debug) {
      if (minDelta.isHorizontal()) {
        printf("Horizontal delta\n");
      } else {
        printf("Vertical   delta\n");
      }
    }
    
    // Generate next offset using only addition
    
    int nextIterOffset;
    int col = minDelta.toX();
    int row = minDelta.toY();
    
    if (minDelta.isHorizontal()) {
      nextIterOffset = iterOffset + 1;
      
#if defined(DEBUG)
      assert(col < regionWidth);
#endif // DEBUG
      
      col += 1;
      
#if defined(DEBUG)
      assert(col < regionWidth);
#endif // DEBUG
    } else {
      nextIterOffset = iterOffset + regionWidth;
      
#if defined(DEBUG)
      assert(row < regionHeight);
#endif // DEBUG
      
      row += 1;
      
#if defined(DEBUG)
      assert(row < regionHeight);
#endif // DEBUG
    }
    
    // The (x,y) that will be processed must not be one of the first 4 in upper left
    
#if defined(DEBUG)
    if (col < 2 && row < 2) {
      assert(0);
    }
#endif // DEBUG
    
    if (debug) {
      printf("iter append %d\n", nextIterOffset);
    }
    
//    if (nextIterOffset == 2) {
//      printf("target %d\n", nextIterOffset);
//    }
    
//    if (col == 6 && row == 9) {
//      printf("target\n");
//    }
    
    iterOrder.push_back(nextIterOffset);
    
    // In the case that an output deltas pointer is defined, generate a prediction
    // pixel and then generate a simple component delta
    
    if (deltasPtr != nullptr) {
      // Predict (R, G, B) using box read logic and generate ave pixel value
      // based on the neighbors.
      
      if ((0)) {
        uint32_t predPixel;
                
        if (minDelta.isHorizontal()) {
          predPixel = CTI_BoxPredictH(ctiStruct,
                                      tablePred,
                                      col, row);
        } else {
          predPixel = CTI_BoxPredictV(ctiStruct,
                                      tablePred,
                                      col, row);
        }
        
        uint32_t actualPixel = ctiStruct.pixelLookup(tablePred, nextIterOffset);
        
        // Generate actual prediction delta by reading the actual pixel value
        // at the offset being predicted and then generating a delta between the
        // predicted value and the actual value.
        
        uint32_t deltaPixel = pixel_component_delta(predPixel, actualPixel, 3);
        
        if (debug) {
          printf("pred   0x%08X\n", predPixel);
          printf("actual 0x%08X\n", actualPixel);
          printf("delta  0x%08X\n", deltaPixel);
          
          // FIXME: display in terms of signed offsets also
          
          {
            uint32_t B = deltaPixel & 0xFF;
            uint32_t G = (deltaPixel >> 8) & 0xFF;
            uint32_t R = (deltaPixel >> 16) & 0xFF;
            
            printf("delta  (%d %d %d)\n", unsigned_byte_to_signed(R), unsigned_byte_to_signed(G), unsigned_byte_to_signed(B));
          }
        }
        
        deltasPtr[nextIterOffset] = deltaPixel;
      } else {
        uint32_t predPixel = CTI_NeighborPredict2(ctiStruct,
                                                 tablePred,
                                                  deltasPtr,
                                                 col, row);
        
        uint32_t actualPixel = ctiStruct.pixelLookup(tablePred, nextIterOffset);
        
        // Generate actual prediction delta by reading the actual pixel value
        // at the offset being predicted and then generating a delta between the
        // predicted value and the actual value.
        
        uint32_t deltaPixel = pixel_component_delta(predPixel, actualPixel, 3);
        
        if (debug && deltaPixel != 0) {
          printf("pred   0x%08X\n", predPixel);
          printf("actual 0x%08X\n", (actualPixel & 0x00FFFFFF));
          printf("delta  0x%08X\n", deltaPixel);
          printf("done\n");
        }
        
        deltasPtr[nextIterOffset] = deltaPixel;
      }
    }
    
    // Mark this offset as processed, update row and col counters
    // that correspond to this specific offset. Note that
    // processedFlags is used only in DEBUG mode to double
    // check the col and row results.
    
    {
#if defined(DEBUG)
      {
        bool nextPixelWasProcessed = ctiStruct.wasProcessed(col, row);
        assert(nextPixelWasProcessed == false);
      }
      
      assert(ctiStruct.wasProcessed(col, row) == false);
#endif // DEBUG
      
      //ctiStruct.setProcessed(col, row);
      ctiStruct.setProcessed(nextIterOffset);
      
#if defined(DEBUG)
      {
        bool nextPixelWasProcessed = ctiStruct.wasProcessed(col, row);
        assert(nextPixelWasProcessed == true);
      }
#endif // DEBUG
      
      if (debug) {
        printf("offset %d -> (x, y) (%d, %d)\n", nextIterOffset, col, row);
      }
      
      int cacheRow = row;
      int cacheCol = col;
      
#if defined(DEBUG)
      {
        assert(cacheCol >= 0);
        assert(cacheCol < regionWidth);
        assert(cacheRow >= 0);
        assert(cacheRow < regionHeight);
      }
#endif // DEBUG
      
      if (debug) {
        // Print matrix of processed flags
        printf("processed:\n");
        for ( int y = 0; y < regionHeight; y++ ) {
          for ( int x = 0; x < regionWidth; x++ ) {
            uint8_t flag = ctiStruct.wasProcessed(x, y);
            printf("%3d", (int)flag);
            assert(flag == 0 || flag == 1);
          }
          printf("\n");
        }
      }
      
      // Update the deltas based on the newly discovered pixels *after* generating
      // a delta pixel based on the prediction. This prevents a delta update from
      // accidently being included in the prediction.
      
#if defined(USE_BOX_DELTA)
//      if (tablePred) {
//        ctiStruct.updateCache(tablePred, cacheCol, cacheRow);
//      } else {
//        ctiStruct.updateRowCaches(tablePred, cacheCol, cacheRow);
//      }
      ctiStruct.updateCache(tablePred, cacheCol, cacheRow);

//      if (cacheCol == 6 && cacheRow == 9) {
//        printf("target\n");
//      }
      
#if defined(BOX_DELTA_SUM_WITH_CACHE)
      ctiStruct.invalidateRowCache(
                                   true,
                                   cacheCol,
                                   cacheRow);
      ctiStruct.invalidateRowCache(
                                   false,
                                   cacheCol,
                                   cacheRow);
#endif // BOX_DELTA_SUM_WITH_CACHE
#endif // USE_BOX_DELTA
      
      // Add vertical prediction that extends from this newly processed pixel down
      
      // If pred 3 did not require that the above value be defined, then
      // it would be possible to kick off a new vertical splay as soon
      // as an X is generated.
      
      // More generally, the search has to follow even a 1 pixel path
      // so that the prediction can be more generic. Assuming that 2 rows
      // or cols are required is too restrictive.
      
      // FIXME: what about filling row zero case when 1 is already filled?

      bool processNextV = false;
      int fromY = -1;
      int toY = -1;
      
      {
      
      bool prevRowWasProcessed = false;
      if (cacheRow > 0) {
        prevRowWasProcessed = ctiStruct.wasProcessed(cacheCol, cacheRow-1);
      }

      bool shouldProcessNextRow = false;
      if ((cacheRow + 1) < regionHeight) {
        bool nextRowWasProcessed = ctiStruct.wasProcessed(cacheCol, cacheRow+1);
        if (!nextRowWasProcessed) {
          shouldProcessNextRow = true;
        }
      }
      
      if (prevRowWasProcessed && shouldProcessNextRow) {
        // Process (2,1) where there is a prev and next is not processed
        //
        // 0 1 1
        // 0 1 ?
        // 0 0 0
        
        processNextV = true;
        fromY = cacheRow - 1;
        toY = cacheRow;
      } else if (!shouldProcessNextRow) {
        // Process (2,0) where next is and next+1 is not
        //
        // 0 1 ?
        // 0 1 1
        // 0 0 0
        
        // Process (2,1) where prev is not processed, next is, next+1 is not
        //
        // 0 1 0
        // 0 1 ?
        // 0 0 1
        // 0 0 0
        
        if ((cacheRow + 2) < regionHeight) {
          bool rowWasProcessed = ctiStruct.wasProcessed(cacheCol, cacheRow+2);
          if (!rowWasProcessed) {
            processNextV = true;
            fromY = cacheRow;
            toY = cacheRow + 1;
          }
        }
      }
      
      }
        
      // FIXME: When should not process because next row
      // is set, but the one after that is not set then
      // this should skip ahead and init. Same for cols.
      
      if (processNextV) {
        // New V pred with (col,row-1) as other
        
#if defined(DEBUG)
        assert(fromY != -1);
        assert(toY != -1);
        assert(fromY < regionHeight);
        assert(toY < regionHeight);
#endif // DEBUG
        
        int fromOffset = CTIOffset2d(cacheCol, fromY, regionWidth);
        int toOffset = CTIOffset2d(cacheCol, toY, regionWidth);
        
#if defined(USE_BOX_DELTA)
        int predY = toY + 1;
        
        int delta = CTI_BoxDeltaPredictV(ctiStruct,
                                    tablePred,
                                    cacheCol,
                                    predY);
#else
        int otherCol = -1;
        
        if (cacheCol == 0) {
          // Ignore other
        } else {
          // Other is (col - 1) for all other rows
          otherCol = cacheCol - 1;
        }
        
        // In the case where pixel otherOffset has not been set yet, ignore in delta calc
        
        int delta = CTI_PredictDelta3(ctiStruct.pixelsPtr,
                                      ctiStruct.colortablePixelsPtr,
                                      ctiStruct.colortableNumPixels,
                                      ctiStruct.tableOffsetsPtr,
                                      tablePred,
                                      ctiStruct.width,
                                      ctiStruct.height,
                                      ctiStruct,
                                      fromOffset,
                                      toOffset,
                                      otherCol,
                                      toY);
#endif // USE_BOX_DELTA
        
        bool isHorizontal = false;
        
        CoordDelta coordDelta = CoordDelta(cacheCol, fromY, cacheCol, toY, isHorizontal);
        
//        if (nextIterOffset == 18) {
//          printf("target\n");
//        }
        
        ctiStruct.addToWaitList(coordDelta, delta);
        
        if (debug) {
          printf("add V pred (%d,%d) -> (%d,%d) : (%d -> %d) : delta %d\n", cacheCol, fromY, cacheCol, toY, fromOffset, toOffset, delta);
        }
        
#if defined(DEBUG)
        ctiStruct.results["addVCalc3"] += 1;
#endif // DEBUG
      }
      
      // FIXME: in the case where a col+2 or row+2 add step is done, be sure to add the closer pixel
      // predictions at col+1 or row+1 to the wait list *after* adding the ones that a farther away
      // since the implicit ordering should favor the closer ones ?

      // Add horizontal prediction that extends from this newly processed pixel down
      
      bool processNextH = false;
      int fromX = -1;
      int toX = -1;
      
      {
      
      bool prevColWasProcessed = false;
      if (cacheCol > 0) {
        prevColWasProcessed = ctiStruct.wasProcessed(cacheCol-1, cacheRow);
      }
      
      bool shouldProcessNextCol = false;
      if ((cacheCol + 1) < regionWidth) {
        bool nextColWasProcessed = ctiStruct.wasProcessed(cacheCol+1, cacheRow);
        if (!nextColWasProcessed) {
          shouldProcessNextCol = true;
        }
      }
        
      if (prevColWasProcessed && shouldProcessNextCol) {
        processNextH = true;
        fromX = cacheCol - 1;
        toX = cacheCol;
      } else if (!shouldProcessNextCol) {
        if ((cacheCol + 2) < regionWidth) {
          bool colWasProcessed = ctiStruct.wasProcessed(cacheCol+2, cacheRow);
          if (!colWasProcessed) {
            processNextH = true;
            fromX = cacheCol;
            toX = cacheCol + 1;
          }
        }
      }
        
      }
      
      if (processNextH)
      {
        // New H pred with (col,row-1) as other
        
#if defined(DEBUG)
        assert(fromX != -1);
        assert(toX != -1);
        assert(fromX < regionWidth);
        assert(toX < regionWidth);
#endif // DEBUG
        
        int fromOffset = CTIOffset2d(fromX, cacheRow, regionWidth);
        int toOffset = CTIOffset2d(toX, cacheRow, regionWidth);
        
#if defined(USE_BOX_DELTA)
        int predX = toX + 1;
        
        // In the case where pixel otherOffset has not been set yet, ignore in delta calc
        
        int delta = CTI_BoxDeltaPredictH(ctiStruct,
                                    tablePred,
                                    predX,
                                    cacheRow);
#else
        int otherRow = -1;
        
        if (cacheRow == 0) {
          // Ignore other
        } else {
          // Other is (row - 1) for all other rows
          otherRow = cacheRow - 1;
        }
        
        // In the case where pixel otherOffset has not been set yet, ignore in delta calc
        
        int delta = CTI_PredictDelta3(ctiStruct.pixelsPtr,
                                      ctiStruct.colortablePixelsPtr,
                                      ctiStruct.colortableNumPixels,
                                      ctiStruct.tableOffsetsPtr,
                                      tablePred,
                                      ctiStruct.width,
                                      ctiStruct.height,
                                      ctiStruct,
                                      fromOffset,
                                      toOffset,
                                      toX,
                                      otherRow);
#endif // USE_BOX_DELTA
        
        bool isHorizontal = true;
        
        CoordDelta coordDelta = CoordDelta(fromX, cacheRow, toX, cacheRow, isHorizontal);
        
        ctiStruct.addToWaitList(coordDelta, delta);
        
        if (debug) {
          printf("add H pred (%d,%d) -> (%d,%d) : (%d -> %d) : delta %d\n", fromX, cacheRow, toX, cacheRow, fromOffset, toOffset, delta);
        }
        
#if defined(DEBUG)
        ctiStruct.results["addHCalc3"] += 1;
#endif // DEBUG
      }
      
      /*
      
      // Note that the cache contents are invalid between when min is invoked
      // and the cache has been invalidated.
      
      bool extendRow = false;
      bool extendCol = false;
      
      // Save col and row counters before advancing
      
      int hCounterForCacheRow = hCounters[cacheRow];
      int vCounterForCacheCol = vCounters[cacheCol];
      
      // Update counters before row and col invalidation
      // since these depend on the counters
      
      CTI_UpdateCounters(
                         hCounters,
                         vCounters,
                         hCounterForCacheRow,
                         vCounterForCacheCol,
                         col,
                         row,
                         regionWidth,
                         regionHeight,
#if defined(DEBUG)
                         processedFlags,
#endif // DEBUG
                         debug
                         );
      
      // Invalidate and reinit row cache
      
      if (cacheCol == hCounterForCacheRow) {
        extendRow = CTI_InvalidateRowCache(pixelsPtr,
                                           colortablePixelsPtr,
                                           colortableNumPixels,
                                           tableOffsetsPtr,
                                           tablePred,
                                           regionWidth,
                                           regionHeight,
                                           ctiStruct,
                                           cacheCol,
                                           cacheRow,
                                           true,
                                           false,
                                           !minDelta.isHorizontal());

      } else {
        if (debug) {
          printf("delta cache not cleared for row %d : hCounterForCacheRow != cacheCol : %d != %d\n", cacheRow, hCounterForCacheRow, cacheCol);
        }
      }
      
      // Check for special case where there are more than 2 rows and the
      // column being processed is 0 or 1. A new cached row may need to
      // be started in this case.
      
      if ((cacheCol == 0 || cacheCol == 1) && (cacheCol < (regionWidth-1)) &&
          (cacheRow >= 2) &&
          (hCounterForCacheRow == 0 || hCounterForCacheRow == 1)) {
        if (debug) {
          printf("row delta possible init case (%d,%d)\n", cacheCol, cacheRow);
        }
        
#if defined(DEBUG)
        assert(cacheRow <= regionHeight);
#endif // DEBUG
        
        bool doInit = false;
        
        if ((cacheCol == 1) && (hCounterForCacheRow == 1)) {
          // Col 0 has already been processed
          doInit = true;
        } else if (cacheCol == 0) {
          // When processing col 0, determine if col 1 has been
          // processed already by looking at col 1 counter.
          
          bool nextColProcessed = CTI_WasColProcessed(
#if defined(DEBUG)
                                                      regionWidth,
                                                      regionHeight,
                                                      processedFlags,
#endif // DEBUG
                                                      vCounters,
                                                      1,
                                                      cacheRow);

          if (nextColProcessed) {
            doInit = true;
          }
        }
        
        if (doInit) {
          int otherCol = 2;
          
          if (otherCol >= regionWidth) {
            doInit = false;
          }
        }
        
        if (doInit) {
          extendRow = true;
        }
      }
      
      // FIXME: add a new cache entry to replace the one that was removed.
      // But then need to invalidate the other one, so if a col was choosen
      // then the row at that col value should be invalidated.
      
      if (cacheRow == vCounterForCacheCol) {
        extendCol = CTI_InvalidateColCache(pixelsPtr,
                                           colortablePixelsPtr,
                                           colortableNumPixels,
                                           tableOffsetsPtr,
                                           tablePred,
                                           regionWidth,
                                           regionHeight,
                                           ctiStruct,
                                           cacheCol,
                                           cacheRow,
                                           true,
                                           false,
                                           minDelta.isHorizontal());
      } else {
        if (debug) {
          printf("delta cache not cleared for col %d : counter != cacheRow : %d != %d\n", cacheCol, vCounterForCacheCol, cacheRow);
        }
      }
      
      // Special case for rows 0 or 1 and more than 2 columns
      
      if ((cacheRow == 0 || cacheRow == 1) && (cacheRow < (regionHeight-1)) &&
          (cacheCol >= 2) &&
          (vCounterForCacheCol == 0 || vCounterForCacheCol == 1)) {
        if (debug) {
          printf("col delta possible init case (%d,%d)\n", cacheCol, cacheRow);
        }
        
        assert(cacheCol <= regionWidth);
        
        bool doInit = false;
        
        if ((cacheRow == 1) && (vCounterForCacheCol == 1)) {
          // Row 0 has already been processed
          doInit = true;
        } else if (cacheRow == 0) {
          // When processing row 0, determine if row 1 has been
          // processed already by looking at row 1 counter.
          
          bool nextRowProcessed = CTI_WasRowProcessed(
#if defined(DEBUG)
                                                      regionWidth,
                                                      regionHeight,
                                                      processedFlags,
#endif // DEBUG
                                                      hCounters,
                                                      cacheCol,
                                                      1);
          
          if (nextRowProcessed) {
            doInit = true;
          }
        }
        
        // If it look like an init should be done, check for the case
        // where the value at +2 was already processed.
        
        if (doInit) {
          // do the row init only in the case where row=2 has not been processed
          int otherRow = 2;
          
          if (otherRow >= regionHeight) {
            doInit = false;
          }
        }
      
        if (doInit) {
          extendCol = true;
        }
      }
      
      if (1 && debug) {
        // Print cached delta for existing H or V
        
        printf("hCache contents (\n");
        for ( int i = 0; i < regionHeight; i++ ) {
          auto it = hCache[i];
          CoordDelta cd;
          if (it != end(ctiStruct.waitList)) {
            cd = *it;
          }
          printf(" %s\n", cd.toString().c_str());
        }
        printf(")\n");
        
        printf("vCache contents (\n");
        for ( int i = 0; i < regionWidth; i++ ) {
          auto it = vCache[i];
          CoordDelta cd;
          if (it != end(ctiStruct.waitList)) {
            cd = *it;
          }
          printf(" %s\n", cd.toString().c_str());
        }
        printf(")\n");
      }
      
#if defined(DEBUG)
      // col/row counter checks should not see this (x,y) coord as processed
      {
        bool nextPixelWasProcessed = CTI_WasRowOrColProcessed(
#if defined(DEBUG)
                                                              regionWidth,
                                                              regionHeight,
                                                              processedFlags,
#endif // DEBUG
                                                              hCounters,
                                                              vCounters,
                                                              col,
                                                              row);
        
        assert(nextPixelWasProcessed == true);
      }
#endif // DEBUG
      
      // Invalidate a horizontally cached column at row+1 if needed
      
      int otherRow = cacheRow + 1;
      if (otherRow < regionHeight) {
        int colFromOtherRow = hCounters[otherRow] - 1;
        if (cacheCol == colFromOtherRow) {
          CTI_InvalidateRowCache(pixelsPtr,
                                 colortablePixelsPtr,
                                 colortableNumPixels,
                                 tableOffsetsPtr,
                                 tablePred,
                                 regionWidth,
                                 regionHeight,
#if defined(DEBUG)
                                 processedFlags,
#endif // DEBUG
                                 hCounters,
                                 vCounters,
                                 hvTree,
                                 hCache,
                                 vCache,
                                 cacheCol,
                                 otherRow,
                                 false,
                                 true);
        }
      }
      
      // Invalidate a vertically cached row at col+1 if needed
      
      int otherCol = cacheCol + 1;
      if (otherCol < regionWidth) {
        int rowFromOtherCol = vCounters[otherCol] - 1;
        if (cacheRow == rowFromOtherCol) {
          CTI_InvalidateColCache(pixelsPtr,
                                 colortablePixelsPtr,
                                 colortableNumPixels,
                                 tableOffsetsPtr,
                                 tablePred,
                                 regionWidth,
                                 regionHeight,
#if defined(DEBUG)
                                 processedFlags,
#endif // DEBUG
                                 hCounters,
                                 vCounters,
                                 hvTree,
                                 hCache,
                                 vCache,
                                 otherCol,
                                 cacheRow,
                                 false,
                                 true);
        }
      }
      
      // Check for row/col init after counter update
      
      if (extendRow) {
        int adjustedCol = hCounters[row];
        if (adjustedCol < regionWidth) {
          CTI_InitRowCache(pixelsPtr,
                           colortablePixelsPtr,
                           colortableNumPixels,
                           tableOffsetsPtr,
                           tablePred,
                           regionWidth,
                           regionHeight,
                           ctiStruct,
                           adjustedCol,
                           cacheRow);
          
          
        }
      }
      
      if (extendCol) {
        int adjustedRow = vCounters[col];
        if (adjustedRow < regionHeight) {
          CTI_InitColCache(pixelsPtr,
                           colortablePixelsPtr,
                           colortableNumPixels,
                           tableOffsetsPtr,
                           tablePred,
                           regionWidth,
                           regionHeight,
                           ctiStruct,
                           cacheCol,
                           adjustedRow);
        }
      }
       
      */
    }
  }
  
  return true;
}

// Initialize the first 4 pixel values in the upper left corner of the region.
// These blocks are explicitly marked as processed and the only step that
// is needed is to create cached delta values for the 4 pixels. Note that these
// cached values will not be invalidated since they only reference each other.

static inline
void CTI_InitBlock(const uint32_t * const pixelsPtr,
                   const uint32_t * const colortablePixelsPtr,
                   const int colortableNumPixels,
                   const uint8_t * const tableOffsetsPtr,
                   const bool tablePred,
                   const int regionWidth,
                   const int regionHeight,
                   CTI_Struct & ctiStruct,
                   vector<uint32_t> & iterOrder,
                   uint32_t * const deltasPtr)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_InitBlock\n");
  }
  
  assert(regionWidth >= 2);
  assert(regionHeight >= 2);
  
  int toOffset;
  int fromOffset;
  int delta;
  
#if defined(DEBUG)
  assert(ctiStruct.isWaitListEmpty() == true);
#endif // DEBUG
  
  typedef struct {
    int rowOrCol;
    CoordDelta cd;
    int delta;
  } InitRCD;
  
  vector<InitRCD> initOrder;
  initOrder.reserve(4);
  
  // FIXME: reorder so that row 0 defaults to being processed before row 1
  
  // Check each horizontal line
  
  for ( int y = 0; y < 2; y++ ) {
    if (debug) {
      printf("H row check : y %2d\n", y);
    }
    
    // Do not consider row unless there are at least 2 values to predict from and width is at least 3
    
    {
      int maxCol = 3;
      if (regionWidth < maxCol) {
        maxCol = regionWidth;
      }
      
      for ( int x = 2; x < maxCol; x++ ) {
        toOffset = CTIOffset2d(x, y, regionWidth);
        
        if (debug) {
          printf("H (x,y) (%2d,%2d) : offset %d\n", x, y, toOffset);
        }
        
        {
          assert(x >= 2);
          
          int toX = x - 1;
          int fromX = x - 2;
          
          toOffset = CTIOffset2d(toX, y, regionWidth);
          fromOffset = CTIOffset2d(fromX, y, regionWidth);
          
          if (debug) {
            printf("found   horizontal line (%d,%d) -> (%d,%d) : (%d -> %d)\n", fromX, y, toX, y, fromOffset, toOffset);
          }
          
          if (1) {
            if (debug) {
              printf("uncached horizontal line (%d,%d) -> (%d,%d) : (%d -> %d)\n", fromX, y, toX, y, fromOffset, toOffset);
            }
            
#if defined(DEBUG)
            int otherOffset;
#endif // DEBUG
            int otherRow;
            
            if (y == 0) {
              // Other is row=1 in the first row
              otherRow = 1;
#if defined(DEBUG)
              if (otherRow < regionHeight) {
                otherOffset = CTIOffset2d(toX, otherRow, regionWidth);
              } else {
                assert(0);
              }
#endif // DEBUG
            } else {
              // Other is (row - 1) for all other rows
              otherRow = y-1;
            }
            
            delta = CTI_PredictDelta3(pixelsPtr,
                                     colortablePixelsPtr,
                                     colortableNumPixels,
                                     tableOffsetsPtr,
                                     tablePred,
                                     regionWidth,
                                     regionHeight,
                                      ctiStruct,
                                     fromOffset,
                                     toOffset,
                                     toX,
                                     otherRow);
            
            bool isHorizontal = true;
            
            CoordDelta coordDelta = CoordDelta(fromX, y, toX, y, isHorizontal);
            
            InitRCD irc;
            irc.rowOrCol = y;
            irc.cd = coordDelta;
            irc.delta = delta;
            initOrder.push_back(irc);
          }
        }
      }
    }
  }
  
  // Check each vertical line
  
  for ( int x = 0; x < 2; x++ ) {
    if (debug) {
      printf("V col check : x %2d\n", x);
    }
    
    {
      int maxRow = 3;
      if (regionHeight < maxRow) {
        maxRow = regionHeight;
      }
      
      for ( int y = 2; y < maxRow; y++ ) {
        toOffset = CTIOffset2d(x, y, regionWidth);
        
        if (debug) {
          printf("V (x,y) (%2d,%2d) : offset %d\n", x, y, toOffset);
        }
        
        {
          // Update the counter for this specific row
          
          if (debug) {
            printf("update counter for vCounters[%d] = %d\n", x, y);
          }
          
          assert(y >= 2);
          
          int toY = y - 1;
          int fromY = y - 2;
          
          toOffset = CTIOffset2d(x, toY, regionWidth);
          fromOffset = CTIOffset2d(x, fromY, regionWidth);
          
          if (debug) {
            printf("found vertical line (%d,%d) -> (%d,%d) : (%d -> %d)\n", x, fromY, x, toY, fromOffset, toOffset);
          }
          
          {
            if (debug) {
              printf("uncached vertical line (%d,%d) -> (%d,%d) : (%d -> %d)\n", x, fromY, x, toY, fromOffset, toOffset);
            }
            
            int otherOffset;
            int otherCol;
            
            if (x == 0) {
              // Other is 1 for the first col
              otherCol = 1;
              if (otherCol < regionWidth) {
                otherOffset = CTIOffset2d(otherCol, toY, regionWidth);
              } else {
                assert(0);
              }
            } else {
              // Other is (col - 1) for all other rows
              otherCol = x-1;
              otherOffset = CTIOffset2d(otherCol, toY, regionWidth);
            }
            
            delta = CTI_PredictDelta3(pixelsPtr,
                                     colortablePixelsPtr,
                                     colortableNumPixels,
                                     tableOffsetsPtr,
                                     tablePred,
                                     regionWidth,
                                     regionHeight,
                                      ctiStruct,
                                     fromOffset,
                                     toOffset,
                                     otherCol,
                                     toY);
            
            bool isHorizontal = false;
            
            CoordDelta coordDelta = CoordDelta(x, fromY, x, toY, isHorizontal);
            
            InitRCD irc;
            irc.rowOrCol = y;
            irc.cd = coordDelta;
            irc.delta = delta;
            initOrder.push_back(irc);
          }
        }
      }
    }
  }
  
  // In order  (r0 r1 c0 c1)
  // Out order (r0 c0 r1 c1) <-> (c1 r1 c0 r0)
  
  // Init as row 0, col 0, row 1, col 1
  
  if (initOrder.size() == 4) {
    swap(initOrder[1], initOrder[2]);
    
    vector<InitRCD> tmp;
    tmp.reserve(4);
    
    for ( auto it = initOrder.rbegin(); it != initOrder.rend(); it++ )
    {
      auto & irc = *it;
      tmp.push_back(irc);
    }
    
    initOrder = tmp;
  }
  
  for ( auto & irc : initOrder )
  {
    CoordDelta coordDelta = irc.cd;
    ctiStruct.addToWaitList(coordDelta, irc.delta);
  }
  
  // Verify that init block has set the cached values for the top left 4 pixels
  
  pair<int,int> pairs[] = {
    pair<int,int>(0,0),
    pair<int,int>(1,0),
    pair<int,int>(0,1),
    pair<int,int>(1,1)
  };
  
  for ( auto pair : pairs) {
    int x = pair.first;
    int y = pair.second;
  
#if defined(USE_BOX_DELTA)
    ctiStruct.updateCache(tablePred, x, y);
#endif // USE_BOX_DELTA
    
    int fromOffset = CTIOffset2d(x, y, ctiStruct.width);
    iterOrder.push_back(fromOffset);
    ctiStruct.setProcessed(x, y);
    
    if (deltasPtr && !tablePred) {
      // Emit upper 4 corner pixels directly without a delta
      deltasPtr[fromOffset] = pixelsPtr[fromOffset];
    } else if (deltasPtr && tablePred) {
      // Emit upper 4 corner pixels directly without a delta
      deltasPtr[fromOffset] = colortablePixelsPtr[tableOffsetsPtr[fromOffset]];
    }
  }
  
  if (debug) {
    printf("CTI_InitBlock returning\n");
  }
  
  return;
}

// Fill in memory associated with a CTI_Struct for
// a given width and height.

static inline
void CTI_Setup(CTI_Struct & ctiStruct,
               const uint32_t * const pixelsPtr,
               const uint32_t * const colortablePixelsPtr,
               const int colortableNumPixels,
               const uint8_t * const tableOffsetsPtr,
               const bool tablePred,
               const int width,
               const int height,
               vector<uint32_t> & iterOrder,
               uint32_t * const deltasPtr)
{
  const bool debug = false;
  //const bool debugDumpCopiedOffsets = false;
  
  if (debug) {
    printf("CTI_Setup\n");
  }
  
  const int regionNumPixels = width * height;
  assert(regionNumPixels > 0);
  
  //int fromOffset;
  
  // Direct pixel by pixel prediction
  
  /*
  
  vector<uint32_t> & regionPixels = ctiStruct.regionPixels;
  
  if (pixelsPtr) {
    assert(tableOffsetsPtr == nullptr);
    
    if (regionPixels.size() == regionNumPixels) {
      // Reuse case where region size has not changed
    } else {
      // Reuse case where region size has changed, or size was zero
      regionPixels = vector<uint32_t>(regionNumPixels);
    }
    
    const int maxX = originX + regionWidth;
    const int maxY = originY + regionHeight;
    int flati = 0;
    
    for ( int y = originY; y < maxY; y++ ) {
      int regionRowOffset = (y * bufferWidth);
      
      for ( int x = originX; x < maxX; x++ ) {
        int regionOffset = regionRowOffset + x;
        regionPixels[flati++] = pixelsPtr[regionOffset];
      }
    }
    
#if defined(DEBUG)
    assert(flati == regionNumPixels);
#endif // DEBUG
    
    if (debugDumpCopiedOffsets) {
      for ( int y = 0; y < regionHeight; y++ ) {
        for ( int x = 0; x < regionWidth; x++ ) {
          int offset = (y * regionWidth) + x;
          printf("0x%08X ", regionPixels[offset]);
        }
        printf("\n");
      }
    }
  }
    
  // Table prediciton
  
  vector<uint8_t> & regionTableOffsets = ctiStruct.regionTableOffsets;
  
  if (tableOffsetsPtr) {
    assert(pixelsPtr == nullptr);
    
    if (regionTableOffsets.size() == regionNumPixels) {
      // Reuse case where region size has not changed
    } else {
      // Reuse case where region size has changed, or size was zero
      regionTableOffsets = vector<uint8_t>(regionNumPixels);
    }
    
    const int maxX = originX + regionWidth;
    const int maxY = originY + regionHeight;
    int flati = 0;
    
    for ( int y = originY; y < maxY; y++ ) {
      int regionRowOffset = (y * bufferWidth);
      
      for ( int x = originX; x < maxX; x++ ) {
        int regionOffset = regionRowOffset + x;
        regionTableOffsets[flati++] = tableOffsetsPtr[regionOffset];
      }
    }
    
#if defined(DEBUG)
    assert(flati == regionNumPixels);
#endif // DEBUG
    
    if (debugDumpCopiedOffsets) {
      for ( int y = 0; y < regionHeight; y++ ) {
        for ( int x = 0; x < regionWidth; x++ ) {
          int offset = (y * regionWidth) + x;
          printf("%d ", regionTableOffsets[offset]);
        }
        printf("\n");
      }
    }
  }
  
  */
  
  // Copy parameters
  
  ctiStruct.pixelsPtr = (uint32_t*) pixelsPtr;
  ctiStruct.colortablePixelsPtr = (uint32_t*) colortablePixelsPtr;
  ctiStruct.colortableNumPixels = colortableNumPixels;
  ctiStruct.tableOffsetsPtr = (uint8_t*)tableOffsetsPtr;

  ctiStruct.width = width;
  ctiStruct.height = height;
  
  // Iter order
  
  if (iterOrder.size() != regionNumPixels) {
    iterOrder.reserve(regionNumPixels);
  }
  iterOrder.clear();
  
  // The core data structures are a tree for O(logN) access to the smallest delta
  // and an array of iterator to support O(1) access to the cached delta for a
  // given row or column.
  
  if (tablePred) {
    // Max table size is one byte
    ctiStruct.initWaitList(255+1);
  } else {
    // 3 * byte deltas
    ctiStruct.initWaitList(255+255+255+1);
  }
  
  // FIXME: Phony init for 16 bit range for now
  
//  ctiStruct.initWaitList(0xFFFF);
  
  // Init deltas so that for a width of N there are (N-1)
  // deltas. The delta at offset 0 corresponds to the
  // delta between 0 and 1.
  
#if defined(USE_BOX_DELTA)
  ctiStruct.cachedHDeltaSums.allocValues(width, height, -1);
  ctiStruct.cachedVDeltaSums.allocValues(width, height, -1);
  
  ctiStruct.cachedHDeltaRows.allocValues(width, height, -1);
  ctiStruct.cachedVDeltaRows.allocValues(width, height, -1);
#endif // USE_BOX_DELTA
  
  // Processed flags indicate when a pixel has been "covered"
  
#if defined(DEBUG)
  assert(width >= 2);
  assert(height >= 2);
#endif // DEBUG
  
  vector<uint8_t> & processedFlags = ctiStruct.processedFlags;
  
  if (processedFlags.size() != regionNumPixels) {
    processedFlags = vector<uint8_t>(regionNumPixels);
    
    for ( int i = 0; i < regionNumPixels; i++ ) {
      processedFlags[i] = 0;
    }
  }
  
  CTI_InitBlock(pixelsPtr,
                colortablePixelsPtr,
                colortableNumPixels,
                tableOffsetsPtr,
                tablePred,
                width, height,
                ctiStruct,
                iterOrder,
                deltasPtr);
  
  return;
}

// Copy pixels starting from origin (x,y) up to width x height
// into a smaller buffer. Returns the number of pixels copied.
// Note that this method supports a copy into a buffer that
// is of a different width that the original buffer.

static inline
void CTI_Iterate(
                 const uint32_t * const pixelsPtr,
                 const uint32_t * const colortablePixelsPtr,
                 const int colortableNumPixels,
                 const uint8_t * const tableOffsetsPtr,
                 const bool tablePred,
                 const int width,
                 const int height,
                 vector<uint32_t> & iterOrder,
                 uint32_t * const deltasPtr)
{
  const bool debug = false;
  
  if (debug) {
    printf("CTI_Iterate\n");
  }
  
  CTI_Struct ctiStruct;
  
  CTI_Setup(ctiStruct,
            pixelsPtr,
            colortablePixelsPtr,
            colortableNumPixels,
            tableOffsetsPtr,
            tablePred,
            width,
            height,
            iterOrder,
            deltasPtr);
  
  // Iterate over all remaining pixels based on min cost huristic
  
  bool hasMoreDeltas;
  
  while (1) {
    hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                    tablePred,
                                    iterOrder,
                                    deltasPtr);
    
    if (!hasMoreDeltas) {
      break;
    }
  }
  
#if defined(DEBUG)
  if (ctiStruct.results.size() > 0) {
    printf("results:\n");
    
    ctiStruct.results["numPixels"] = width * height;
    
    int minRemove = ctiStruct.results["minRemove"];
    int multipleRemoves = minRemove - (width * height);
    ctiStruct.results["numRemovedOver"] = multipleRemoves;
    
    vector<string> keys;
    for ( auto pair : ctiStruct.results ) {
      keys.push_back(pair.first);
    }
    sort(begin(keys), end(keys));
    
    for ( string key : keys ) {
      int val = ctiStruct.results[key];
      printf("%20s = %8d\n", key.c_str(), val);
    }
    printf("results done:\n");
  }
#endif // DEBUG
  
  if (debug) {
    int N = (int) iterOrder.size();
    
    printf("N = %d\n", N);
    
    for ( int i = 0; i < N; i++ ) {
      printf("iter %d\n", iterOrder[i]);
    }
  }
  
#if defined(DEBUG)
  for ( int y = 0; y < height; y++ ) {
    for ( int x = 0; x < width; x++ ) {
      assert(ctiStruct.wasProcessed(x, y) == true);
    }
  }
#endif // DEBUG
  
  return;
}

// Util function that will set processed flags for a matrix as defined
// by the input boolean flags. This util method assumes that none
// of the original 4 pixels in the upper left corner will be touched
// and that the normal init logic has been executed.

static
void CTI_ProcessFlags(CTI_Struct & ctiStruct,
                      uint8_t *flagsPtr,
                      const bool tablePred)
{
  int width = ctiStruct.width;
  int height = ctiStruct.height;
  
  for ( int y = 0; y < height; y++ ) {
    for ( int x = 0; x < width; x++ ) {
      int offset = CTIOffset2d(x, y, width);
      
      if ((x < 2) && (y < 2)) {
        assert(ctiStruct.processedFlags[offset] == 1);
        // Skip topleft coords which must be set to 1
        continue;
      }
      
      if (flagsPtr[offset]) {
        assert(ctiStruct.processedFlags[offset] == 0);
        ctiStruct.processedFlags[offset] = 1;
#if defined(USE_BOX_DELTA)
        ctiStruct.updateCache(tablePred, x, y);
#endif // USE_BOX_DELTA
      } else {
        assert(ctiStruct.processedFlags[offset] == 0);
      }
    }
  }
}
