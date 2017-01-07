//
//  BitFlags2D.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Bit flags that represent an 8x8 two dimensional set
//  of either true or false settings. This API contains
//  a couple of specific details for performance, first
//  the offsets into the bits are in terms of (X,Y)
//  coordinates with a fixed constant size of 8x8.
//  This makes it possible to access specific bits
//  without only shift and addition operations.

#include "assert.h"

#include <string>
#include <sstream>

#include <vector>
#include <queue>
#include <set>

#if defined(DEBUG)
#include <iostream>
#endif // DEBUG

using namespace std;

// Each set of 64 bits represents and 8x8 block of bits

#define BitFlags2DN 8
#define BitFlags2DNShift 3

class BitFlags2D {
public:
  // Empty constructor
  
  BitFlags2D()
  : _bits(0)
  {
  }
  
  BitFlags2D(uint64_t flags)
  : _bits(flags)
  {
  }
  
  // Calculate offset given 2D coordinates
  
  unsigned int offset(unsigned int x, unsigned int y) const {
    unsigned int off = (y << BitFlags2DNShift) + x;
#if defined(DEBUG)
    assert(off >= 0);
    assert(off < 64);
#endif // DEBUG
    return off;
  }

  // Return bit flag at (X,Y) location in 2D field
  
  bool isSet(unsigned int x, unsigned int y) const {
    unsigned int off = offset(x,y);
    unsigned int shift = ((BitFlags2DN*BitFlags2DN - 1) - off);
    bool isFlagSet = (_bits >> shift) & 0x1;
    return isFlagSet;
  }

  // Set the bit flag at (X,Y) to true
  
  void setBit(unsigned int x, unsigned int y) {
    unsigned int off = offset(x,y);
    unsigned int shift = ((BitFlags2DN*BitFlags2DN - 1) - off);
    uint64_t mask = 0x1;
    _bits |= (mask << shift);
    return;
  }

  // Clear the bit flag at (X,Y)
  
  void clearBit(unsigned int x, unsigned int y) {
    unsigned int off = offset(x,y);
    unsigned int shift = ((BitFlags2DN*BitFlags2DN - 1) - off);
    uint64_t mask = 0x1;
    _bits &= ~(mask << shift);
    return;
  }
  
  // Clear all bits
  
  void clearAllBits() {
    _bits = 0;
  }
  
  // Get current flags as 64 bit number
  
  uint64_t bits() const {
    return _bits;
  }
  
  // Set current bits flags from a 64 bit number

  void setBits(uint64_t inBits) {
    _bits = inBits;
  }
  
  string toString() const {
    stringstream s;
    
    for ( int y = 0; y < BitFlags2DN; y++) {
      for ( int x = 0; x < BitFlags2DN; x++) {
        bool isFlagSet = isSet(x, y);
        if (isFlagSet) {
          s << "1" << " ";
        } else {
          s << "0" << " ";
        }
      }
      s << "\n";
    }
    return s.str();
  }
  
  // Operator equals is a fast primitive type compare
  
  bool operator== (const BitFlags2D & rhs) const
  {
    return _bits == rhs._bits;
  }

  bool operator!= (const BitFlags2D & rhs) const
  {
    return _bits != rhs._bits;
  }
  
private:
  // The bits are contained in an array of 64 bit unsigned numbers

  uint64_t _bits;
  
};

