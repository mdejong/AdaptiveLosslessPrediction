//
// Copyright 2016 Mo DeJong.
//
// See LICENSE for terms.

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include "png.h"

void abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

typedef struct {
  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  int hasAlpha;
  
  png_structp png_ptr;
  png_infop info_ptr;
  int number_of_passes;
  png_bytep * row_pointers;
  
  uint32_t *pixels;
} PngContext;

void PngContext_init(PngContext *cxt) {
  cxt->pixels = NULL;
  cxt->row_pointers = NULL;
}

// Init settings on context, if the isAlpha flag is true then
// pixels will be treated as BGRA, otherwise BGR.

void PngContext_settings(PngContext *cxt, int hasAlpha) {
  if (hasAlpha) {
    cxt->color_type = PNG_COLOR_TYPE_RGBA;
  } else {
    cxt->color_type = PNG_COLOR_TYPE_RGB;
  }
  cxt->bit_depth = 8;
  cxt->number_of_passes = 1;
}

// Define settings on toCxt based on the settings from fromCxt.
// Note that this logic only makes it possible to write 8 bit
// images that are BGR or BGRA pixels.

void PngContext_copy_settngs(PngContext *toCxt, PngContext *fromCxt) {
  int hasAlpha = fromCxt->hasAlpha;
  PngContext_settings(toCxt, hasAlpha);
}

void PngContext_alloc_pixels(PngContext *cxt, int width, int height) {
  cxt->width = width;
  cxt->height = height;
  
  /* allocate pixels data and read into array of pixels */
  
  cxt->pixels = (uint32_t*) malloc(cxt->width * cxt->height * sizeof(uint32_t));
  
  if (cxt->pixels == NULL) {
    abort_("[PngContext_alloc_pixels] could not allocate %d bytes to store pixel data", (cxt->width * cxt->height * sizeof(uint32_t)));
  }
}

// BGR or BGRA data will be read into this buffer of pixels

const int debugPrintPixelsReadAndWritten = 0;

void allocate_row_pointers(PngContext *cxt)
{
  if (cxt->row_pointers != NULL) {
    abort_("[allocate_row_pointers] row_pointers already allocated");
  }
  
  cxt->row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * cxt->height);
  
  if (cxt->row_pointers == NULL) {
    abort_("[allocate_row_pointers] could not allocate %d bytes to store row data", (sizeof(png_bytep) * cxt->height));
  }
  
  int y;
  
  for (y=0; y < cxt->height; y++) {
    cxt->row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(cxt->png_ptr,cxt->info_ptr));
    
    if (cxt->row_pointers[y] == NULL) {
      abort_("[allocate_row_pointers] could not allocate %d bytes to store row data", png_get_rowbytes(cxt->png_ptr,cxt->info_ptr));
    }
  }
}

void free_row_pointers(PngContext *cxt)
{
  if (cxt->row_pointers == NULL) {
    return;
  }
  
  int y;
  
  for (y=0; y < cxt->height; y++) {
    free(cxt->row_pointers[y]);
  }
  free(cxt->row_pointers);
  cxt->row_pointers = NULL;
}

void read_png_file(char* file_name, PngContext *cxt)
{
  char header[8];    // 8 is the maximum size that can be checked
  
  PngContext_init(cxt);
  
  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp)
    abort_("[read_png_file] File %s could not be opened for reading", file_name);
  fread(header, 1, 8, fp);
  if (png_sig_cmp((png_const_bytep)header, 0, 8))
    abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);
  
  /* initialize stuff */
  cxt->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
  if (cxt->png_ptr == NULL)
    abort_("[read_png_file] png_create_read_struct failed");
  
  cxt->info_ptr = png_create_info_struct(cxt->png_ptr);
  if (cxt->info_ptr == NULL)
    abort_("[read_png_file] png_create_info_struct failed");
  
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[read_png_file] Error during init_io");
  
  png_init_io(cxt->png_ptr, fp);
  png_set_sig_bytes(cxt->png_ptr, 8);
  
  png_read_info(cxt->png_ptr, cxt->info_ptr);
  
  int width = png_get_image_width(cxt->png_ptr, cxt->info_ptr);
  int height = png_get_image_height(cxt->png_ptr, cxt->info_ptr);
  
  PngContext_alloc_pixels(cxt, width, height);
  
  cxt->color_type = png_get_color_type(cxt->png_ptr, cxt->info_ptr);
  cxt->bit_depth = png_get_bit_depth(cxt->png_ptr, cxt->info_ptr);
  
  cxt->number_of_passes = png_set_interlace_handling(cxt->png_ptr);

  if (cxt->bit_depth > 8) {
    abort_("[read_png_file] PNG with bit depth larger than 8 not supported");
  }
  
  /* read file */
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[read_png_file] Error during read_image");
  
  int isBGRA = 0;
  
  png_byte ctByte = png_get_color_type(cxt->png_ptr, cxt->info_ptr);
    
  if (ctByte == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(cxt->png_ptr);
    
    if (cxt->bit_depth < 8) {
      png_set_packing(cxt->png_ptr);
    }
  }
  
  if (ctByte == PNG_COLOR_TYPE_GRAY && cxt->bit_depth < 8) {
    png_set_gray_to_rgb(cxt->png_ptr);
  }
  
  if (ctByte & PNG_COLOR_MASK_ALPHA) {
    isBGRA = 1;
  } else {
    isBGRA = 0;
  }
  
  if (png_get_valid(cxt->png_ptr, cxt->info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(cxt->png_ptr);
    isBGRA = 1;
  }
  
  cxt->hasAlpha = isBGRA;
  
  int pixeli = 0;

  allocate_row_pointers(cxt);
  png_read_update_info(cxt->png_ptr, cxt->info_ptr);
  png_read_image(cxt->png_ptr, cxt->row_pointers);
  
  for (int y=0; y < cxt->height; y++) {
    png_byte* row = cxt->row_pointers[y];
    
    if (isBGRA) {
      for (int x=0; x < cxt->width; x++) {
        png_byte* ptr = &(row[x*4]);
        
        uint32_t B = ptr[2];
        uint32_t G = ptr[1];
        uint32_t R = ptr[0];
        uint32_t A = ptr[3];
        
        uint32_t pixel = (A << 24) | (R << 16) | (G << 8) | B;
        
        if (debugPrintPixelsReadAndWritten) {
          fprintf(stdout, "Read pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
        }

        if (debugPrintPixelsReadAndWritten) {
          if (A != 0 && A != 0xFF) {
            fprintf(stdout, "Read non opaque pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
          }
        }
        
        cxt->pixels[pixeli] = pixel;
        
        pixeli++;
      }
    } else {
      // BGR with no alpha channel
      for (int x=0; x < cxt->width; x++) {
        png_byte* ptr = &(row[x*3]);
        
        uint32_t B = ptr[2];
        uint32_t G = ptr[1];
        uint32_t R = ptr[0];
        
        uint32_t pixel = (0xFF << 24) | (R << 16) | (G << 8) | B;
        
        if (debugPrintPixelsReadAndWritten) {
          fprintf(stdout, "Read pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
        }
        
        cxt->pixels[pixeli] = pixel;
        
        pixeli++;
      }
    }
  }
  
  fclose(fp);
  
  free_row_pointers(cxt);
}


void write_png_file(char* file_name, PngContext *cxt)
{
  /* create file */
  FILE *fp = fopen(file_name, "wb");
  if (!fp)
    abort_("[write_png_file] File %s could not be opened for writing", file_name);
  
  
  /* initialize stuff */
  cxt->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
  if (cxt->png_ptr == NULL)
    abort_("[write_png_file] png_create_write_struct failed");
  
  cxt->info_ptr = png_create_info_struct(cxt->png_ptr);
  if (cxt->info_ptr == NULL)
    abort_("[write_png_file] png_create_info_struct failed");
  
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[write_png_file] Error during init_io");
  
  png_init_io(cxt->png_ptr, fp);
  
  
  /* write header */
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[write_png_file] Error during writing header");
  
  png_set_IHDR(cxt->png_ptr, cxt->info_ptr, cxt->width, cxt->height,
               cxt->bit_depth, cxt->color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  
  png_write_info(cxt->png_ptr, cxt->info_ptr);
  
  /* write pixels back to row_pointers */
  
  allocate_row_pointers(cxt);
  
  int isBGRA = 0;
  int isGrayscale = 0;
  
  png_byte ctByte = png_get_color_type(cxt->png_ptr, cxt->info_ptr);
  
  if (ctByte == PNG_COLOR_TYPE_RGBA) {
    isBGRA = 1;
  } else if (ctByte == PNG_COLOR_TYPE_RGB) {
    isBGRA = 0;
  } else if (ctByte == PNG_COLOR_TYPE_GRAY) {
    isGrayscale = 1;
  } else {
    abort_("[write_png_file] unsupported input format type");
  }
  
  int pixeli = 0;
  
  for (int y=0; y < cxt->height; y++) {
    png_byte* row = cxt->row_pointers[y];
    
    if (isGrayscale) {
      for (int x=0; x < cxt->width; x++) {
        png_byte* ptr = &(row[x]);
        
        uint32_t pixel = cxt->pixels[pixeli];
        
        uint32_t gray = pixel & 0xFF;
        
        ptr[0] = gray;
        
        if (debugPrintPixelsReadAndWritten) {
          fprintf(stdout, "Wrote pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
        }
        
        pixeli++;
      }
    } else if (isBGRA) {
      for (int x=0; x < cxt->width; x++) {
        png_byte* ptr = &(row[x*4]);
        
        uint32_t pixel = cxt->pixels[pixeli];
        
        uint32_t B = pixel & 0xFF;
        uint32_t G = (pixel >> 8) & 0xFF;
        uint32_t R = (pixel >> 16) & 0xFF;
        uint32_t A = (pixel >> 24) & 0xFF;
        
        ptr[0] = R;
        ptr[1] = G;
        ptr[2] = B;
        ptr[3] = A;
        
        if (debugPrintPixelsReadAndWritten) {
          fprintf(stdout, "Wrote pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
        }
        
        pixeli++;
      }
    } else {
      for (int x=0; x < cxt->width; x++) {
        png_byte* ptr = &(row[x*3]);
        
        uint32_t pixel = cxt->pixels[pixeli];
        
        uint32_t B = pixel & 0xFF;
        uint32_t G = (pixel >> 8) & 0xFF;
        uint32_t R = (pixel >> 16) & 0xFF;
        
        ptr[0] = R;
        ptr[1] = G;
        ptr[2] = B;
        
        if (debugPrintPixelsReadAndWritten) {
          fprintf(stdout, "Wrote pixel 0x%08X at (x,y) (%d, %d)\n", pixel, x, y);
        }
        
        pixeli++;
      }
    }
  }
  
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[write_png_file] Error during writing bytes");
  
  png_write_image(cxt->png_ptr, cxt->row_pointers);
  
  
  /* end write */
  if (setjmp(png_jmpbuf(cxt->png_ptr)))
    abort_("[write_png_file] Error during end of write");
  
  png_write_end(cxt->png_ptr, NULL);
  
  fclose(fp);
}

void PngContext_dealloc(PngContext *cxt)
{
  free_row_pointers(cxt);
  free(cxt->pixels);
}
