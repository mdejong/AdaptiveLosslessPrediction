//
// Copyright 2016 Mo DeJong.
//
// See LICENSE for terms.

// Signed add/sub for each component

#define RSHIFT_MASK(num, shift, mask) ((num >> shift) & mask)

#define MASK_LSHIFT(num, mask, shift) ((num & mask) << shift)

static inline
uint32_t component_addsub_8by4(uint32_t prev, uint32_t current, const bool doSub) {
  const bool debug = false;
  
  if (debug) {
    fprintf(stdout, "ADDSUB prev=0x%08X, current=0x%08X, doSub=%d\n", prev, current, (int)doSub);
  }
  
  uint8_t prev_c3 = RSHIFT_MASK(prev, 24, 0xFF);
  uint8_t prev_c2 = RSHIFT_MASK(prev, 16, 0xFF);
  uint8_t prev_c1 = RSHIFT_MASK(prev, 8, 0xFF);
  uint8_t prev_c0 = RSHIFT_MASK(prev, 0, 0xFF);
  
  uint8_t c3 = RSHIFT_MASK(current, 24, 0xFF);
  uint8_t c2 = RSHIFT_MASK(current, 16, 0xFF);
  uint8_t c1 = RSHIFT_MASK(current, 8, 0xFF);
  uint8_t c0 = RSHIFT_MASK(current, 0, 0xFF);
  
  if (debug) {
    fprintf(stdout,"prev_c3=%3d, prev_c2=%3d, prev_c1=%3d, prev_c0=%3d\n", prev_c3, prev_c2, prev_c1, prev_c0);
    fprintf(stdout,"     c3=%3d,      c2=%3d,      c1=%3d,      c0=%3d\n", c3, c2, c1, c0);
  }
  
  uint32_t r3, r2, r1, r0;
  
  if (doSub) {
    r3 = (c3 - prev_c3);
    r2 = (c2 - prev_c2);
    r1 = (c1 - prev_c1);
    r0 = (c0 - prev_c0);
  } else {
    r3 = (c3 + prev_c3);
    r2 = (c2 + prev_c2);
    r1 = (c1 + prev_c1);
    r0 = (c0 + prev_c0);
  }
  
  if (debug) {
    fprintf(stdout,"r3=%d, r2=%d, r1=%d, r0=%d\n", r3, r2, r1, r0);
  }
  
  // Mask to 8bit value and shift into component position
  
  uint32_t components =
  MASK_LSHIFT(r3, 0xFF, 24) |
  MASK_LSHIFT(r2, 0xFF, 16) |
  MASK_LSHIFT(r1, 0xFF, 8) |
  MASK_LSHIFT(r0, 0xFF, 0);
  
  if (debug) {
    fprintf(stdout,"return word 0x%0.8X\n", components);
  }
  
  return components;
}

// Given a pair of pixels, compute the abs value between the R,G,B components
// and return this abs() value with 0xFF as the alpha value.

static inline
uint32_t absError(uint32_t p1, uint32_t p2) {
  uint32_t deltaPixel = component_addsub_8by4(p1, p2, 1);
  
  int8_t c3, c2, c1, c0;
  
  //c3 = (pixel >> 24) & 0xFF;
  c2 = (deltaPixel >> 16) & 0xFF;
  c1 = (deltaPixel >> 8) & 0xFF;
  c0 = (deltaPixel >> 0) & 0xFF;
  
  if ((0)) {
    fprintf(stdout, "p2 0x%08X\n", p2);
    fprintf(stdout, "p1 0x%08X\n", p1);
    fprintf(stdout, "delta 0x%08X aka (%d %d %d)\n", deltaPixel, c2, c1, c0);
  }
  
  //c3 = abs(c3);
  c3 = 0xFF;
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
  
  c = c3;
  absPixel |= (c << 24);
  
  if ((0)) {
    fprintf(stdout, "abs delta 0x%08X aka (%d %d %d)\n", absPixel, c2, c1, c0);
  }
  
  return absPixel;
}

// Given a buffer that contains uint32_t pixels, calculate the absolute error
// sum of each pixel component.

static inline
void calc_sum_abs_error(uint32_t numPixels,
                        uint32_t *actual,
                        uint32_t *approx,
                        uint32_t *sumC0Ptr,
                        uint32_t *sumC1Ptr,
                        uint32_t *sumC2Ptr)
{
  uint32_t sumC0 = 0;
  uint32_t sumC1 = 0;
  uint32_t sumC2 = 0;
  
  for (int i = 0; i < numPixels; i++) {
    uint32_t actualPixel = actual[i];
    uint32_t approxPixel = approx[i];
    
    uint32_t errPixel = absError(actualPixel, approxPixel);
    uint32_t errC0 = errPixel & 0xFF;
    uint32_t errC1 = (errPixel >> 8) & 0xFF;
    uint32_t errC2 = (errPixel >> 16) & 0xFF;
    
    sumC0 += errC0;
    sumC1 += errC1;
    sumC2 += errC2;
  }
  
  *sumC0Ptr = sumC0;
  *sumC1Ptr = sumC1;
  *sumC2Ptr = sumC2;
  
  return;
}

// Given a buffer that contains uint32_t pixels, calculate the error
// squared for each pixel component

static inline
void calc_sum_sqr_error(uint32_t numPixels,
                        uint32_t *actual,
                        uint32_t *approx,
                        uint64_t *sumC0Ptr,
                        uint64_t *sumC1Ptr,
                        uint64_t *sumC2Ptr)
{
  uint64_t sumC0 = 0;
  uint64_t sumC1 = 0;
  uint64_t sumC2 = 0;
  
  for (int i = 0; i < numPixels; i++) {
    uint32_t actualPixel = actual[i];
    uint32_t approxPixel = approx[i];
    
    uint32_t errPixel = absError(actualPixel, approxPixel);
    uint32_t errC0 = errPixel & 0xFF;
    uint32_t errC1 = (errPixel >> 8) & 0xFF;
    uint32_t errC2 = (errPixel >> 16) & 0xFF;
    
    sumC0 += errC0 * errC0;
    sumC1 += errC1 * errC1;
    sumC2 += errC2 * errC2;
  }
  
  *sumC0Ptr = sumC0;
  *sumC1Ptr = sumC1;
  *sumC2Ptr = sumC2;
  
  return;
}

// mean abs error is the sum of each component error
// divided by the number of pixels

static inline
void calc_mean_abs_error(uint32_t numPixels,
                         uint32_t *actual,
                         uint32_t *approx,
                         uint32_t *meanC0Ptr,
                         uint32_t *meanC1Ptr,
                         uint32_t *meanC2Ptr)
{
  uint32_t sumC0 = 0;
  uint32_t sumC1 = 0;
  uint32_t sumC2 = 0;
  
  calc_sum_abs_error(numPixels, actual, approx, &sumC0, &sumC1, &sumC2);
  
  uint32_t meanC0 = sumC0 / numPixels;
  uint32_t meanC1 = sumC1 / numPixels;
  uint32_t meanC2 = sumC2 / numPixels;
  
  *meanC0Ptr = meanC0;
  *meanC1Ptr = meanC1;
  *meanC2Ptr = meanC2;
  
  return;
}

// Calculate combined mean absolute error as a double where the abs error
// for each component is added together.

static inline
double calc_combined_mean_abs_error(uint32_t numPixels,
                                    uint32_t *actual,
                                    uint32_t *approx)
{
  uint32_t sumC0 = 0;
  uint32_t sumC1 = 0;
  uint32_t sumC2 = 0;
  
  calc_sum_abs_error(numPixels, actual, approx, &sumC0, &sumC1, &sumC2);
  
  double cMAE = 0.0;
  
  cMAE += sumC0;
  cMAE += sumC1;
  cMAE += sumC2;
  
  cMAE /= numPixels;
  
  return cMAE;
}

// Calculate combined mean squared error as a double

static inline
double calc_combined_mean_sqr_error(uint32_t numPixels,
                                    uint32_t *actual,
                                    uint32_t *approx)
{
  uint64_t sumC0 = 0;
  uint64_t sumC1 = 0;
  uint64_t sumC2 = 0;
  
  calc_sum_sqr_error(numPixels, actual, approx, &sumC0, &sumC1, &sumC2);
  
  double cMAE = 0.0;
  
  cMAE += sumC0;
  cMAE += sumC1;
  cMAE += sumC2;
  
  cMAE /= numPixels;
  
  return cMAE;
}
