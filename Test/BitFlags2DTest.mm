//
//  BitFlags2DTest.mm
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  64 bit flags for 2D block

#import <XCTest/XCTest.h>

#import "BitFlags2D.hpp"

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

@interface BitFlags2DTest : XCTestCase

@end

@implementation BitFlags2DTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void) testFlagsNotSet {
  BitFlags2D bf2D;
  
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      bool isFlagSet = bf2D.isSet(x, y);
      XCTAssert(isFlagSet == false, @"isFlagSet");
    }
  }
}

- (void) testFlagsSet_0_0 {
  BitFlags2D bf2D;

  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bf2D.setBit(0, 0);
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bool isFlagSet = bf2D.isSet(0, 0);
  XCTAssert(isFlagSet == true, @"isFlagSet");
  
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (x == 0 && y == 0) {
        bool isFlagSet = bf2D.isSet(x, y);
        XCTAssert(isFlagSet == true, @"isFlagSet");
        continue;
      }
      
      bool isFlagSet = bf2D.isSet(x, y);
      XCTAssert(isFlagSet == false, @"isFlagSet : %d %d", x, y);
    }
  }
  
  {
  string str = bf2D.toString();
  printf("%s\n", str.c_str());
  }
}

- (void) testFlagsSet_0_1 {
  BitFlags2D bf2D;
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bf2D.setBit(1, 0);
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bool isFlagSet = bf2D.isSet(1, 0);
  XCTAssert(isFlagSet == true, @"isFlagSet");
  
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (x == 1 && y == 0) {
        bool isFlagSet = bf2D.isSet(x, y);
        XCTAssert(isFlagSet == true, @"isFlagSet");
        continue;
      }
      
      bool isFlagSet = bf2D.isSet(x, y);
      XCTAssert(isFlagSet == false, @"isFlagSet : %d %d", x, y);
    }
  }
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
}

- (void) testFlagsSet_7_7 {
  BitFlags2D bf2D;
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bf2D.setBit(7, 7);
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
  
  bool isFlagSet = bf2D.isSet(7, 7);
  XCTAssert(isFlagSet == true, @"isFlagSet");
  
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (x == 7 && y == 7) {
        bool isFlagSet = bf2D.isSet(x, y);
        XCTAssert(isFlagSet == true, @"isFlagSet");
        continue;
      }
      
      bool isFlagSet = bf2D.isSet(x, y);
      XCTAssert(isFlagSet == false, @"isFlagSet : %d %d", x, y);
    }
  }
  
  {
    string str = bf2D.toString();
    printf("%s\n", str.c_str());
  }
}

@end

