//
// Copyright 2016 Mo DeJong.
//
// See LICENSE for terms.

#include "PngContext.h"

#include "CalcError.h"

#include <unordered_map>
#include <vector>

#include <iostream>

#include <assert.h>

#include <time.h>

#include "ColortableIter.hpp"

using namespace std;

static
clock_t
start_timer ( void );

static
double
stop_timer ( const clock_t start_time );

static
void dump_iter_n(int width, int height, int hasAlpha, uint32_t *inPixels, const vector<uint32_t> & iterOrder, int * iterStepPtr);

static
void dump_rgb(int width, int height, int hasAlpha, uint32_t * pixelsPtr, char *filename);

static
void dump_grayscale(int width, int height, uint8_t * offsetsPtr, char * filename);

static
void dump_colortable(int hasAlpha, uint32_t * colortablePtr, int numColors);

void
__attribute__ ((noinline))
process_file(PngContext *cxt)
{
  int inputImageNumPixels = cxt->width * cxt->height;
  
  printf("read  %d pixels from input image\n", inputImageNumPixels);
  
  // Determine if this image has been processed into a LTEQ 256 colortable
  
  bool enable256 = false;
  
  unordered_map<uint32_t, uint32_t> histogram;
  
  if (enable256) {
  
  for (int i = 0; i < inputImageNumPixels; i++) {
    uint32_t pixel = cxt->pixels[i];
    histogram[pixel] += 1;
  }
  
  }
    
  int numUniquePixels = (int) histogram.size();
  
  printf("scanned  %d unique pixels in input image\n", numUniquePixels);
  
  // Grayscale input checking
  
  bool checkGrayscale = true;
  bool isGrayscale = false;
  
  if (checkGrayscale) {
    isGrayscale = true;

    // Note that reading (cxt->color_type == PNG_COLOR_TYPE_GRAY) is not
    // processing byte input properly. Need to convert to RGB that contains grayscale.
    
    // This check does not work for grayscale PNG input
//    if (cxt->color_type == PNG_COLOR_TYPE_GRAY) {
//      isGrayscale = true;
//    }

    for (int i = 0; i < inputImageNumPixels; i++) {
      uint32_t pixel = cxt->pixels[i];
      
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
//      printf("pixels[%5d] = 0x%08X\n", i, pixel);
      
      if (B == G && B == R) {
        // pixel is grayscale
      } else {
        isGrayscale = false;
        break;
      }
    }
  }
  
  // Generate 8 bit offsets table that will be used to lookup table deltas
  
  uint8_t *colortableOffsets = nullptr;
  uint32_t *colortablePixels = nullptr;
  uint32_t *deltasPtr = nullptr;
  
  if (enable256 && numUniquePixels <= 256) {
    colortableOffsets = new uint8_t[inputImageNumPixels]();
    
    // Create lookup table
    
    unordered_map<uint32_t, uint8_t> pixelToOffset;
    int tableOffset = 0;
    
    // Simple int order sort based on pixel values
    
    vector<uint32_t> sortVec;
    
    for ( auto & pair : histogram ) {
      uint32_t pixel = pair.first;
      sortVec.push_back(pixel);
    }
    
    sort(begin(sortVec), end(sortVec));
    
    for ( auto pixel : sortVec ) {
      pixelToOffset[pixel] = tableOffset++;
    }
    
    // Lookup each pixel in the table
    
    for (int i = 0; i < inputImageNumPixels; i++) {
      uint32_t pixel = cxt->pixels[i];
      uint8_t pixelOffset = pixelToOffset[pixel];
      colortableOffsets[i] = pixelOffset;
    }
    
    // Dump indexes output
    
    dump_grayscale(cxt->width, cxt->height, colortableOffsets, (char*)"iter_grayscale.png");
    
    // Colortable
    
    colortablePixels = new uint32_t[numUniquePixels]();
    
    {
      int i = 0;
      
      for ( auto & pair : pixelToOffset ) {
        uint32_t pixel = pair.first;
        colortablePixels[i++] = pixel;
      }
    }
    
    dump_colortable(cxt->hasAlpha, colortablePixels, numUniquePixels);
  }
  
  clock_t startT;
  double elapsed;
  
  vector<uint32_t> iterOrder;
  
  const int numIterationLoops = 10;
  
  if (isGrayscale) {
    uint8_t *grayscaleBytes = new uint8_t[inputImageNumPixels]();
    
    for (int i = 0; i < inputImageNumPixels; i++) {
      uint32_t pixel = cxt->pixels[i];

      //printf("pixels[%5d] = 0x%08X\n", i, pixel);
      
      uint32_t B = pixel & 0xFF;
      grayscaleBytes[i] = B;
    }
    
    // Dump indexes output
    
    dump_grayscale(cxt->width, cxt->height, grayscaleBytes, (char*)"in_grayscale_bytes.png");

    startT = start_timer();
    
    for (int i = 0; i < numIterationLoops; i++)
    {
      CTI_IterateGray(grayscaleBytes,
                      cxt->width, cxt->height,
                      iterOrder,
                      nullptr);
    }
    
    elapsed = stop_timer(startT);
    
    delete [] grayscaleBytes;
  } else if (enable256 && numUniquePixels <= 256) {
    // 1 component colortable index processing
    
    startT = start_timer();
    
    for (int i = 0; i < numIterationLoops; i++)
    {
      CTI_IterateTable256(
                  colortablePixels,
                  numUniquePixels,
                  colortableOffsets,
                  cxt->width, cxt->height,
                  iterOrder);
      
    }
    
    elapsed = stop_timer(startT);
  } else {
    // 3 component RGB processing
    
    // Note that generating deltas can have a significant impact on performance
    // since each pixel has to determine a predicted delta pixel.
    
    const bool genDeltas = true;
    
    if (genDeltas) {
      deltasPtr = new uint32_t[inputImageNumPixels]();
    }
    
    startT = start_timer();
    
    for (int i = 0; i < numIterationLoops; i++)
    {
      CTI_IterateRGB(cxt->pixels,
                  cxt->width, cxt->height,
                  iterOrder,
                  deltasPtr);
      
    }
    
    elapsed = stop_timer(startT);
    
    if (genDeltas) {
      dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, deltasPtr, (char*)"iter_deltas.png");
    }
    
    // Convert the RGB deltas centered around zero to abs()
    // applied to each component so that the output result
    // is a grayscale positive or negative delta.
    
    if (genDeltas) {
      uint32_t *absDeltasPtr = new uint32_t[inputImageNumPixels]();
      
      for (int i = 0; i < inputImageNumPixels; i++)
      {
        if (i == 0 || i == 1 || (i == cxt->width) || (i == cxt->width+1)) {
          // Skip abs and emit zero in the upper left corner pixels case
          absDeltasPtr[i] = 0;
        } else {
          uint32_t pixel = deltasPtr[i];
          uint32_t absPixel = abs_of_each_component(pixel);
          absDeltasPtr[i] = absPixel;
        }
      }
      
      dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, absDeltasPtr, (char*)"iter_abs_deltas.png");
      
      delete [] absDeltasPtr;
    }
    
    if (genDeltas) {
    // Out = Pred = orig + err
    
    // Write over the first 4 pixels in upper left to simplify
    // delta calculations

    uint32_t *predPixelsPtr = new uint32_t[inputImageNumPixels]();
    
    for (int i = 0; i < inputImageNumPixels; i++)
    {
      if (i == 0 || i == 1 || (i == cxt->width) || (i == cxt->width+1)) {
        // Skip abs and emit zero in the upper left corner pixels case
        predPixelsPtr[i] = deltasPtr[i];
      } else {
        uint32_t origPixel = cxt->pixels[i];
        uint32_t deltaPixel = deltasPtr[i];
        uint32_t predPixel = pixel_component_sum(origPixel, deltaPixel, 3);
        predPixelsPtr[i] = predPixel;
      }
    }
    
    // Calculate abs and mean err
    
    if ((genDeltas)) {
      double cMAE = calc_combined_mean_abs_error(inputImageNumPixels, cxt->pixels, predPixelsPtr);
      
      fprintf(stdout, "combined MAE %0.8f\n", cMAE);
      
      double cMSE = calc_combined_mean_sqr_error(inputImageNumPixels, cxt->pixels, predPixelsPtr);
      
      fprintf(stdout, "combined MSE %0.8f\n", cMSE);
    }

    delete [] predPixelsPtr;
    }
  }
  
  printf("elapsed %.2f\n", elapsed);
  
  cout << "done : processed " << iterOrder.size() << endl;
  
  // Based on the iter order and in delta gen mode, format the
  // prediction error as iter order data, so that the error
  // values appear together in the output data stream.
  
  if (deltasPtr) {
    uint32_t *iterOrderedPixels = new uint32_t[inputImageNumPixels]();
    
    int i = 0;
    
    for ( uint32_t offset : iterOrder ) {
      uint32_t predErr = deltasPtr[offset];
      iterOrderedPixels[i] = predErr;
      i += 1;
    }
    
    dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, iterOrderedPixels, (char*)"iter_order_deltas.png");
    
    delete [] iterOrderedPixels;
  }

  if (deltasPtr) {
    uint32_t *iterOrderedPixels = new uint32_t[inputImageNumPixels]();
    
    int i = 0;
    
    for ( uint32_t offset : iterOrder ) {
      uint32_t predErr = deltasPtr[offset];
      uint32_t absPredErr = abs_of_each_component(predErr);
      iterOrderedPixels[i] = absPredErr;
      i += 1;
    }
    
    dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, iterOrderedPixels, (char*)"iter_order_abs_deltas.png");
    
    delete [] iterOrderedPixels;
  }
  
  delete [] colortableOffsets;
  delete [] colortablePixels;
  delete [] deltasPtr;
  
  // Dump iteration step images
  
  if ((0)) {
    int iterStepPtr = 0;
    
    for ( int i = 0 ; i < inputImageNumPixels; i++ ) {
      dump_iter_n(cxt->width, cxt->height, cxt->hasAlpha, cxt->pixels, iterOrder, &iterStepPtr);
    }
  }
  
  // Dump iteration step images
  
  if ((1)) {
    int iterStepPtr = 0;
    
    for ( int i = 0 ; i < inputImageNumPixels; i++ ) {
      if ((i % 1000) == 0) {
        dump_iter_n(cxt->width, cxt->height, cxt->hasAlpha, cxt->pixels, iterOrder, &iterStepPtr);
      } else {
        iterStepPtr += 1;
      }
    }
  }

  // Huge 4K steps
  
  if ((0)) {
    int iterStepPtr = 0;
    
    for ( int i = 0 ; i < inputImageNumPixels; i++ ) {
      if ((i % 25000) == 0) {
        dump_iter_n(cxt->width, cxt->height, cxt->hasAlpha, cxt->pixels, iterOrder, &iterStepPtr);
      } else {
        iterStepPtr += 1;
      }
    }
  }
  
  // Turn original image into an iteration ordered 1D representation
  // that orderes pixels in terms of gradient height.
  
  if ((1)) {
    uint32_t *iterOrderedPixels = new uint32_t[inputImageNumPixels]();
    
    int i = 0;
    
    for ( uint32_t offset : iterOrder ) {
      // Lookup pixel at offset
      uint32_t pixel = cxt->pixels[offset];
      iterOrderedPixels[i] = pixel;
      i += 1;
    }
    
    dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, iterOrderedPixels, (char*)"iter_order_pixels.png");
    
    delete [] iterOrderedPixels;
  }
  
  // Calculate and emit deltas from gradclamp process
  
  if ((1))
  {
    uint32_t * deltasPtr = nullptr;
    
    const bool genDeltas = true;
    
    if (genDeltas) {
      deltasPtr = new uint32_t[inputImageNumPixels]();
    }

    if (genDeltas) {
      gradclamp8by4_encode_pred_error(cxt->pixels,
                                      deltasPtr,
                                      0,
                                      cxt->width * cxt->height,
                                      cxt->width);
    }
    
    if (genDeltas) {
      dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, deltasPtr, (char*)"gradclamp_deltas.png");
    }
    
    // Convert the RGB deltas centered around zero to abs()
    // applied to each component so that the output result
    // is a grayscale positive or negative delta.
    
    if (genDeltas) {
      uint32_t *absDeltasPtr = new uint32_t[inputImageNumPixels]();
      
      for (int i = 0; i < inputImageNumPixels; i++)
      {
        if (i == 0 || i == 1 || (i == cxt->width) || (i == cxt->width+1)) {
          // Skip abs and emit zero in the upper left corner pixels case
          absDeltasPtr[i] = 0;
        } else {
          uint32_t pixel = deltasPtr[i];
          uint32_t absPixel = abs_of_each_component(pixel);
          absDeltasPtr[i] = absPixel;
        }
      }
      
      dump_rgb(cxt->width, cxt->height, cxt->hasAlpha, absDeltasPtr, (char*)"gradclamp_abs_deltas.png");
      
      delete [] absDeltasPtr;
    }
    
    if (genDeltas) {
      // Out = Pred = orig + err
      
      // Write over the first 4 pixels in upper left to simplify
      // delta calculations
      
      uint32_t *predPixelsPtr = new uint32_t[inputImageNumPixels]();
      
      for (int i = 0; i < inputImageNumPixels; i++)
      {
        if (i == 0 || i == 1 || (i == cxt->width) || (i == cxt->width+1)) {
          // Skip abs and emit zero in the upper left corner pixels case
          predPixelsPtr[i] = deltasPtr[i];
        } else {
          uint32_t origPixel = cxt->pixels[i];
          uint32_t deltaPixel = deltasPtr[i];
          uint32_t predPixel = pixel_component_sum(origPixel, deltaPixel, 3);
          predPixelsPtr[i] = predPixel;
        }
      }
      
      // Calculate abs and mean err
      
      if ((genDeltas)) {
        double cMAE = calc_combined_mean_abs_error(inputImageNumPixels, cxt->pixels, predPixelsPtr);
        
        fprintf(stdout, "combined MAE %0.8f\n", cMAE);
        
        double cMSE = calc_combined_mean_sqr_error(inputImageNumPixels, cxt->pixels, predPixelsPtr);
        
        fprintf(stdout, "combined MSE %0.8f\n", cMSE);
      }
      
      delete [] predPixelsPtr;
    }
  }
  
  return;
}

// Dump an image that duplicates the original but then adds white pixel over
// each step that is part of the iteration.

static
void dump_iter_n(int width, int height, int hasAlpha, uint32_t *inPixels, const vector<uint32_t> & iterOrder, int * iterStepPtr) {
  const int inputImageNumPixels = width * height;
  
  PngContext dumpCxt;
  PngContext_init(&dumpCxt);
  PngContext_settings(&dumpCxt, hasAlpha);
  PngContext_alloc_pixels(&dumpCxt, width, height);
  
  uint32_t *outPixels = dumpCxt.pixels;
  
  memcpy(outPixels, inPixels, inputImageNumPixels * sizeof(uint32_t));
  
  int istep = *iterStepPtr;
  
  for ( int i = 0 ; i < (istep + 1); i++ ) {
    uint32_t offset = iterOrder[i];
    outPixels[offset] = 0xFFFF0000;
  }
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "iter%d.png", istep);
  *iterStepPtr += 1;
    
  write_png_file((char*)&buffer[0], &dumpCxt);
    
  printf("wrote iter step %d as \"%s\"\n", istep, buffer);
    
  PngContext_dealloc(&dumpCxt);
  
  return;
}

static
void dump_rgb(int width, int height, int hasAlpha, uint32_t * pixelsPtr, char *filename) {
  const int inputImageNumPixels = width * height;
  
  PngContext dumpCxt;
  PngContext_init(&dumpCxt);
  PngContext_settings(&dumpCxt, hasAlpha);
  PngContext_alloc_pixels(&dumpCxt, width, height);
  
  uint32_t *outPixels = dumpCxt.pixels;
  
  for ( int i = 0 ; i < inputImageNumPixels; i++ ) {
    uint32_t pixel = pixelsPtr[i];
    pixel |= (0xFF << 24);
    outPixels[i] = pixel;
  }
  
  write_png_file(filename, &dumpCxt);
  
  printf("wrote \"%s\"\n", filename);
  
  PngContext_dealloc(&dumpCxt);
  
  return;
}

static
void dump_grayscale(int width, int height, uint8_t * offsetsPtr, char * filename) {
  const int inputImageNumPixels = width * height;
  
  PngContext dumpCxt;
  PngContext_init(&dumpCxt);
  int hasAlpha = 0; // no alpha w grayscale output
  PngContext_settings(&dumpCxt, hasAlpha);
  PngContext_alloc_pixels(&dumpCxt, width, height);
  
  uint32_t *outPixels = dumpCxt.pixels;
  
  for ( int i = 0 ; i < inputImageNumPixels; i++ ) {
    uint32_t offset = offsetsPtr[i];
    assert(offset <= 0xFF);
    offset = offset & 0xFF;
    offset = (0xFF << 24) | (offset << 16) | (offset << 8) | (offset);
    outPixels[i] = offset;
  }
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "%s", (char*)filename);
  
  write_png_file((char*)&buffer[0], &dumpCxt);
  
  printf("wrote \"%s\"\n", buffer);
  
  PngContext_dealloc(&dumpCxt);
  
  return;
}

// Dump a colortable, typically of 256 or fewer colors

static
void dump_colortable(int hasAlpha, uint32_t * colortablePtr, int numColors) {
  PngContext dumpCxt;
  PngContext_init(&dumpCxt);
  PngContext_settings(&dumpCxt, hasAlpha);
  PngContext_alloc_pixels(&dumpCxt, numColors, 1);
  
  uint32_t *outPixels = dumpCxt.pixels;
  
  for ( int i = 0 ; i < numColors; i++ ) {
    uint32_t pixel = colortablePtr[i];
    outPixels[i] = pixel;
  }
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "iter_colortable.png");
  
  write_png_file((char*)&buffer[0], &dumpCxt);
  
  printf("wrote \"%s\"\n", buffer);
  
  PngContext_dealloc(&dumpCxt);
  
  return;
}

static
clock_t
start_timer ( void )
{
  return clock();
}

static
double
stop_timer ( const clock_t start_time )
{
  return ( ( double ) ( clock (  ) - start_time ) ) / CLOCKS_PER_SEC;
}

// deallocate memory

void cleanup(PngContext *cxt)
{
  PngContext_dealloc(cxt);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage miniterorder PNG\n");
    exit(1);
  }
  PngContext cxt;
  fprintf(stdout, "reading PNG \"%s\"\n", argv[1]);
  read_png_file(argv[1], &cxt);
  
  if ((0)) {
    // Write input data just read back out to a PNG image to make sure read/write logic
    // is dealing correctly with wacky issues like grayscale and palette images
    
    PngContext copyCxt;
    PngContext_init(&copyCxt);
    PngContext_copy_settngs(&copyCxt, &cxt);
    PngContext_alloc_pixels(&copyCxt, cxt.width, cxt.height);
    
    uint32_t *inOriginalPixels = cxt.pixels;
    uint32_t *outPixels = copyCxt.pixels;
    
    int numPixels = cxt.width * cxt.height;
    
    for (int i = 0; i < numPixels; i++) {
      uint32_t inPixel = inOriginalPixels[i];
      outPixels[i] = inPixel;
    }
    
    char *inoutFilename = (char*)"in_out.png";
    write_png_file((char*)inoutFilename, &copyCxt);
    printf("wrote input copy to %s\n", inoutFilename);
    PngContext_dealloc(&copyCxt);
  }

  printf("processing %d pixels from image of dimensions %d x %d\n", cxt.width*cxt.height, cxt.width, cxt.height);
  
  process_file(&cxt);
  
  cleanup(&cxt);
  return 0;
}
