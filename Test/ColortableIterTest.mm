//
//  ColortableIterTest.mm
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Test vector table methods

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

// Count the number of horizontal deltas

static inline
int ctiStruct_horizontal_count(CTI_Struct & ctiStruct) {
  return ctiStruct.countDeltasTypes(true, false);
}

// Count the number of vertical table entries

static inline
int ctiStruct_vertical_count(CTI_Struct & ctiStruct) {
  return ctiStruct.countDeltasTypes(false, true);
}

// Call to CTI_Iterate that accepts input pixels only

static inline
void CTI_IterateOverPixels(
                              const uint32_t * const pixelsPtr,
                              const int regionWidth,
                              const int regionHeight,
                              vector<uint32_t> & iterOrder)
{
  
  CTI_Iterate(pixelsPtr,
              nullptr,
              -1,
              nullptr,
              false,
              regionWidth, regionHeight,
              iterOrder,
              nullptr);
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
  
  CTI_IterateOverPixels(pixelPtr,
                        regionWidth, regionHeight,
                        iterOrder);
  
  delete [] pixelPtr;
}

// Reorder block rows so that rows are sorted by initial column value.

@interface ColortableIterTest : XCTestCase

@end

@implementation ColortableIterTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void) testWaitListAdd1 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(2);

  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }

  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,");
  }

  {
    CoordDelta cd(1, 0, 2, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }

  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,0,");
  }
  
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,0,1,");
  }
}

- (void) testWaitListAdd1Remove1 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(1);
  
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,");
  }

  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "0,");
  }
  
  int err;
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "");
  }
  
  XCTAssert(cd.toX() == 1);
  XCTAssert(cd.toY() == 0);
}

- (void) testWaitListAdd2Remove2 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(1);

  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  {
    CoordDelta cd(1, 0, 2, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,0,");
  }
  
  CoordDelta cd;
  int err;
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,");
  }
  
  XCTAssert(cd.toX() == 2);
  XCTAssert(cd.toY() == 0);
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "");
  }
  
  XCTAssert(cd.toX() == 1);
  XCTAssert(cd.toY() == 0);
}

- (void) testWaitListAdd3Remove3 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(4);
  
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  {
    CoordDelta cd(1, 0, 2, 0, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  {
    CoordDelta cd(2, 0, 3, 0, true);
    ctiStruct.addToWaitList(cd, 1);
  }

  {
    CoordDelta cd(3, 0, 4, 0, true);
    ctiStruct.addToWaitList(cd, 2);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,1,1,2,");
  }
  
  // Query just the values associated with 0

  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "0,");
  }
  
  {
    string s = ctiStruct.waitListToString(1);
    XCTAssert(s == "1,1,");
  }

  {
    string s = ctiStruct.waitListToString(2);
    XCTAssert(s == "2,");
  }
  
  CoordDelta cd;
  
  // Remove 0
  
  int err;
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "1,1,2,");
  }

  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(1);
    XCTAssert(s == "1,1,");
  }
  
  XCTAssert(err == 0);
  
  XCTAssert(cd.toX() == 1);
  XCTAssert(cd.toY() == 0);
  
  // Remove the delta=1 that was pushed to the front
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "1,2,");
  }
  
  XCTAssert(err == 1);
  
  XCTAssert(cd.toX() == 3);
  XCTAssert(cd.toY() == 0);
  
  // Remove the delta=1 that was inserted first
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "2,");
  }

  XCTAssert(err == 1);
  
  XCTAssert(cd.toX() == 2);
  XCTAssert(cd.toY() == 0);
  
  // Remove 2nd 2
  
  cd = ctiStruct.firstOnWaitList(&err);
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(1);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(2);
    XCTAssert(s == "");
  }
  
  XCTAssert(err == 2);
  
  XCTAssert(cd.toX() == 4);
  XCTAssert(cd.toY() == 0);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == true);
}

// Wait list test case where error value 3 is added at end
// and then 1 is added. The delta=1 should be inserted before
// the delta=3 value in the wait list.

- (void) testWaitListInsertNotAtEnd1 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(3+1);
  
  // insert at end
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 3);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "3,");
  }
  
  // insert a smaller delta

  {
    CoordDelta cd(1, 0, 2, 0, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "1,3,");
  }

  {
    string s = ctiStruct.waitListToString(0);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(1);
    XCTAssert(s == "1,");
  }
  
  {
    string s = ctiStruct.waitListToString(2);
    XCTAssert(s == "");
  }
  
  {
    string s = ctiStruct.waitListToString(3);
    XCTAssert(s == "3,");
  }
}

// Add a value with error 0, then 3, then add a value with
// the error 1. This should search backwards from 3.

- (void) testWaitListInsertNotAtEnd2 {
  CTI_Struct ctiStruct;
  ctiStruct.initWaitList(3+1);
  
  // insert at end
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 3);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "3,");
  }
  
  // insert at front
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }

  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,3,");
  }
  
  // insert a value smaller than 3 but larger than 0
  
  {
    CoordDelta cd(1, 0, 2, 0, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  {
    string s = ctiStruct.waitListToString(-1);
    XCTAssert(s == "0,1,3,");
  }
}

// Single step test, fill in row 2 at column 1

- (void) test3by2FillRow2Col1 {
  
  uint32_t CT[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414  // 2
  };
  
//  int colortableNumPixels = (int)sizeof(CT)/sizeof(CT[0]);
//  
//  uint8_t tableOffsets[] = {
//    0, 0,
//    1, 2,
//    0, 0,
//  };
  
//  uint32_t expectedIterOrder[] = {
//    0, 1, 2, 3,
//    4, 5
//  };
  
  uint32_t imagePixels[] = {
    CT[0], CT[0],
    CT[1], CT[2],
    CT[0], CT[0]
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  const bool tablePred = false;
  
  const int regionWidth = 2;
  const int regionHeight = 3;
  
  CTI_Setup(ctiStruct,
            imagePixels,
            nullptr,
            -1,
            nullptr,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);

  XCTAssert(iterOrder.size() == 4);
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  
  // Choose (0, 2) as pixel to be processed
  
  XCTAssert(iterOrder[4] == 4);
  
  // Choose (1, 2) as the final pixel to be processed
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas == true);
  
  // The next call to min search will return an empty delta
  
  {
  
  CoordDelta minDelta;
  
  CTI_MinimumSearch(ctiStruct,
                    tablePred,
                    &minDelta);
   
    if (minDelta.isEmpty()) {
      hasMoreDeltas = false;
    } else {
      hasMoreDeltas = true;
    }
  }

  XCTAssert(hasMoreDeltas == false);
  
  XCTAssert(iterOrder.size() == 6);
  
  XCTAssert(iterOrder[5] == 5);
  
  for ( int wasProcessed : ctiStruct.processedFlags ) {
    XCTAssert(wasProcessed);
  }
  
  return;
}

// Test
// X X 2
// X X 2
// 0 1 2
//
// Where processing (0,2) and then (1,2) will result in
// row 2 delta being initialized

- (void) test3by3FillRow2Col0And1InitRow2 {
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414  // 2
  };
  
  //  uint32_t offsets[] = {
  //    0, 1, 2,
  //    3, 4, 5,
  //    6, 7, 8
  //  };
  
  uint8_t tableOffsets[] = {
    0, 1, 2,
    0, 1, 2,
    0, 1, 2
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numColortablePixels = (int)sizeof(colortablePixelsPtr)/sizeof(colortablePixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 3;
  const int regionHeight = 3;
  
  CTI_Setup(ctiStruct,
            nullptr,
            colortablePixelsPtr,
            numColortablePixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  
  // Choose (0, 2) as pixel to be processed
  
  XCTAssert(iterOrder[4] == 6);
  
  // Choose (1, 2) as the pixel to be processed
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas == true);
  
  // At this point, a delta for row 2 should have been defined
  
  
  XCTAssert(iterOrder.size() == 6);
  
  XCTAssert(iterOrder[5] == 7);
  
  // Processing offset 7 should have initilized row 0
  // since a horizontal delta can now be calculated
  // for all 3 rows. There should be no vertical deltas.
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  
  // All 3 rows should be cached and they should all have the same delta value
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 3);
  
  // Grab the first element on the wait list, this value is not recalculated
  // and checked like calling CTI_MinimumSearch() since recalculation is not done.
  
  int err;
  
  CoordDelta delta = ctiStruct.firstOnWaitList(&err);
  XCTAssert(delta.isHorizontal() == true);
  XCTAssert(delta.toX() == 1 && delta.toY() == 2);
  XCTAssert(err == 1);
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 0,
    1, 1, 0,
    1, 1, 0
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, 3, 3);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, 3, 3);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Do not init row 2 in the case where the next horizontal delta would predict
// a pixel that was already processed:
//
// processed:
// 1 1 1
// 1 1 1
// 0 1 1
//
// Where processing (0,2) and then (1,2) would result in
// row 2 delta being initialized except that the next
// column is already processed.

- (void) test3by3FillRow2Col0And1InitRow2Nop {
  
  uint32_t colortablePixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414  // 2
  };
  
  //  uint32_t offsets[] = {
  //    0, 1, 2,
  //    3, 4, 5,
  //    6, 7, 8
  //  };
  
  uint8_t tableOffsets[] = {
    0, 1, 2,
    0, 1, 2,
    0, 1, 2
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int colortableNumPixels = (int)sizeof(colortablePixelsPtr)/sizeof(colortablePixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 3;
  const int regionHeight = 3;
  
  CTI_Setup(ctiStruct,
            nullptr,
            colortablePixelsPtr,
            colortableNumPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1,
      1, 1, 1,
      0, 1, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a vertical delta from (0, 0) -> (0, 1)
  
  {
    CoordDelta cd(0, 0, 0, 1, false);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 6);
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, 3, 3);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, 3, 3);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Do not init row 2 in the case where the next horizontal delta would predict
// a pixel that was already processed:
//
// processed:
// 1 1 1
// 1 1 1
// 1 0 1
//
// Where processing (1,2) the next slot should be seen as processed
// and so a row init should not be done for row 2.

- (void) test3by3FillRow2Col0And1InitRow2Nop2 {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414  // 2
  };
  
  //  uint32_t offsets[] = {
  //    0, 1, 2,
  //    3, 4, 5,
  //    6, 7, 8
  //  };
  
  uint8_t tableOffsets[] = {
    0, 1, 2,
    0, 1, 2,
    0, 1, 2
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 3;
  const int regionHeight = 3;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1,
      1, 1, 1,
      1, 0, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a vertical delta from (1, 0) -> (1, 1)
  
  {
    CoordDelta cd(1, 0, 1, 1, false);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 7);

  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, 3, 3);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, 3, 3);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case starts with processed flags like the following and then (0,2) is processed.
//
// 1  1  1  1
// 1  1  1  1
// 0  1  1  0
// 1  1  1  1
//
// In this case, row 2 should be initialized but the tricky part is that since
// col 2 was already processed that means that a prediction should be made
// from col 2 to col 3.

- (void) test4by4InitRow2WithSkip {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1,
      1, 1, 1, 1,
      0, 1, 1, 0,
      1, 1, 1, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }

  ctiStruct.clearWaitList();
  
  // Cache a vertical delta from (0, 0) -> (0, 1)
  
  {
    CoordDelta cd(0, 0, 0, 1, false);
    ctiStruct.addToWaitList(cd, 0);
  }

  // ache larger H delta
  
  {
    CoordDelta cd(1, 2, 2, 2, true);
    ctiStruct.addToWaitList(cd, 3);
  }
  
  // Should be 1 of each
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 1);
  
  // Process
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 8);
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
      XCTAssert(cd.fromX() == 1);
      XCTAssert(cd.fromY() == 2);
      
      XCTAssert(cd.toX() == 2);
      XCTAssert(cd.toY() == 2);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 0,
    1, 1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case starts with processed flags like the following and then (0,2) is processed.
//
// 1  1  0  1
// 1  1  1  1
// 1  1  1  1
// 1  1  0  1
//
// In this case, col 2 should be initialized but the tricky part is that since
// row 2 was already processed that means that a prediction should be made
// from row 1 to row 2.

- (void) test4by4InitCol2WithSkip {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 0, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 0, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  {
    CoordDelta cd(0, 0, 1, 0, true);
    ctiStruct.addToWaitList(cd, 0);
  }
  
  // add larger V delta
  
  {
    CoordDelta cd(2, 1, 2, 2, false);
    ctiStruct.addToWaitList(cd, 3);
  }
  
  // Should be 1 of each
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 1);
  
  // process
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 2);
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  // Col 2 should have been initialized
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);

  XCTAssert(ctiStruct.isWaitListEmpty() == false);

  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);

  {
      XCTAssert(cd.fromX() == 2);
      XCTAssert(cd.fromY() == 1);
      
      XCTAssert(cd.toX() == 2);
      XCTAssert(cd.toY() == 2);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 0, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case where invalidation of a row should not happen when a column earlier in the row
// has not been processed. In this case (3,2) is processed but this should not invalidate
// the cached row value for (0,2) -> (1,2).
//
// 1  1  1  1
// 1  1  1  1
// 1  1  0  0
// 1  1  1  1
//
// In this case, row 2 should be initialized but the tricky part is that since
// col 2 was already processed that means that a prediction should be made
// from col 2 to col 3.

- (void) test4by4DoNotInvalidateRowCache {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 0, 0,
      1, 1, 1, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a horizontal value from (0, 2) -> (1,2)
  
  {
    CoordDelta cd(0, 2, 1, 2, true);
    ctiStruct.addToWaitList(cd, 2);
  }
  
  // Cache a vertical value from (3,0) -> (3,1)
  
  {
    CoordDelta cd(3, 0, 3, 1, false);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 11);
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  
  // Existing cached value for row 2 should not have been
  // invalidated.

  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
      XCTAssert(cd.fromX() == 0);
      XCTAssert(cd.fromY() == 2);
      
      XCTAssert(cd.toX() == 1);
      XCTAssert(cd.toY() == 2);
      
      XCTAssert(err == 2);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 0, 1,
    1, 1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}


// Test case where invalidation of a col should not happen when a column earlier in the row
// has not been processed. In this case (2,3) is processed but this should not invalidate
// the cached row value for (2,0) -> (2,1).
//
// 1  1  1  1
// 1  1  1  1
// 1  1  0  1
// 1  1  0  1
//
// In this case, row 2 should be initialized but the tricky part is that since
// col 2 was already processed that means that a prediction should be made
// from col 2 to col 3.

- (void) test4by4DoNotInvalidateColCache {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 0, 1,
      1, 1, 0, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a vertical value from (2,0) -> (2,1)
  
  {
    CoordDelta cd(2, 0, 2, 1, false);
    ctiStruct.addToWaitList(cd, 2);
  }
  
  // Cache a horizontal value from (0,3) -> (1,3)
  
  {
    CoordDelta cd(0, 3, 1, 3, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 14);
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  // Existing cached value for col 2 should not have been
  // invalidated.
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
    XCTAssert(cd.fromX() == 2);
    XCTAssert(cd.fromY() == 0);
    
    XCTAssert(cd.toX() == 2);
    XCTAssert(cd.toY() == 1);
    
    XCTAssert(err == 2);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 0, 1,
    1, 1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case where a horizontal delta is processed and then the unprocessed
// element as (2,1) is processed.
//
// 1  1  1  1
// 1  1  0  1
// 1  1  0  1
// 1  1  1  1
//
// In this case, row 2 should be initialized but the tricky part is that since
// col 2 was already processed that means that a prediction should be made
// from col 2 to col 3. This should invalidate the col cache since the
// next value to the right has been processed already.

- (void) test4by4InvalidateRowCacheInitColCache {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1,
      1, 1, 0, 1,
      1, 1, 0, 1,
      1, 1, 1, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a horizontal value from (0,1) -> (1,1)
  
  {
    CoordDelta cd(0, 1, 1, 1, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 6);
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  // A new vertical cached value should have been added
  // since row 2 now has enough elements.
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
      XCTAssert(cd.fromX() == 2);
      XCTAssert(cd.fromY() == 0);
      
      XCTAssert(cd.toX() == 2);
      XCTAssert(cd.toY() == 1);
      
      XCTAssert(err == 0);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 0, 1,
    1, 1, 1, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case where an unprocessed element appears to the left of an unprocessed
// element that is discovered via a vertical delta. The element at (2,2)
// must be processed and then the col counter must be advanced.
//
// 1  1  1  1
// 1  1  1  1
// 1  0  0  1
// 1  1  0  1

- (void) test4by4CheckColToLeftOfDiscovered {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 0, 0, 1,
      1, 1, 0, 1
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a horizontal value from (2,0) -> (2,1)
  
  {
    CoordDelta cd(2, 0, 2, 1, false);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 10);
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  // A new vertical cached value should have been added
  // since row 2 now has enough elements.
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
    XCTAssert(cd.fromX() == 2);
    XCTAssert(cd.fromY() == 1);
    
    XCTAssert(cd.toX() == 2);
    XCTAssert(cd.toY() == 2);
    
    XCTAssert(err == 0);
  }
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 0, 1, 1,
    1, 1, 0, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Test case where an unprocessed element is up and to the left, the
// (2,1) element will be set but the column count would be 1 for row
// 1 and this column count cannot be used to check the processed status
// of column 2.
//
// 1  1  1  1
// 1  0  1  1
// 1  1  0  1
// 1  1  0  1

- (void) test4by4CheckColAboveAndLeftOfDiscovered {
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414, // 2
    0x00151515  // 3
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 4;
  const int regionHeight = 4;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);
  
  // 1  1  1  1
  // 1  0  1  1
  // 1  1  0  1
  // 1  1  0  1
  
#if defined(USE_BOX_DELTA)
  // Clear processed flag for 5 at the start
  ctiStruct.processedFlags[5] = 0;
  
  // Cache must be explicitly updated step by step
  ctiStruct.processedFlags[2] = 1;
  ctiStruct.updateCache(tablePred, 2, 0);
  ctiStruct.processedFlags[3] = 1;
  ctiStruct.updateCache(tablePred, 3, 0);
  
  // Clear cache for (1, 1)
  {
    int16_t & c11H = ctiStruct.cachedHDeltaSums.values[CTIOffset2d(1, 1, ctiStruct.width)];
    int16_t & c11V = ctiStruct.cachedVDeltaSums.values[CTIOffset2d(1, 1, ctiStruct.height)];
    c11H = -1;
    c11V = -1;
  }
  
  // Clear cache for (1, 0)
  {
    int16_t & c10V = ctiStruct.cachedVDeltaSums.values[CTIOffset2d(0, 1, ctiStruct.height)];
    c10V = -1;
  }
  
  // Clear cache for (0, 1)
  {
    int16_t & c01H = ctiStruct.cachedHDeltaSums.values[CTIOffset2d(0, 1, ctiStruct.width)];
    c01H = -1;
  }
  
  ctiStruct.processedFlags[6] = 1;
  ctiStruct.updateCache(tablePred, 2, 1);
  ctiStruct.processedFlags[7] = 1;
  ctiStruct.updateCache(tablePred, 3, 1);
  
  ctiStruct.processedFlags[8] = 1;
  ctiStruct.updateCache(tablePred, 0, 2);
  ctiStruct.processedFlags[9] = 1;
  ctiStruct.updateCache(tablePred, 1, 2);
  // Skip 10
  ctiStruct.processedFlags[11] = 1;
  ctiStruct.updateCache(tablePred, 3, 2);
  
  ctiStruct.processedFlags[12] = 1;
  ctiStruct.updateCache(tablePred, 0, 3);
  ctiStruct.processedFlags[13] = 1;
  ctiStruct.updateCache(tablePred, 1, 3);
  // Skip 14
  ctiStruct.processedFlags[15] = 1;
  ctiStruct.updateCache(tablePred, 3, 3);
#else
  // Mark all as processed
  
  {
    for ( auto it = begin(ctiStruct.processedFlags); it != end(ctiStruct.processedFlags); it++ ) {
      *it = 1;
    }
  }
  
  // Mark (1, 1) as not processed
  
  ctiStruct.processedFlags[5] = 0;
  
  // Mark (2, 2) as not processed
  
  ctiStruct.processedFlags[10] = 0;
  
  // Mark (2, 3) as not processed
  
  ctiStruct.processedFlags[14] = 0;
#endif // USE_BOX_DELTA
  
  ctiStruct.clearWaitList();
  
  // Cache a vertical value from (2,0) -> (2,1)
  
  {
    CoordDelta cd(2, 0, 2, 1, false);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 10);
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 0);
  
  // row 2 should have a cached value
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 1);
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1,
    1, 0, 1, 1,
    1, 1, 1, 1,
    1, 1, 0, 1
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// In this test case, the value at (2,2) would be processed and it should
// invalidate the cached row value for row 2. The tricky part is, the
// invalidation for row 2 needs to advance past any already processed
// columns and create a new cached delta value that predicts (4,2).
//
// 1  1  1  1  1
// 1  1  1  1  1
// 1  1  0  1  0

- (void) test5by3ProcessingUpdateCachedRow {
  
  uint32_t pixelsPtr[] = {
    0x00000000 // 0
  };
  
  //  uint32_t offsets[] = {
  //     0,  1,  2,  3,  4,
  //     5,  6,  7,  8,  9,
  //    10, 11, 12, 13, 14,
  //  };
  
  // Note that all value are zero, so that a recalculated cached delta
  // will always be zero.
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
  };
  
  vector<uint32_t> iterOrder;
  
  // Do 1 iter step after init logic
  
  CTI_Struct ctiStruct;
  
  int numPixels = (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  const bool tablePred = true;
  
  const int regionWidth = 5;
  const int regionHeight = 3;
  
  CTI_Setup(ctiStruct,
            nullptr,
            pixelsPtr,
            numPixels,
            tableOffsets,
            tablePred,
            regionWidth, regionHeight,
            iterOrder,
            nullptr);
  
  XCTAssert(iterOrder.size() == 4);

  // 1  1  1  1  1
  // 1  1  1  1  1
  // 1  1  0  1  0
  
  {
    uint8_t processed[] = {
      1, 1, 1, 1, 1,
      1, 1, 1, 1, 1,
      1, 1, 0, 1, 0
    };
    
    CTI_ProcessFlags(ctiStruct, processed, tablePred);
  }
  
  ctiStruct.clearWaitList();
  
  // Cache a horizontal value from (0,2) -> (1,2)
  
  {
    CoordDelta cd(0, 2, 1, 2, true);
    ctiStruct.addToWaitList(cd, 1);
  }
  
  bool hasMoreDeltas;
  
  hasMoreDeltas = CTI_IterateStep(ctiStruct,
                                  tablePred,
                                  iterOrder,
                                  nullptr);
  
  XCTAssert(hasMoreDeltas);
  
  XCTAssert(iterOrder.size() == 5);
  XCTAssert(iterOrder[4] == 12);
  
  XCTAssert(ctiStruct_vertical_count(ctiStruct) == 0);
  
  // Rows 1 and 2 should have a cached value, note that
  // a cached value of zero means that the row was reclaculated.
  
  XCTAssert(ctiStruct_horizontal_count(ctiStruct) == 1);
  
  XCTAssert(ctiStruct.isWaitListEmpty() == false);
  
  int err;
  
  CoordDelta cd = ctiStruct.firstOnWaitList(&err);
  
  {
      // (2, 2) -> (3, 2) delta 0
      
      XCTAssert(cd.fromX() == 2);
      XCTAssert(cd.fromY() == 2);
      
      XCTAssert(cd.toX() == 3);
      XCTAssert(cd.toY() == 2);
      
      XCTAssert(err == 0);
  }
  
  // 1  1  1  1  1
  // 1  1  1  1  1
  // 1  1  0  1  0
  
  uint8_t wasProcessedExpected[] = {
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 0
  };
  
  vector<vector<uint8_t> > wasProcessedExpectedMat = format2DVec(wasProcessedExpected, regionWidth, regionHeight);
  
  vector<vector<uint8_t> > wasProcessedMat = format2DVec(ctiStruct.processedFlags, regionWidth, regionHeight);
  
  XCTAssert(wasProcessedMat == wasProcessedExpectedMat);
  
  return;
}

// Iterate 2x3 image, no horizontal deltas, 2 vertical deltas.

- (void) testColortableIterate2by3 {
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0
    0x000A0A0A, // 1
    0x00141414  // 2
  };

//  uint32_t offsets[] = {
//    0, 1,
//    2, 3,
//    4, 5,
//  };
  
  uint8_t tableOffsets[] = {
    0, 0,
    1, 2,
    0, 0,
  };
  
  uint32_t expectedIterOrder[] = {
    0, 1, 2, 3,
    4, 5
  };
  
  vector<uint32_t> iterOrder;
  
  CTI_Iterate(nullptr,
              pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              true,
              2, 3,
              iterOrder,
              nullptr);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 6);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 2, 3);
  
  vector<vector<uint32_t> > expectedIterOrderMat = format2DVec(expectedIterOrder, 2, 3);
  
  XCTAssert(iterOrderMat == expectedIterOrderMat);
  
  return;
}

// Iterate a 3x2 table, this will default to selecting the first block of 4 and then
// the prediction will deal with the remaining 4 values.

- (void) testColortableIterate3by2 {
  
  // int order pixels : bird4x4.png
  
  //  pixels[  0] = 0x007A3339 : 122 51 57
  //  pixels[  1] = 0x00832F37 : 131 47 55
  //  pixels[  2] = 0x008F4449 : 143 68 73
  //  pixels[  3] = 0x00985157 : 152 81 87
  //  pixels[  4] = 0x00995259 : 153 82 89
  //  pixels[  5] = 0x009D5254 : 157 82 84
  //  pixels[  6] = 0x009E525A : 158 82 90
  //  pixels[  7] = 0x00B97678 : 185 118 120
  //  pixels[  8] = 0x00BE7F83 : 190 127 131
  //  pixels[  9] = 0x00C58C8E : 197 140 142
  //  pixels[ 10] = 0x00D4A5A8 : 212 165 168
  //  pixels[ 11] = 0x00D9A4AB : 217 164 171
  //  pixels[ 12] = 0x00D9A9B2 : 217 169 178
  //  pixels[ 13] = 0x00DB9FA4 : 219 159 164
  //  pixels[ 14] = 0x00DC9BA1 : 220 155 161
  //  pixels[ 15] = 0x00E3BAC5 : 227 186 197
  
  uint32_t pixelsPtr[] = {
    0x007A3339,
    0x00832F37,
    0x008F4449,
    0x00985157,
    0x00995259,
    0x009D5254,
    0x009E525A,
    0x00B97678,
    0x00BE7F83,
    0x00C58C8E,
    0x00D4A5A8,
    0x00D9A4AB,
    0x00D9A9B2,
    0x00DB9FA4,
    0x00DC9BA1,
    0x00E3BAC5
  };
  
  uint8_t tableOffsets[] = {
    15, 12, 11,
    10,  9,  7
  };
  
  uint32_t expectedIterOrder[] = {
    0, 1, 3, 4,
    2, 5
  };
  
  vector<uint32_t> iterOrder;
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              3, 2,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 6);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 3, 2);
  
  vector<vector<uint32_t> > expectedIterOrderMat = format2DVec(expectedIterOrder, 3, 2);
  
  XCTAssert(iterOrderMat == expectedIterOrderMat);
  
  return;
}

// Iterate a 3x3 table, this tests consumption of the original rows and columns from the
// first 4 pixels but it also tests that the 3rd row or column is extended once the
// previous 2.

- (void) testColortableIterate3by3 {
  
  // int order pixels : bird4x4.png
  
  //  pixels[  0] = 0x007A3339 : 122 51 57
  //  pixels[  1] = 0x00832F37 : 131 47 55
  //  pixels[  2] = 0x008F4449 : 143 68 73
  //  pixels[  3] = 0x00985157 : 152 81 87
  //  pixels[  4] = 0x00995259 : 153 82 89
  //  pixels[  5] = 0x009D5254 : 157 82 84
  //  pixels[  6] = 0x009E525A : 158 82 90
  //  pixels[  7] = 0x00B97678 : 185 118 120
  //  pixels[  8] = 0x00BE7F83 : 190 127 131
  //  pixels[  9] = 0x00C58C8E : 197 140 142
  //  pixels[ 10] = 0x00D4A5A8 : 212 165 168
  //  pixels[ 11] = 0x00D9A4AB : 217 164 171
  //  pixels[ 12] = 0x00D9A9B2 : 217 169 178
  //  pixels[ 13] = 0x00DB9FA4 : 219 159 164
  //  pixels[ 14] = 0x00DC9BA1 : 220 155 161
  //  pixels[ 15] = 0x00E3BAC5 : 227 186 197
  
  uint32_t pixelsPtr[] = {
    0x007A3339,
    0x00832F37,
    0x008F4449,
    0x00985157,
    0x00995259,
    0x009D5254,
    0x009E525A,
    0x00B97678,
    0x00BE7F83,
    0x00C58C8E,
    0x00D4A5A8,
    0x00D9A4AB,
    0x00D9A9B2,
    0x00DB9FA4,
    0x00DC9BA1,
    0x00E3BAC5
  };
  
  uint8_t tableOffsets[] = {
    15, 12, 11,
    10,  9,  7,
     8,  9, 10
  };
  
  uint32_t expectedIterOrder[] = {
    0, 1, 3, 4,
    2, 5, 6, 7, 8
  };
  
  vector<uint32_t> iterOrder;
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              3, 3,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 9);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 3, 3);
  
  vector<vector<uint32_t> > expectedIterOrderMat = format2DVec(expectedIterOrder, 3, 3);
  
  XCTAssert(iterOrderMat == expectedIterOrderMat);
  
  return;
}

// Iterate a 3x3 table that has a hard vertical edge on the right side.

- (void) testColortableIterate3by3EdgeR {
  uint32_t pixelsPtr[] = {
    0x00000000,
    0x00FFFFFF,
  };

//  uint8_t order[] = {
//    0, 1, 2,
//    3, 4, 5,
//    6, 7, 8
//  };
  
  uint8_t tableOffsets[] = {
    0,  0, 1,
    0,  0, 1,
    0,  0, 1
  };
  
  uint32_t expectedIterOrder[] = {
    0, 1, 3, 4,
    2, 6, 5, 8, 7
  };
  
  vector<uint32_t> iterOrder;
  
  CTI_IteratePixelsInTable(pixelsPtr,
                           (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
                           tableOffsets,
                           false,
                           3, 3,
                           iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 9);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 3, 3);
  
  vector<vector<uint32_t> > expectedIterOrderMat = format2DVec(expectedIterOrder, 3, 3);
  
  XCTAssert(iterOrderMat == expectedIterOrderMat);
  
  return;
}

// A 4x4 matrix where all the entries are the zero value

- (void) testColortableClosestIteration4x4Zeros {
  
  uint32_t pixelsPtr[] = {
    0x00000000,
    0x00FFFFFF,
  };
  
  uint8_t tableOffsets[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
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
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              dim, dim,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// An approach where a 4x4 matrix of offsets into a table and a colortable
// are used to predict values and then choose the next pixel to advance to
// based on choosing the pixel that is closest to a valid pixel in the
// colortable.

- (void) testColortableClosestIteration {
  
  // int order pixels : bird4x4.png
  
//  pixels[  0] = 0x007A3339 : 122 51 57
//  pixels[  1] = 0x00832F37 : 131 47 55
//  pixels[  2] = 0x008F4449 : 143 68 73
//  pixels[  3] = 0x00985157 : 152 81 87
//  pixels[  4] = 0x00995259 : 153 82 89
//  pixels[  5] = 0x009D5254 : 157 82 84
//  pixels[  6] = 0x009E525A : 158 82 90
//  pixels[  7] = 0x00B97678 : 185 118 120
//  pixels[  8] = 0x00BE7F83 : 190 127 131
//  pixels[  9] = 0x00C58C8E : 197 140 142
//  pixels[ 10] = 0x00D4A5A8 : 212 165 168
//  pixels[ 11] = 0x00D9A4AB : 217 164 171
//  pixels[ 12] = 0x00D9A9B2 : 217 169 178
//  pixels[ 13] = 0x00DB9FA4 : 219 159 164
//  pixels[ 14] = 0x00DC9BA1 : 220 155 161
//  pixels[ 15] = 0x00E3BAC5 : 227 186 197
  
  uint32_t pixelsPtr[] = {
    0x007A3339,
    0x00832F37,
    0x008F4449,
    0x00985157,
    0x00995259,
    0x009D5254,
    0x009E525A,
    0x00B97678,
    0x00BE7F83,
    0x00C58C8E,
    0x00D4A5A8,
    0x00D9A4AB,
    0x00D9A9B2,
    0x00DB9FA4,
    0x00DC9BA1,
    0x00E3BAC5
  };
  
  uint8_t tableOffsets[] = {
    15, 12, 11, 13,
    10,  9,  7,  5,
     8,  3,  0,  4,
     2,  1,  6, 14
  };

//  uint32_t order[] = {
//     0,  1,  2,  3,
//     4,  5,  6,  7,
//     8,  9, 10, 11,
//    12, 13, 14, 15
//  };
  
  uint32_t expected[] = {
    0,  1,   4,  5,
    2,  3,   6,  7,
    8, 12,   9, 10,
   11, 15,  13, 14
  };

  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              dim, dim,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}



// An approach where a 4x4 matrix of offsets into a table and a colortable
// are used to predict values and then choose the next pixel to advance to
// based on choosing the pixel that is closest to a valid pixel in the
// colortable.

- (void) testColortableBox4By4 {
  
  // int order pixels : box4x4.png
  
//  pixels[  0] = 0x00000000 : 0 0 0
//  pixels[  1] = 0x00BE7F83 : 190 127 131
//  pixels[  2] = 0x00D4A5A8 : 212 165 168
//  pixels[  3] = 0x00D9A4AB : 217 164 171
//  pixels[  4] = 0x00D9A9B2 : 217 169 178
//  pixels[  5] = 0x00DB9FA4 : 219 159 164
//  pixels[  6] = 0x00DBA8B0 : 219 168 176
//  pixels[  7] = 0x00DEAEB6 : 222 174 182
//  pixels[  8] = 0x00DEAFB7 : 222 175 183
//  pixels[  9] = 0x00E0B4BE : 224 180 190
//  pixels[ 10] = 0x00E3BAC5 : 227 186 197
  
  uint32_t pixelsPtr[] = {
    0x00000000,
    0x00BE7F83,
    0x00D4A5A8,
    0x00D9A4AB,
    0x00D9A9B2,
    0x00DB9FA4,
    0x00DBA8B0,
    0x00DEAEB6,
    0x00DEAFB7,
    0x00E0B4BE,
    0x00E3BAC5
  };
  
  uint8_t tableOffsets[] = {
    10, 4, 3, 5,
     2, 0, 0, 3,
     1, 0, 0, 8,
     3, 6, 7, 9
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint32_t expected[] = {
    0,  1,  4,   5,
    2,  3,  8,  12,
    9,  13, 14, 15,
    6,   7, 11, 10
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  // FIXME: might special case the "stuck" state
  // to go down from upper right or right from
  // lower left ? Might also be useful to cover
  // the remaining middle 3 and then predict
  // outward for the remaining pixels. In the case
  // of an edge.
  //
  // 1 1 1 1
  // 1 1 0 0
  // 1 0 0 0
  // 1 0 0 0
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              dim, dim,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// The image box4x4.png where the colortable is sorted into B -> W
// order and then color table offsets are used to select the order.

- (void) testColortableBox4By4BW {
  
  // BW sorted order pixels : box4x4.png
  
//  pixels[  0] = 0x00000000 : 0 0 0
//  pixels[  1] = 0x00BE7F83 : 190 127 131
//  pixels[  2] = 0x00DB9FA4 : 219 159 164
//  pixels[  3] = 0x00D9A4AB : 217 164 171
//  pixels[  4] = 0x00D4A5A8 : 212 165 168
//  pixels[  5] = 0x00DBA8B0 : 219 168 176
//  pixels[  6] = 0x00D9A9B2 : 217 169 178
//  pixels[  7] = 0x00DEAEB6 : 222 174 182
//  pixels[  8] = 0x00DEAFB7 : 222 175 183
//  pixels[  9] = 0x00E0B4BE : 224 180 190
//  pixels[ 10] = 0x00E3BAC5 : 227 186 197
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0 0 0
    0x00BE7F83, // 190 127 131
    0x00DB9FA4, // 219 159 164
    0x00D9A4AB, // 217 164 171
    0x00D4A5A8, // 212 165 168
    0x00DBA8B0, // 219 168 176
    0x00D9A9B2, // 217 169 178
    0x00DEAEB6, // 222 174 182
    0x00DEAFB7, // 222 175 183
    0x00E0B4BE, // 224 180 190
    0x00E3BAC5  // 227 186 197
  };
  
  const int tableN = sizeof(pixelsPtr)/sizeof(pixelsPtr[0]);
  
  uint8_t tableOffsets[] = {
    10, 6, 3, 2,
    4,  0, 0, 3,
    1,  0, 0, 8,
    3,  5, 7, 9
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  uint32_t expected[] = {
     0,  1,  4,  5,
     2,  3,  6,  7,
    11, 10, 14, 15,
     8, 12,  9, 13
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  const bool tablePred = true;
  
  CTI_Iterate(nullptr,
              pixelsPtr,
              tableN,
              tableOffsets,
              tablePred,
              dim, dim,
              iterOrder,
              nullptr);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// A slope that goes down and to the right, 4x4.

- (void) testColortableDR4By4 {
  
  // int order pixels : CurDR4x4.png

//  pixels[  0] = 0x00000000 : 0 0 0
//  pixels[  1] = 0x00060D12 : 6 13 18
//  pixels[  2] = 0x0006131A : 6 19 26
//  pixels[  3] = 0x00071926 : 7 25 38
//  pixels[  4] = 0x00080F15 : 8 15 21
//  pixels[  5] = 0x00091218 : 9 18 24
//  pixels[  6] = 0x00234662 : 35 70 98
//  pixels[  7] = 0x00264A65 : 38 74 101
//  pixels[  8] = 0x00264D69 : 38 77 105
//  pixels[  9] = 0x00269AE3 : 38 154 227
//  pixels[ 10] = 0x00274B66 : 39 75 102
//  pixels[ 11] = 0x00274C67 : 39 76 103
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0 0 0
    0x00060D12, // 6 13 18
    0x0006131A, // 6 19 26
    0x00071926, // 7 25 38
    0x00080F15, // 8 15 21
    0x00091218, // 9 18 24
    0x00234662, // 35 70 98
    0x00264A65, // 38 74 101
    0x00264D69, // 38 77 105
    0x00269AE3, // 38 154 227
    0x00274B66, // 39 75 102
    0x00274C67  // 39 76 103
  };
  
  uint8_t tableOffsets[] = {
    1, 6, 8, 11,
    0, 1, 7, 11,
    3, 0, 4, 10,
    9, 2, 0,  5
  };
  
//  uint32_t order[] = {
//     0,  1,  2,  3,
//     4,  5,  6,  7,
//     8,  9, 10, 11,
//    12, 13, 14, 15
//  };
  
  uint32_t expected[] = {
     0,  1,  4,  5,
     6,  8, 12,  2,
     3, 10, 14,  9,
    13, 15, 11,  7
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  CTI_IteratePixelsInTable(pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              false,
              dim, dim,
              iterOrder);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// Use CurDR4x4.png image but apply BW sort order to the colortable, this
// changes the table sort order but does not change distances between colors.

- (void) testColortableDR4By4BW {
  
  // B->W sorted order pixels : CurDR4x4.png
  
//  sorted order pixels:
//  pixels[  0] = 0x00000000 : 0 0 0
//  pixels[  1] = 0x00060D12 : 6 13 18
//  pixels[  2] = 0x00080F15 : 8 15 21
//  pixels[  3] = 0x00091218 : 9 18 24
//  pixels[  4] = 0x0006131A : 6 19 26
//  pixels[  5] = 0x00071926 : 7 25 38
//  pixels[  6] = 0x00234662 : 35 70 98
//  pixels[  7] = 0x00264A65 : 38 74 101
//  pixels[  8] = 0x00274B66 : 39 75 102
//  pixels[  9] = 0x00274C67 : 39 76 103
//  pixels[ 10] = 0x00264D69 : 38 77 105
//  pixels[ 11] = 0x00269AE3 : 38 154 227
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0 0 0
    0x00060D12, // 6 13 18
    0x00080F15, // 8 15 21
    0x00091218, // 9 18 24
    0x0006131A, // 6 19 26
    0x00071926, // 7 25 38
    0x00234662, // 35 70 98
    0x00264A65, // 38 74 101
    0x00274B66, // 39 75 102
    0x00274C67, // 39 76 103
    0x00264D69, // 38 77 105
    0x00269AE3 // 38 154 227
  };
  
  uint8_t tableOffsets[] = {
    1,  6, 10, 9,
    0,  1,  7, 9,
    5,  0,  2, 8,
    11, 4,  0, 3
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  // Note that the iter order here is the same, it currently depends only on
  // the pixel values as opposed to the table deltas.
  
  uint32_t expected[] = {
    0,  1,  4,   5,
    6,  8,  12,  2,
   10,  3,  14,  9,
   13, 11,  15,  7
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  CTI_Iterate(nullptr,
              pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              true,
              dim, dim,
              iterOrder,
              nullptr);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// bwbox.png where the colortable contains only 2 entries. This is a tricky case
// since the delta is +-1 either way in the table.

- (void) testColortableBox2Colors4By4BW {
  
  // B->W sorted order pixels : bwbox4x4.png
  
  //  sorted order pixels:
  // pixels[  0] = 0x00000000
  // pixels[  1] = 0x00FFFFFF
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0 0 0
    0x00FFFFFF  // 255 255 255
  };
  
  uint8_t tableOffsets[] = {
    1,1,1,1,
    1,0,0,1,
    1,0,0,1,
    1,1,1,1
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //     8,  9, 10, 11,
  //    12, 13, 14, 15
  //  };
  
  // Note that the iter order here is the same, it currently depends only on
  // the pixel values as opposed to the table deltas.
  
  uint32_t expected[] = {
    0,  1,  4,   5,
    2,  3,  8,  12,
    6,  7,  11, 15,
   10,  14,  9, 13
  };
  
  const int dim = 4;
  
  vector<uint32_t> iterOrder;
  
  CTI_Iterate(nullptr,
              pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              true,
              dim, dim,
              iterOrder,
              nullptr);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 16);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, 4, 4);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, 4, 4);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

// 4x2 input where left 2x2 box is black and right 2x2 box is blue

- (void) test4x2BlackBlue {
  
  // B->W sorted order pixels : bwbox4x4.png
  
  //  sorted order pixels:
  // pixels[  0] = 0x00000000
  // pixels[  1] = 0x00FFFFFF
  
  uint32_t pixelsPtr[] = {
    0x00000000, // 0 0 0
    0x003AA3ED    // 58 163 237
  };
  
  uint8_t tableOffsets[] = {
    0,0,1,1,
    0,0,1,1,
  };
  
  //  uint32_t order[] = {
  //     0,  1,  2,  3,
  //     4,  5,  6,  7,
  //  };
  
  // Note that the iter order here is the same, it currently depends only on
  // the pixel values as opposed to the table deltas.
  
  uint32_t expected[] = {
    0,  1,  4,   5,
    6,  2,  3,   7
  };
  
  int width = 4;
  int height = 2;
  
  vector<uint32_t> iterOrder;
  
  CTI_Iterate(nullptr,
              pixelsPtr,
              (int)sizeof(pixelsPtr)/sizeof(pixelsPtr[0]),
              tableOffsets,
              true,
              width, height,
              iterOrder,
              nullptr);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  XCTAssert(iterOrder.size() == 8);
  
  vector<vector<uint32_t> > iterOrderMat = format2DVec(iterOrder, width, height);
  
  vector<vector<uint32_t> > expectedMat = format2DVec(expected, width, height);
  
  XCTAssert(iterOrderMat == expectedMat);
  
  for ( auto & row : iterOrderMat ) {
    cout << keyFromVec(row) << endl;
  }
  
  cout << "done" << endl;
  
  return;
}

@end

