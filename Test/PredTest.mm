//
//  PredTest.mm
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Tests for prediction logic based on 3 component RGB elements in a matrix.

#import <XCTest/XCTest.h>

#import "ColortableIter.hpp"

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

// Old API that would use pixel by pixel prediction but a colortable was
// passed in and pixel were looked up in the colortable.

static inline
void CTI_IteratePixelsInTable(
                              const uint32_t * const colortablePixelsPtr,
                              const int colortableNumPixels,
                              const uint8_t * const tableOffsetsPtr,
                              const bool tablePred,
                              const int regionWidth,
                              const int regionHeight,
                              vector<uint32_t> & iterOrder)
{
  assert(tablePred == false);
  
  int N = regionWidth * regionHeight;
  
  uint32_t *pixelPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    uint8_t offset = tableOffsetsPtr[i];
    uint32_t pixel = colortablePixelsPtr[offset];
    pixelPtr[i] = pixel;
  }
  
  CTI_Iterate(pixelPtr,
              nullptr,
              -1,
              nullptr,
              tablePred,
              regionWidth, regionHeight,
              iterOrder,
              nullptr);
  
  delete [] pixelPtr;
}



// Reorder block rows so that rows are sorted by initial column value.

@interface PredTest : XCTestCase

@end

@implementation PredTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

// A 4x4 matrix of RGB values

- (void) test4x4CachedPredictionValues1 {
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000,
    0x000A0A0A,
    0x00FFFFFF,
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 1, 1,
    0, 0, 1, 1,
    2, 1, 1, 1,
    2, 1, 1, 1
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint32_t expected[] = {
    0,  1,   4,  5,
    2,  3,   8,  12,
    6,  7,  11, 15,
    10, 14,  9, 13
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  {
    const bool tablePred = false;
    
    int width = dim;
    int height = dim;
    
    int N = width * height;
    
    uint32_t *pixelPtr = new uint32_t[N];
    
    for ( int i = 0; i < N; i++ ) {
      uint8_t colortableOffset = tableOffsets[i];
      uint32_t pixel = colortablePixelsPtr[colortableOffset];
      pixelPtr[i] = pixel;
    }

//    CTI_Iterate(pixelPtr,
//                nullptr,
//                -1,
//                nullptr,
//                tablePred,
//                dim, dim,
//                iterOrder);
    
    CTI_Struct ctiStruct;
    
    // Call setup to process the upper left pixels
    
    CTI_Setup(ctiStruct,
              pixelPtr,
              nullptr,
              -1,
              nullptr,
              tablePred,
              width, height,
              iterOrder,
              nullptr);

    // The init logic will iterate over the first 4 pixels.
    
    // X X C -
    // X X - -
    // - - - -
    // - - - -
    
    auto & cachedHDeltaSums = ctiStruct.cachedHDeltaSums;
    auto & cachedVDeltaSums = ctiStruct.cachedVDeltaSums;
    
    {
      int16_t c0H = cachedHDeltaSums.values[0];
      int16_t c0V = cachedVDeltaSums.values[0];

      int16_t c1H = cachedHDeltaSums.values[CTIOffset2d(1, 0, ctiStruct.width)];
      int16_t c1V = cachedVDeltaSums.values[CTIOffset2d(0, 1, ctiStruct.height)];
      
      int16_t c2H = cachedHDeltaSums.values[CTIOffset2d(0, 1, ctiStruct.width)];
      int16_t c2V = cachedVDeltaSums.values[CTIOffset2d(1, 0, ctiStruct.height)];

      int16_t c3H = cachedHDeltaSums.values[CTIOffset2d(1, 1, ctiStruct.width)];
      int16_t c3V = cachedVDeltaSums.values[CTIOffset2d(1, 1, ctiStruct.height)];

      // (0,0) has H (right) cache and V (down) cache set
      
      XCTAssert(c0H == 0);
      XCTAssert(c0V == 0);
      
      // (1,0) has V set but H is not set since right is not processed
      
      XCTAssert(c1H == -1);
      XCTAssert(c1V == 0);
      
      // (0,1) has no pixel to the left and no pixel below

      XCTAssert(c2H == 0);
      XCTAssert(c2V == -1);
      
      // (1, 1) has no right or down cache
      
      XCTAssert(c3H == -1);
      XCTAssert(c3V == -1);
    }
    
    // The CTI_IterateStep call then selects the horizontal
    // delta that chooses (2, 0) indicated by C above.
    
    {
      bool hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                           tablePred,
                                           iterOrder,
                                           nullptr);
      XCTAssert(hasMoreDeltas);
    }
    
    {
      // Cache at (1, 0) has been updated with a H (right) cache
      
      int16_t c1H = cachedHDeltaSums.values[CTIOffset2d(1, 0, ctiStruct.width)];
      int16_t c1V = cachedVDeltaSums.values[CTIOffset2d(0, 1, ctiStruct.height)];
      
      // (2,0) has no vertical cached value since it is in row 0
      
      XCTAssert(c1V == 0);
      
      // (2, 0) calculates an average that includes two zeros
      // from the 2 slots to the left.
      
      XCTAssert(c1H == 30);
    }

    {
      // (2,0) has 2 invalid cache values
      
      int16_t c2H = cachedHDeltaSums.values[CTIOffset2d(2, 0, ctiStruct.width)];
      int16_t c2V = cachedVDeltaSums.values[CTIOffset2d(0, 2, ctiStruct.height)];
      
      XCTAssert(c2V == -1);
      XCTAssert(c2H == -1);
    }
    
    // The delta for all 4 upper left pixels is 0 since
    // they are all the same pixel.
    
    delete [] pixelPtr;
  }
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 5);
  
  XCTAssert(iterOrder.back() == 2);
  
  /*
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  */
   
  return;
}

// Predict pixel (R,G,B) based on ave of neighbor pixels

- (void) testNeighborPred1 {
  
  uint32_t colortablePixelsPtr[] = {
    0x000A0B0C,
    0x00FFFFFF
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 1, 1,
    0, 0, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  {
    const bool tablePred = false;
    
    int width = dim;
    int height = dim;
    
    int N = width * height;
    
    uint32_t *pixelsPtr = new uint32_t[N];
    
    for ( int i = 0; i < N; i++ ) {
      uint8_t colortableOffset = tableOffsets[i];
      uint32_t pixel = colortablePixelsPtr[colortableOffset];
      //printf("i = %d : table offset %d : pixel 0x%08X\n", i, colortableOffset, pixel);
      pixelsPtr[i] = pixel;
    }
    
    assert(pixelsPtr[0] == colortablePixelsPtr[0]);
    assert(pixelsPtr[1] == colortablePixelsPtr[0]);
    assert(pixelsPtr[2] == colortablePixelsPtr[1]);
    
    CTI_Struct ctiStruct;
    
    // Call setup to process the upper left pixels
    
    CTI_Setup(ctiStruct,
              pixelsPtr,
              nullptr,
              -1,
              nullptr,
              tablePred,
              width, height,
              iterOrder,
              nullptr);
    
    // Predict first pixel based only on left neighbor
    
    // X X C -
    // X X - -
    // - - - -
    // - - - -
    
    uint32_t predPixel = CTI_NeighborPredict(ctiStruct,
                                             tablePred,
                                             2, 0);

    uint32_t actualPixel = pixelsPtr[1];
    
    if ((0)) {
    printf("predPixel   0x%08X\n", predPixel);
    printf("actualPixel 0x%08X\n", actualPixel);
    }
    
    XCTAssert(predPixel == actualPixel);

    XCTAssert(predPixel == 0x000A0B0C);
    
    delete [] pixelsPtr;
  }
  
  return;
}

- (void) testNeighborPred2 {
  
  uint32_t colortablePixelsPtr[] = {
    0x000A0B0C,
    0x00FFFFFF
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 1, 1,
    0, 0, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  {
    const bool tablePred = false;
    
    int width = dim;
    int height = dim;
    
    int N = width * height;
    
    uint32_t *pixelsPtr = new uint32_t[N];
    
    for ( int i = 0; i < N; i++ ) {
      uint8_t colortableOffset = tableOffsets[i];
      uint32_t pixel = colortablePixelsPtr[colortableOffset];
      //printf("i = %d : table offset %d : pixel 0x%08X\n", i, colortableOffset, pixel);
      pixelsPtr[i] = pixel;
    }
    
    assert(pixelsPtr[0] == colortablePixelsPtr[0]);
    assert(pixelsPtr[1] == colortablePixelsPtr[0]);
    assert(pixelsPtr[2] == colortablePixelsPtr[1]);
    
    CTI_Struct ctiStruct;
    
    // Call setup to process the upper left pixels
    
    CTI_Setup(ctiStruct,
              pixelsPtr,
              nullptr,
              -1,
              nullptr,
              tablePred,
              width, height,
              iterOrder,
              nullptr);
    
    // Predict a pixel based on two horizontal neighbors
    
    // X X - -
    // X X C X
    // - - - -
    // - - - -
    
    ctiStruct.processedFlags[7] = 1;
    
    uint32_t predPixel = CTI_NeighborPredict(ctiStruct,
                                             tablePred,
                                             2, 1);
    
    uint32_t actualPixel = pixelsPtr[6];
    
    if ((0)) {
      printf("predPixel   0x%08X\n", predPixel);
      printf("actualPixel 0x%08X\n", actualPixel);
    }

    // ave(0x000A0B0C, 0x00FFFFFF)
    
    XCTAssert(predPixel == 0x00848585);
    
    delete [] pixelsPtr;
  }
  
  return;
}

- (void) testNeighborPred3 {
  
  uint32_t colortablePixelsPtr[] = {
    0x000A0B0C,
    0x00FFFFFF
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 1, 1,
    0, 0, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  {
    const bool tablePred = false;
    
    int width = dim;
    int height = dim;
    
    int N = width * height;
    
    uint32_t *pixelsPtr = new uint32_t[N];
    
    for ( int i = 0; i < N; i++ ) {
      uint8_t colortableOffset = tableOffsets[i];
      uint32_t pixel = colortablePixelsPtr[colortableOffset];
      //printf("i = %d : table offset %d : pixel 0x%08X\n", i, colortableOffset, pixel);
      pixelsPtr[i] = pixel;
    }
    
    assert(pixelsPtr[0] == colortablePixelsPtr[0]);
    assert(pixelsPtr[1] == colortablePixelsPtr[0]);
    assert(pixelsPtr[2] == colortablePixelsPtr[1]);
    
    CTI_Struct ctiStruct;
    
    // Call setup to process the upper left pixels
    
    CTI_Setup(ctiStruct,
              pixelsPtr,
              nullptr,
              -1,
              nullptr,
              tablePred,
              width, height,
              iterOrder,
              nullptr);
    
    // Predict a pixel based on two horizontal neighbors
    
    // X X X -
    // X X C -
    // - - - -
    // - - - -
    
    ctiStruct.processedFlags[2] = 1;
    
    uint32_t predPixel = CTI_NeighborPredict(ctiStruct,
                                             tablePred,
                                             2, 1);
    
    uint32_t actualPixel = pixelsPtr[6];
    
    if ((0)) {
      printf("predPixel   0x%08X\n", predPixel);
      printf("actualPixel 0x%08X\n", actualPixel);
    }
    
    // ave(0x000A0B0C, 0x00FFFFFF)
    
    XCTAssert(predPixel == 0x00848585);
    
    delete [] pixelsPtr;
  }
  
  return;
}

// Fast divide by 3 in range of 2^10 = 1024

- (void) testDiv3 {
  for ( unsigned int i = 0; i < 1024; i++ ) {
    unsigned int d3 = fast_div_3(i);
    //assert(d3 == (i / 3));
    XCTAssert(d3 == (i / 3), @"i = %d", i);
  }
}

- (void) testDiv2 {
  for ( unsigned int i = 0; i < 1024; i++ ) {
    unsigned int d2 = fast_ave_2(i, i+2);
    unsigned int i2 = (i + (i+2)) / 2;
    XCTAssert((d2 == i2), @"i = %d", i);
  }
}

// Unsigned to signed conversion

- (void) testSignedByteRange {
  // (0, 127) -> (0, 127)
  
  for ( unsigned int i = 0; i < 127; i++ ) {
    unsigned int bVal = i;
    int sVal = unsigned_byte_to_signed(bVal);
    XCTAssert(sVal == bVal, @"i = %d", i);
  }
  
  // (128, 255) -> (-128, 127)
  
  int ni = 1;
  
  for ( unsigned int i = 255; i >= 128; i--, ni++ ) {
    unsigned int bVal = i;
    int sVal = unsigned_byte_to_signed(bVal);
    XCTAssert(sVal == (ni * -1), @"i = %d", i);
  }
}

// Signed to unsigned conversion

- (void) testUnsignedByteRange {
  // (0, 127) -> (0, 127)
  
  for ( unsigned int i = 0; i < 127; i++ ) {
    unsigned int bVal = i;
    int sVal = signed_byte_to_unsigned(bVal);
    XCTAssert(sVal == bVal, @"i = %d", i);
  }
  
  // (128, 255) -> (-128, 127)
  
  int ni = 1;
  
  for ( unsigned int i = 255; i >= 128; i--, ni++ ) {
    int iVal = (ni * -1);
    int bVal = signed_byte_to_unsigned(iVal);
    XCTAssert(bVal == i, @"i = %d", i);
  }
}

- (void) testAve012_t1 {
  {
    int a = average_012(-1, -1);
    XCTAssert(a == -1);
  }
  
  {
    int a = average_012(-1, 0);
    XCTAssert(a == 0);
  }
  
  {
    int a = average_012(0, -1);
    XCTAssert(a == 0);
  }
  
  {
    int a = average_012(0, 1);
    XCTAssert(a == 0);
  }

  {
    int a = average_012(2, 4);
    XCTAssert(a == 3);
  }

  {
    int a = average_012(5, 8);
    XCTAssert(a == 6);
  }
}

- (void) testAve123_t1 {
  {
    int a = average_123(0, -1, -1);
    XCTAssert(a == 0);
  }

  {
    int a = average_123(10, -1, -1);
    XCTAssert(a == 10);
  }

  {
    int a = average_123(5, 5, 5);
    XCTAssert(a == 5);
  }
  
  {
    int a = average_123(2, 5, 8);
    XCTAssert(a == 5);
  }
  
  {
    int a = average_123(2, 5, 10);
    XCTAssert(a == 5);
  }

}

/*
 
 When a gradclamp is to be done, but there is also an additional
 pixel that near the center pixel, should the gradclamp return the
 one above or should it skew the results nearer to the other known
 pixel via an average? Currently, the UP pixel is used directly
 but that may not be totally accurate when dealing with a big delta.
 Also, how does the H vs V pred direction play into this? For example,
 if the area nearby is kown to be increasing up and to the right, the
 the prediction could take advantage of that. (the block above)
 
 NeighborBits:
 1 1 0
 1 x 0
 1 0 0
 0x00000000 0x004091E8 0xFFFFFFFF
 0x00000000 0xFFFFFFFF 0xFFFFFFFF
 0x00000000 0xFFFFFFFF 0xFFFFFFFF
 
 gradclamp:
 0x00000000 0x004091E8:
 0x00000000 X:
 predPixel 0x004091E8 X:
 pred   0x004091E8
 actual 0x00000000
 delta  0x00C06F18
 
 */

/*
 
 In this case a diag could be used as opposed to a gradclamp.
 
 CTI_NeighborPredict2(10,11)
 C  (10,11) 186
 NeighborBits:
 0 1 1
 0 x 1
 1 0 1
 0xFFFFFFFF 0x003AA3ED 0x003AA3ED
 0xFFFFFFFF 0xFFFFFFFF 0x003AA3ED
 0x003AA3ED 0xFFFFFFFF 0x003AA3ED
 
 */

/*
 
 In this case, a V delta could be mixed with a diag to
 create an ave that is closer to the in between of the
 known pixels. The diag from top left to bottom right
 is currently unused. This would only be useful when
 the deltas of the diag are known to be less than the
 deltas from the V.
 
 NeighborBits:
 1 1 1
 1 x 0
 1 1 1
 0x003AA3ED 0x0030ADED 0x0030ADED
 0x003AA3ED 0xFFFFFFFF 0xFFFFFFFF
 0x003AA3ED 0x003AA3ED 0x003AA3ED
 
 U -> D : 0x0030ADED -> 0x003AA3ED
 V ave (R G B) (53 168 237)
 pred   0x0035A8ED
 actual 0x003AA3ED
 delta  0x0005FB00
 
 */

/*
 
 In this case the diag is a better indicator
 of the actual value since the top left to
 bottom right one is zero and there is a non-zero
 delta for the V.
 
 C  (14,4) 78
 NeighborBits:
 1 1 1
 1 x 0
 1 1 1
 0x0030ADED 0x0030ADED 0x0030ADED
 0x003AA3ED 0xFFFFFFFF 0xFFFFFFFF
 0x003AA3ED 0x003AA3ED 0x0030ADED
 
 U -> D : 0x0030ADED -> 0x003AA3ED
 V ave (R G B) (53 168 237)
 pred   0x0035A8ED
 actual 0x0030ADED
 delta  0x00FB0500
 */

// Neighbor prediction on a 3x3 grid where the pixel to
// be predicted is on a hard vertical edge. Since the
// pixel to be predicted is between 2 values that
// are very alike, the average between the very alike
// axis values should be a lot better than the axis
// values that are not alike.

- (void) testNeighborPredVertical1 {
  
  // A vertical edge is indicated by a large
  // horizontal pred err in the neighbor above
  // or below the pixel to be predicted.
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000,
    0x00FFFFFF,
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 1,
    0, 0, 1,
    0, 0, 1
  };
  
  int width = 3;
  int height = 3;
  
  int N = width * height;
  
  uint32_t *pixelPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    uint8_t colortableOffset = tableOffsets[i];
    uint32_t pixel = colortablePixelsPtr[colortableOffset];
    pixelPtr[i] = pixel;
  }
  
  // Prediction error for (2,0) is 0x00FFFFFF
  
  uint32_t inPredErrors[] = {
    0x00000000, 0x00000000, 0x00FFFFFF,
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00FFFFFF
  };
  
  uint32_t *deltasPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    deltasPtr[i] = inPredErrors[i];
  }
  
  const bool tablePred = false;
  
  CTI_Struct ctiStruct;
  
  // Call setup to process the upper left pixels
  
  vector<uint32_t> iterOrder;
  
  CTI_Setup(ctiStruct,
            pixelPtr,
            nullptr,
            -1,
            nullptr,
            tablePred,
            width, height,
            iterOrder,
            nullptr);
  
  uint32_t predPixel;
  
  // Reset processed flags
  
  {
    uint8_t processed[] = {
      1, 1, 1,
      1, 1, 0,
      1, 1, 1,
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  // Prediction of (2,1) has U, D, L neighbors.
  // A simple mix of the vertical and horizontal
  // would be in the middle, but the fact that
  // the pred errors of U and D are known can
  // indicate that the weight V should be a lot higher.
  
  predPixel = CTI_NeighborPredict2(ctiStruct,
                                   tablePred,
                                   deltasPtr,
                                   2, 1);
  
  printf("0x%08X\n", predPixel);
  
  XCTAssert(predPixel == 0x00FFFFFF); // (255 255 255)
  
  return;
}

// Neighbor prediction on a 3x3 grid where the pixel to
// be predicted is on a hard vertical edge. Since the
// pixel to be predicted is between 2 values that
// are very alike, the average between the very alike
// axis values should be a lot better than the axis
// values that are not alike.

- (void) testNeighborPredVertical2 {
  
  // A vertical edge is indicated by a large
  // horizontal pred err in the neighbor above
  // or below the pixel to be predicted.
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000,
    0x00040404, // (4, 4, 4)
    0x000A0A0A, // (10, 10, 10)
  };
  
  uint8_t tableOffsets[] = {
    2, 1, 0,
    2, 1, 0,
    2, 1, 0
  };
  
  int width = 3;
  int height = 3;
  
  int N = width * height;
  
  uint32_t *pixelPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    uint8_t colortableOffset = tableOffsets[i];
    uint32_t pixel = colortablePixelsPtr[colortableOffset];
    pixelPtr[i] = pixel;
  }
  
  // Prediction error for (2,0) is 0x00FFFFFF
  
  uint32_t inPredErrors[] = {
    0x00000000, 0x00000000, 0x00FFFFFF,
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00FFFFFF
  };
  
  uint32_t *deltasPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    deltasPtr[i] = inPredErrors[i];
  }
  
  const bool tablePred = false;
  
  CTI_Struct ctiStruct;
  
  // Call setup to process the upper left pixels
  
  vector<uint32_t> iterOrder;
  
  CTI_Setup(ctiStruct,
            pixelPtr,
            nullptr,
            -1,
            nullptr,
            tablePred,
            width, height,
            iterOrder,
            nullptr);
  
  uint32_t predPixel;
  
  // Reset processed flags
  
  {
    uint8_t processed[] = {
      1, 1, 0,
      1, 0, 1,
      1, 1, 0,
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  // Prediction for (1,1) has both H and V axis,
  // check the abs() of the delta for each component
  // and then return a prediction that is the
  // ave() of the smaller of H and V deltas.
  //
  // Ave of U and D:
  // (4, 4, 4) -> X -> (4, 4, 4)
  
  predPixel = CTI_NeighborPredict2(ctiStruct,
                                   tablePred,
                                   deltasPtr,
                                   1, 1);
  
  printf("0x%08X\n", predPixel);
  
  XCTAssert(predPixel == 0x00040404); // (255 255 255)
  
  return;
}

// Neighbor prediction on a 3x3 grid where the pixel to
// be predicted is on a hard vertical edge. Since the
// pixel to be predicted is between 2 values that
// are very alike, the average between the very alike
// axis values should be a lot better than the axis
// values that are not alike.

- (void) testNeighborPredVertical3 {
  
  // A vertical edge is indicated by a large
  // horizontal pred err in the neighbor above
  // or below the pixel to be predicted.
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000,
    0x000A040A, // (10,  4, 10)
    0x00040A04, // ( 4, 10,  4)
  };
  
  uint8_t tableOffsets[] = {
    0, 1, 0,
    2, 1, 1,
    0, 2, 0
  };
  
  int width = 3;
  int height = 3;
  
  int N = width * height;
  
  uint32_t *pixelPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    uint8_t colortableOffset = tableOffsets[i];
    uint32_t pixel = colortablePixelsPtr[colortableOffset];
    pixelPtr[i] = pixel;
  }
  
  // Prediction error for (2,0) is 0x00FFFFFF
  
  uint32_t inPredErrors[] = {
    0x00000000, 0x00000000, 0x00FFFFFF,
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00FFFFFF
  };
  
  uint32_t *deltasPtr = new uint32_t[N];
  
  for ( int i = 0; i < N; i++ ) {
    deltasPtr[i] = inPredErrors[i];
  }
  
  const bool tablePred = false;
  
  CTI_Struct ctiStruct;
  
  // Call setup to process the upper left pixels
  
  vector<uint32_t> iterOrder;
  
  CTI_Setup(ctiStruct,
            pixelPtr,
            nullptr,
            -1,
            nullptr,
            tablePred,
            width, height,
            iterOrder,
            nullptr);
  
  uint32_t predPixel;
  
  // Reset processed flags
  
  {
    uint8_t processed[] = {
      1, 1, 0,
      1, 0, 1,
      1, 1, 0,
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  // Prediction for (1,1) has both H and V axis, but
  // since each delta is the same it indicates a tie
  // and the tie breaker is to choose the H delta.
  //
  // Ave of L and R:
  // (ave(4,10), ave(4,10), ave(4,10))
  // (7, 7, 7)
  
  predPixel = CTI_NeighborPredict2(ctiStruct,
                                   tablePred,
                                   deltasPtr,
                                   1, 1);
  
  printf("0x%08X\n", predPixel);
  
  XCTAssert(predPixel == 0x00070707); // (255 255 255)
  
  return;
}

@end

