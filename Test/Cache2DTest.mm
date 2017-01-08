//
//  Cache2DTest.mm
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Tests for prediction logic based on 3 component RGB elements in a matrix.

#import <XCTest/XCTest.h>

#import "Cache2D.hpp"

#import <set>
#include <string>
#include <sstream>

#include <iostream>

using namespace std;

// Given a vector of values, format as a comma separated string

template <typename T>
string keyFromVec(const vector<T> & vec) {
  if (vec.size() == 0) {
    return "";
  }
  std::stringstream strstm;
  for (int i=0; i < vec.size() - 1; i++ ) {
    T v = vec[i];
    strstm << ((int)v) << ",";
  }
  strstm << ((int)vec[vec.size() - 1]);
  string key = strstm.str();
  return std::move(key);
}

template <typename T>
vector<T>
formatVec(T *ptr, int num)
{
  vector<T> vec;
  
  for ( int i = 0; i < num; i++ ) {
    vec.push_back(ptr[i]);
  }
  
  return std::move(vec);
}

template <typename T>
vector<vector<T> >
format2DVec(T *ptr, int width, int height)
{
  vector<vector<T> > vec2D;
  
  for ( int row = 0; row < height; row++ ) {
    int offset = (row * width) + 0;
    vector<T> vec = formatVec(&ptr[offset], width);
    vec2D.push_back(vec);
  }
  
  return std::move(vec2D);
}

template <typename T>
vector<vector<T> >
format2DVec(vector<T> & vec, int width, int height)
{
  vector<vector<T> > vec2D;
  
  for ( int row = 0; row < height; row++ ) {
    vector<T> rowVec;
    for ( int col = 0; col < width; col++ ) {
      int offset = (row * width) + col;
      rowVec.push_back(vec[offset]);
    }
    vec2D.push_back(rowVec);
  }
  
  return std::move(vec2D);
}

// Reorder block rows so that rows are sorted by initial column value.

@interface Cache2DTest : XCTestCase

@end

@implementation Cache2DTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void) testInvalidateHNop {
  Cache2DSum3<int16_t, true> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    -1,  -1,  -1,
    -1,  -1,  -1
  };
  
  // Output state
  
  int16_t expected[] = {
    -1, -1, -1,
    -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

- (void) testInvalidateVNop {
  Cache2DSum3<int16_t, false> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    -1,  -1,  -1,
    -1,  -1,  -1
  };
  
  // Output state
  
  int16_t expected[] = {
    -1, -1, -1,
    -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow1 {
  Cache2DSum3<int16_t, true> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,
    0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1, -1, -1,
    0,  0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (0,0) which should write to (0,0) (1,0) (2,0)
  
  cs.invalidate(0,0);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateVInvalidateRow1 {
  Cache2DSum3<int16_t, false> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state (transposed)
  
  int16_t input[] = {
    0,  0,
    0,  0,
    0,  0
  };
  
  // Expected output (transposed)
  
  int16_t expected[] = {
    -1, -1,
     0,  0,
     0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (0,0) which should write to (0,0) (0,1) (transposed)
  
  cs.invalidate(0,0);
  
  // Copy values with transposed width and height, note that values are not transposed
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, height, width);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, height, width);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow2 {
  Cache2DSum3<int16_t, true> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,
    0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1, -1, -1,
    0,  0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (1,0) which should write to (1,0) (2,0)
  
  cs.invalidate(1,0);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateVInvalidateRow2 {
  Cache2DSum3<int16_t, false> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,
    0,  0,
    0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1, -1,
    0,  0,
    0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalidate (0,1) which should write to transposed (1,0) which is offset 1
  
  cs.invalidate(0,1);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, height, width);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, height, width);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow3 {
  Cache2DSum3<int16_t, true> cs;
  
  // 3x2 cache of values
  
  int width = 3;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,
    0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    0,  -1, -1,
    0,  0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (2,0) which should write to (2,0)
  
  cs.invalidate(2,0);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 3x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateVInvalidateRow3 {
  Cache2DSum3<int16_t, false> cs;
  
  // 3x3 cache of values
  
  int width = 3;
  int height = 3;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,
    0,  0,  0,
    0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1, -1, -1,
    0,  0,  0,
    0,  0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // invalidate (0,1) which should write to offsets 1,2
  
  cs.invalidate(0,1);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 4x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow4 {
  Cache2DSum3<int16_t, true> cs;
  
  // 4x2 cache of values
  
  int width = 4;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,  0,
    0,  0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1,  -1,  -1,   0,
     0,   0,   0,   0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (0,0) which should write to (0,0) (1,0) (2,0)
  
  cs.invalidate(0,0);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 4x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateVInvalidateRow4 {
  Cache2DSum3<int16_t, false> cs;
  
  // 4x2 cache of values
  
  int width = 4;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,  0,
    0,  0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1, -1,
    0,  0,
    0,  0,
    0,  0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // invalidate (0,0) which should write to (0,0) (0,1)
  
  cs.invalidate(0,0);
  
  vector<vector<int16_t> > valuesMat = format2DVec(cs.values, height, width);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, height, width);
  
  XCTAssert(valuesMat == expectedMat);
}

// Setup a 4x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow5 {
  Cache2DSum3<int16_t, true> cs;
  
  // 4x2 cache of values
  
  int width = 4;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,  0,
    0,  0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1,  -1,  -1,  -1,
    0,   0,   0,   0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (1,0) which should write to (1,0) (2,0) (3,0)
  
  cs.invalidate(1,0);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}

// Setup a 4x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateVInvalidateRow5 {
  Cache2DSum3<int16_t, false> cs;
  
  // 4x4 cache of values
  
  int width = 4;
  int height = 4;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,  0,
    0,  0,  0,  0,
    0,  0,  0,  0,
    0,  0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    -1,  -1,  -1,  -1,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (0,1) which should write to (0,1) (0,2) (0,3) = offsets (1 2 3)
  
  cs.invalidate(0,1);
  
  vector<vector<int16_t> > valuesMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(valuesMat == expectedMat);
}

// Setup a 4x2 matrix with cached zero values and then invalidate to make
// sure that the cache invalidation is writing to the proper slots.

- (void) testInvalidateHInvalidateRow6 {
  Cache2DSum3<int16_t, true> cs;
  
  // 4x2 cache of values
  
  int width = 4;
  int height = 2;
  
  cs.allocValues(width,height,0);
  
  // Input state
  
  int16_t input[] = {
    0,  0,  0,  0,
    0,  0,  0,  0
  };
  
  // Expected output
  
  int16_t expected[] = {
    0,  0,  0, 0,
    0,  -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    cs.values[i] = input[i];
  }
  
  // Invalide (2,1) which should write to (2,1) (3,1)
  
  cs.invalidate(2,1);
  
  vector<vector<int16_t> > iterOrderMat = format2DVec(cs.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
}


// Test case where a 4x4 matrix contains a cached value
// and then a neighbor causes the cache to be calculated.

- (void) testCalcRow3Sum1 {
  // 4x4 cache of values
  
  int width = 4;
  int height = 4;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    0,  1,  2,  3,
    4,  5,  6,  7,
    8,  9,  10, 11,
    12, 13, 14, 15
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    0,  0,  1,  2,
   -1, -1, -1, -1,
   -1, -1, -1, -1,
   -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // get the cached value at (0,0) which causes a cache miss

  // ave(0) = 0
  
  int16_t c00 = l2Cache.getCachedValue(l1Cache, 0, 0);
  
  XCTAssert(c00 == 0);

  // ave(0,1) = 0
  
  int16_t c10 = l2Cache.getCachedValue(l1Cache, 1, 0);
  
  XCTAssert(c10 == 0);
  
  // ave(0,1,2) = 1
  
  int16_t c20 = l2Cache.getCachedValue(l1Cache, 2, 0);
  
  XCTAssert(c20 == 1);

  // ave(1,2,3) = 2
  
  int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
  
  XCTAssert(c30 == 2);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// Read the same cached values 2 times and make sure the
// results are the same.

- (void) testCalcRow3Sum2 {
  // 4x4 cache of values
  
  int width = 4;
  int height = 4;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    0,  1,  2,  3,
    4,  5,  6,  7,
    8,  9,  10, 11,
    12, 13, 14, 15
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    0,  0,  1,  2,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Each read causes a cache miss
  
  {
    // ave(0) = 0
    
    int16_t c00 = l2Cache.getCachedValue(l1Cache, 0, 0);
    
    XCTAssert(c00 == 0);
    
    // ave(0,1) = 0
    
    int16_t c10 = l2Cache.getCachedValue(l1Cache, 1, 0);
    
    XCTAssert(c10 == 0);
    
    // ave(0,1,2) = 1
    
    int16_t c20 = l2Cache.getCachedValue(l1Cache, 2, 0);
    
    XCTAssert(c20 == 1);
    
    // ave(1,2,3) = 2
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 2);
  }

  // Re-read should be a cache hit each time
  
  {
    // ave(0) = 0
    
    int16_t c00 = l2Cache.getCachedValue(l1Cache, 0, 0);
    
    XCTAssert(c00 == 0);
    
    // ave(0,1) = 0
    
    int16_t c10 = l2Cache.getCachedValue(l1Cache, 1, 0);
    
    XCTAssert(c10 == 0);
    
    // ave(0,1,2) = 1
    
    int16_t c20 = l2Cache.getCachedValue(l1Cache, 2, 0);
    
    XCTAssert(c20 == 1);
    
    // ave(1,2,3) = 2
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 2);
  }
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// Read a cached value, then invalidate that slot, then read again

- (void) testCalcRow3WInvalidate1 {
  // 4x4 cache of values
  
  int width = 4;
  int height = 4;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    0,  1,  2,  3,
    4,  5,  6,  7,
    8,  9,  10, 11,
    12, 13, 14, 15
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    -1, -1, -1,  2,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Each read causes a cache miss
  
  {
    // ave(1,2,3) = 2
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 2);
  }
  
  // Cache slot should not invalid
  
  XCTAssert(l2Cache.values[3] != -1);
  
  l2Cache.invalidate(3, 0);
  
  // Cache slot should not invalid
  
  XCTAssert(l2Cache.values[3] == -1);
  
  // Re-read should be a cache hit each time
  
  {
    // ave(1,2,3) = 2
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 2);
    
    // Cache slot should not invalid
    
    XCTAssert(l2Cache.values[3] != -1);
  }
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// Set cache values of all zero and then invalidate

- (void) testCalcRow3WInvalidate2 {
  // 4x4 cache of values
  
  int width = 4;
  int height = 4;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    0,  1,  2,  3,
    4,  5,  6,  7,
    8,  9,  10, 11,
    12, 13, 14, 15
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
   -1, -1, -1, -1,
    1,  1,  1,  1,
    1,  1,  1,  1,
   -1, -1, -1,  1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  // Each cache slot set to 1
  
  l2Cache.allocValues(width,height,1);

  // Invalidate (1,0)
  
  l2Cache.invalidate(1, 0);

  // Invalidate (0,3)
  
  l2Cache.invalidate(0, 3);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// In the case where the delta to the right becomes available *after* the pixel
// itself has been processed, need to invalidate the slot immediately to the left also!

// 1 10 20 X X

// Test case where the delta changes when a pixel is processed and then
// the cache value must be recomputed to account for the change.

- (void) testCalcRow3Recalc1 {
  int width = 5;
  int height = 5;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    1,  10,  20,  30, 40,
    2,  20,  30,  40, 50,
    3,  30,  40,  50, 60,
    4,  40,  50,  60, 70
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    -1, -1, -1, 20, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Read w cache miss from (3,0)
  
  int cacheOffset = 3;
  
  // Should be invalid
  XCTAssert(l2Cache.values[cacheOffset] == -1);
  
  {
    // ave(10,20,30) = 20
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 20);
  }
  
  // Cache slot should not invalid
  
  XCTAssert(l2Cache.values[cacheOffset] != -1);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

- (void) testCalcRow3Recalc2 {
  int width = 5;
  int height = 5;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    1,  10,  20,  30, 40,
    2,  20,  30,  40, 50,
    3,  30,  40,  50, 60,
    4,  40,  50,  60, 70
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Read w cache miss from (3,0)
  
  int cacheOffset = 3;
  
  // Should be invalid
  XCTAssert(l2Cache.values[cacheOffset] == -1);
  
  {
    // ave(10,20,30) = 20
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 20);
  }
  
  // Cache slot should not invalid
  
  XCTAssert(l2Cache.values[cacheOffset] != -1);
  
  // Invalidate (3, 0)
  
  l2Cache.invalidate(3, 0);
  
  XCTAssert(l2Cache.values[cacheOffset] == -1);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// Invalidate slot to the right of the cache

- (void) testCalcRow3Recalc3 {
  int width = 5;
  int height = 5;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    1,  10,  20,  30, 40,
    2,  20,  30,  40, 50,
    3,  30,  40,  50, 60,
    4,  40,  50,  60, 70
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Read w cache miss from (3,0)
  
  int cacheOffset = 3;
  
  // Should be invalid
  XCTAssert(l2Cache.values[cacheOffset] == -1);
  
  {
    // ave(10,20,30) = 20
    
    int16_t c30 = l2Cache.getCachedValue(l1Cache, 3, 0);
    
    XCTAssert(c30 == 20);
  }
  
  // Cache slot should not invalid
  
  XCTAssert(l2Cache.values[cacheOffset] != -1);
  
  // Invalidate (4, 0)
  
  l2Cache.invalidate(4, 0);
  
  // Slot at (3, 0) should have been invalidated
  
  XCTAssert(l2Cache.values[cacheOffset] == -1);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

// Invalidate 2 cache slots to the right

- (void) testCalcRow3Recalc4 {
  int width = 5;
  int height = 5;
  
  Cache2D<int16_t, true> l1Cache;
  
  l1Cache.allocValues(width,height,0);
  
  int16_t l1DeltaInput[] = {
    1,  10,  20,  30, 40,
    2,  20,  30,  40, 50,
    3,  30,  40,  50, 60,
    4,  40,  50,  60, 70
  };
  
  // Expected delta ave
  
  int16_t expected[] = {
    -1, -1, -1, 20, 30,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1
  };
  
  for ( unsigned int i = 0; i < (width*height); i++ ) {
    l1Cache.values[i] = l1DeltaInput[i];
  }
  
  Cache2DSum3<int16_t, true> l2Cache;
  
  l2Cache.allocValues(width,height,-1);
  
  // Generate cache miss for each col in row 0

  for ( unsigned int i = 0; i < width; i++ ) {
    l2Cache.getCachedValue(l1Cache, i, 0);
  }
  
  // Invalidate (0,0) which invalidates (0,1) (0,2)
  
  l2Cache.invalidate(0, 0);
  
  vector<vector<int16_t> > cacheMat = format2DVec(l2Cache.values, width, height);
  
  vector<vector<int16_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(cacheMat == expectedMat);
}

@end

