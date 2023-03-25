//************************************************************
//
// bmp2gbtile
//
// This program reads in a .bmp file and generates a 1D
// tile structure and color table in a .h file.  This .h file
// is in a format recognizable by GBA hardware
//
// Copyright (C) 2004 Matthew Wachowski
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
// 02111-1307, USA.
//
//************************************************************

#include "bmp2gbtile.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

//************************************************************
// Program entry point

int main(int argc, char* argv[]) {

  // Generate the object that does the work

  GenTiles gen_tiles;

  // Set default argument values

  int obj_offset = 0;
  char* classname = 0;
  bool tile_mode  = false;
  bool no_attr    = false;
  bool no_palette = false;
  const char* palette_filename = '\0';
  int lang = LANG_C;
  int file_count = argc - 2;

  // look for and parse command line args

  int i = 2;
  for (; i < argc; ++i) {
    
    if (argv[i][0] == '-') {

      if (!strcmp(argv[i], "-classname")) {
        ++i;
        if (i >= argc) {
          printf("Error: -classname requires an argument\n");
          return 1;
        }
        classname = argv[i];
        file_count -= 2;
      }
      else if (!strcmp(argv[i], "-obj_offset")) {
        ++i;
        if (i >= argc) {
          printf("Error: -obj_offset requires an argument\n");
          return 1;
        }
        obj_offset = atoi(argv[i]);
        if ((obj_offset < 0) || (obj_offset >= 1023)) {
          printf("Error: max obj_offset = 1023\n");
          return 1;
        }
        file_count -= 2;
      }
      else if (!strcmp(argv[i], "-tile")) {
        tile_mode = true;
        file_count -= 1;
      }
      else if (!strcmp(argv[i], "-no_attr")) {
        no_attr = true;
        file_count -= 1;
      }
      else if (!strcmp(argv[i], "-no_palette")) {
        no_palette = true;
        file_count -= 1;
      }
      else if (!strcmp(argv[i], "-palette_file")) {
        ++i;
        if (i >= argc) {
          printf("Error: -palette_file requires a filename\n");
          return 1;
        }
        palette_filename = argv[i];
        file_count -= 2;
      }
      else if (!strcmp(argv[i], "-lang")) {
        ++i;
        if (i >= argc) {
          printf("Error: -palette_file requires c or c++ as an argument\n");
          return 1;
        }
        if (!strcmp(argv[i], "c") || !strcmp(argv[i], "C")) {
          lang = LANG_C;
        }
        else if (!strcmp(argv[i], "c++") || 
                 !strcmp(argv[i], "C++") ||
                 !strcmp(argv[i], "cpp") ||
                 !strcmp(argv[i], "CPP")) {
          lang = LANG_CPP;
        }
        else {
          printf("Error: -palette_file requires c or c++ as an argument, %s was given\n", 
                 argv[i]);
          return 1;
        }
        file_count -= 2;
      }
      else {
        printf("unknown option: %s\n", argv[i]);
        return 1;
      }

    }

  }

  // If no arguments are left, no files were given...

  if (file_count < 1) {
    gen_tiles.printHelp();
    return 1;
  }

  // If the classname was not explicitly specified,
  // then match the classname with the given
  // filename

  if (!classname) {

    classname = new char[strlen(argv[1]) + 1];
    strcpy(classname, argv[1]);

    // Truncate the name at the .

    int j=0;
    for (; classname[j]; ++j) {
      if (classname[j] == '.') { classname[j] = '\0'; }
    }

  }

  printf("Using a output classname of %s\n", classname);

  // Initialize the generation object

  gen_tiles.init(file_count, tile_mode, no_attr, no_palette, 
                 lang, classname);

  // Generate the source files

  return gen_tiles.generate(argv[1], 
                            (const char**)(argv + 2), 
                            file_count,
                            obj_offset, 
                            palette_filename);

}

//************************************************************
// Print help to stdout

void GenTiles::printHelp() {

  const char* help_str =
 
    "\nusage: bmp2gbtile <output_file.h> <file1.bmp> ?file2.bmp? ... ?-classname <name>? ?-obj_offset <n>?\n\n"

    "This program converts a set of 16/256 color bmp files into GBA sprite/tile data\n\n"

    "The program takes one or more .bmp files as input and generates \n"
    "a single header file based on all read files.  This header will \n"
    "contain a single '1D' binary stream of the tile data along with an \n"
    "enumeration of palette information for each sprite.\n\n"

    "This program also generates an index structure that will give correct\n"
    "GBA hardware settings for each object.  The format of this structure\n"
    "will be the OBJ format\n\n"

    "This program will perform limited palette mactching using the following algorithm:\n"
    "  1) All bitmaps are loaded into memory 16x16 palettes are cleared\n"
    "  2) bitmaps are split into 2 groups (A,B) where 0 < A <= 16 < B < 256 colors\n"
    "  3) For each group A_i from A from the most colors to the least:\n"
    "     a) select a palette that has the lowest palette delta from A_i,\n"
    "        expand the palette to support A_i, remap A_i, and continue\n"
    "  4) Try to fit group B into existing palette.\n\n"
    "If any of the above steps fail, the program will not complete and\n"
    "will return an error\n\n"

    "Options include:\n\n"

    "  -classname <name>      The header file will be generated as a static\n"
    "                         class with a default classname of <output_file>.\n"
    "                         This option can be used to override this name.\n\n"

    "  -lang <name>           The language format to use.  Currently c or c++ is\n"
    "                         accepted as input\n\n"
    
    "  -obj_offset <n>        This can be used to set the object offset.  This\n"
    "                         parameter is useful when using bitmapped graphics\n"
    "                         modes (set obj_offset to 512 in these modes for\n"
    "                         correct header information\n\n"
    
    "  -tile                  This puts the bmp2gbtile program in tile mode.  In tile\n"
    "                         mode, any bmp dimension that is a multiple of 8 is accepted.\n"
    "                         in this mode, the bitmap is scanned from left to right and 8x8\n"
    "                         chunks are outputted\n\n"

    "  -no_attr               This surpresses attrN data from the output files\n\n"
    
    "  -no_palette            This surpresses palette data from the output files\n\n"         

    "  -palette_file <file>   This loads a specially formatted palette constraint file\n"
    "                         that can be used to manually constrain parts of a palette\n"
    "                         This is useful when combining the results of multiple\n"
    "                         Invovations on bmp2gbtile, when it makes sence to do things\n"
    "                         this way.\n\n"
    
    "The format of the palette file is simple, here is an example:\n\n"
    
    "# This is a comment\n\n"
    
    "# start a palette by stating 'pal' followed by a palette number (0-15)\n"
    "# if using 256 colors, simply use '0'\n\n"
    "pal 0\n"
    "# The rest is the colors in 5 bpp, BGR format, the same as the GBA\n"
    "# hardware would expect.  The color idx always starts at zero, there\n"
    "# is currently no way to start at a non zero index.  Palette data is\n"
    "# assumed hex\n"
    "#Here are two colors (0 and 1):\n"
    "7FFF\n"
    "0000\n\n";

  fprintf(stdout, "%s", help_str);

};

//************************************************************
// The main control for cpp/hpp generation, this program
// calls helper functions to convert "processed" paramters
// to source files

int GenTiles::generate(const char* output_file, const char** input_files, 
                       int input_file_count, int obj_offset, 
                       const char* palette_filename) {

  // Setup any manually constrained palette information

  if (loadPaletteFile(palette_filename)) {
    cleanup();
    return 1;
  }

  // Load all bitmaps into RAM

  if (loadAllFiles(input_files, input_file_count)) {
    cleanup();
    return 1;
  }
  
  // Try to build 16x16 color palettes that cover all loaded bitmaps
  // modify bitmap indexes, if needed

  if (buildPalette()) {
    cleanup();
    return 1;
  }

  // Now that we have all the data we need, create the final output file

  if (createOutput(output_file, obj_offset)) {
    cleanup();
    return 1;
  }

  // All done

  printf("\n--- Process Completed ---\n\n");
  
  return 0;

}

//************************************************************
// This function clears class variables to initial values

void GenTiles::init(int input_file_count, bool _tile_mode, 
                    bool _no_attr, bool _no_palette, int _lang,
                    const char* _classname) {

  tile_mode  = _tile_mode;
  no_attr    = _no_attr;
  no_palette = _no_palette;
  lang       = _lang;

  bitmaps = new unsigned char*[input_file_count];
  memset(bitmaps, 0, sizeof(char*) * input_file_count);

  bitmap_names = new char*[input_file_count];
  memset(bitmap_names, 0, sizeof(char*) * input_file_count);

  classname = new char[strlen(_classname) + 1];
  strcpy(classname, _classname);

  bmp_count = 0;

  // clean out the color maps

  memset(col_map, 0, sizeof(col_map));
  memset(col_count, 0, sizeof(col_count));

}

//************************************************************
// This function frees resources after the process has
// completed

void GenTiles::cleanup() {

  int i = 0;
  for (; i < bmp_count; ++i) {
    delete[] bitmaps[i];
    delete[] bitmap_names[i];
  }

  delete[] bitmaps;
  delete[] bitmap_names;
  delete[] classname;

}

//************************************************************
// This function is a simple loop that calls loadFile() for
// each provided .bmp file

int GenTiles::loadAllFiles(const char** input_files, 
                           int input_file_count) {

  printf("\n--- Loading All Files ---\n");

  int i = 0;
  for (; i < input_file_count; ++i) {
    if (loadFile(input_files[i])) {
      return 1;
    }
  }
  
  return 0;

}

//************************************************************
// This function loads a .bmp file into memory.  Currently
// only 4-bit and 8-bi color palettes are supported
// only uncompressed .bmp files are currently supported

int GenTiles::loadFile(const char* filename) {

  // Try to open the file

  printf("\n%s: Opening...              ", filename);

  FILE* f = fopen(filename, "rb");

  if (!f) {
    printf("Failed... \n");
    return 1;
  }

  // The open succeeded, get the file size

  struct stat file_stat;
  if (stat(filename, &file_stat)) {
    printf("Failed to get filesize... \n");
    fclose(f);
    return 1;
  }

  int filesize = file_stat.st_size;

  if (filesize < (sizeof(BMP_Header) + 0x20)) {
    printf("File is too small to be valid (min size = %u)  \n",
           sizeof(BMP_Header) + 0x20);
    fclose(f);
    return 1;
  }

  printf("OK\n");

  // Read the file into memory

  bitmaps[bmp_count] = new unsigned char[filesize];


  printf("%s: Loading File...         ", filename);

  if (fread(bitmaps[bmp_count], 1, filesize, f) <= 0) {
    printf("Failed to load file.  \n");
    delete[] bitmaps[bmp_count];
    fclose(f);
    return 1;
  }
  fclose(f);
  printf("OK\n");

  bitmap_names[bmp_count] = new char[strlen(filename) + 1];
  strcpy(bitmap_names[bmp_count], filename);

  // Look at the contents to make sure everything is ok

  BMP_Header* bmp = (BMP_Header*)bitmaps[bmp_count];
  ++bmp_count;

  if (checkBMP(bmp, filename, filesize)) {
    return 1;
  }

  // BMP files are specified in 24-bit color.  We need to
  // reduce this to 16-bit color and compress palettes as
  // needed

  if (reduceColors(bmp, filename)) {
    return 1;
  }

  return 0;

}

//************************************************************
// This function processes the header of a .bmp file, filling
// in the BMP_Header structure as appropriate and returning
// an error if invalid/unsupported bmp parameters are
// detected

int GenTiles::checkBMP(BMP_Header* bmp, const char* filename, 
                       int filesize) {

  // Make sure our header is correct

  char c1 = bmp->file_hdr.type & 0xFF;
  char c2 = bmp->file_hdr.type >> 8;

  if ((c1 < 33) || (c1 > 126)) { c1 = '.'; }
  if ((c2 < 33) || (c2 > 126)) { c2 = '.'; }

  printf("%s: Type        = %c%c        ", filename, c1, c2);
  
  if (bmp->file_hdr.type != 0x4D42) {
    printf("ERROR\n");
    return 1;
  }
  else {
    printf("OK\n");
  }

  // Check the file size parameter

  printf("%s: File Size   = %-8u  ", filename, bmp->file_hdr.size);

  if (bmp->file_hdr.size != filesize) {
    printf("MISMATCH\n");
    return 1;
  }
  else {
    printf("OK\n");
  }

  // set both reserved fields to zero

  bmp->file_hdr.palette_idx = 0;
  bmp->file_hdr.reserved2   = 0;

  // make sure our data offset is ok

  if ((bmp->file_hdr.data_offset + bmp->info_hdr.image_size) > filesize) {
    printf("ERROR: data_offset + image_size > filesize.  Image data is corrupted\n");
    return 1;
  }

  // check width and height

  printf("%s: Dimensions  = %-2u x %-2u   ",
         filename, bmp->info_hdr.width,
         bmp->info_hdr.height);

  if (tile_mode) {

    if (bmp->info_hdr.width % 8) {
      printf("ERROR: width must be a multiple of 8\n");
      return 1;
    }

    if (bmp->info_hdr.height % 8) {
      printf("ERROR: height must be a multiple of 8\n");
      return 1;
    }

  }
  else {

    // The GBA hardware supports a limited set of possible
    // sprite dimensions.  These limitiations are
    // enforced by the switch statements below

    switch (bmp->info_hdr.width) {
    case 8:
    case 16:
    case 32:
    case 64:
      break;
    default:
      printf("ERROR: width must be 8, 16, 32 or 64.  \n");
      return 1;
    }

    switch (bmp->info_hdr.height) {
    case 8:
    case 16:
    case 32:
    case 64:
      break;
    default:
      printf("ERROR: height must be 8, 16, 32 or 64.  \n");
      return 1;
    }
  }

  printf("OK\n");

  // Check planes

  if (bmp->info_hdr.planes != 1) {
    printf("ERROR: planes != 1\n");
    return 1;
  }

  // Check bits per pixel

  printf("%s: bits/pixel  = %u         ", filename, 
         bmp->info_hdr.bit_count);

  if ((bmp->info_hdr.bit_count != 4) &&
      (bmp->info_hdr.bit_count != 8)) {
    printf("ERROR: bpp must be 4 or 8\n");
    return 1;
  }
  else {
    printf("OK\n");
  }

  // Check Compression Scheme

  printf("%s: compression = %u         ", filename, 
         bmp->info_hdr.compression);

  if (bmp->info_hdr.compression != 0) {
    printf("ERROR: this program does not support compressed images\n");
    return 1;
  }
  else {
    printf("OK\n");
  }
  
  // We are OK

  printf("%s: colors_used = %u         \n", filename, 
         bmp->info_hdr.cols_used);

  return 0;

}

//************************************************************
// This function converts the 24-bit palette specification
// native to .bmp formats to a 15-bit palette specification
// native to the GBA hardware

int GenTiles::reduceColors(BMP_Header* bmp, 
                           const char* filename) {

  printf("%s: reducing colors from 24->16 bit... ", filename);

  // Get a pointer to the start of the palette for this
  // bitmap

  unsigned int* pal = 
    (unsigned int*)(((unsigned char*)bmp) + sizeof(BMP_Header));

  // Reduce each color from 24 bits to 16 bits

  int i = 0;
  for (; i < bmp->info_hdr.cols_used; ++i) {

    unsigned int red   = (pal[i] >> 16) & 0xFF;
    unsigned int green = (pal[i] >> 8) & 0xFF;
    unsigned int blue  = pal[i] & 0xFF;

    pal[i] = ((blue >> 3) << 10) | ((green >> 3) << 5) | (red >> 3);

    //check for duplicates

    int j = 0;
    for (; j < i; ++j) {
      if (pal[j] == pal[i]) {
        deleteDuplicateColor(bmp, i, j);
        --i;
      }
    }

  }

  printf("OK\n");

  return 0;

}

//************************************************************
// Delete color:
//
// This:
//   1) changes the palette so that index del_idx is missing
//   2) goes throughh the bitmap and changes all del_idx values
//      to new_idx and all > del_idx values to --val.
//   3) decrements the cols_used count

int GenTiles::deleteDuplicateColor(BMP_Header* bmp, 
                                   int high_idx, int low_idx) {

  printf("(delete idx %u) ", high_idx);

  //first, fix the palette

  unsigned int* pal = 
    (unsigned int*)(((unsigned char*)bmp) + sizeof(BMP_Header));
  int next_idx = 0;
  int i = 0;
  
  for (; i < bmp->info_hdr.cols_used; ++i) {
    if (i != high_idx) {
      pal[next_idx] = pal[i];
      ++next_idx;
    }
  }

  // now decrement cols_used

  --bmp->info_hdr.cols_used;

  // now go through the bitmap and remap colors

  unsigned char* bitmap = 
    (unsigned char*)(bmp) + bmp->file_hdr.data_offset;
  int length = bmp->info_hdr.image_size;
  unsigned char col  = 0;
  unsigned char col2 = 0;

  // Finally, remap the indexed palette so that this 
  // color is no longer included

  for (i=0; i < length; ++i) {

    // check the bit width to decide on the
    // deletion procedure

    switch (bmp->info_hdr.bit_count) {
    case 4:

      // we actually have 2 pixels to process here

      col = bitmap[i] >> 4;
      if (col == high_idx) {
        col = low_idx;
      }
      else if (col > high_idx) {
        --col;
      }

      col2 = bitmap[i] & 0x0F;
      if (col2 == high_idx) {
        col2 = low_idx;
      }
      else if (col2 > high_idx) {
        --col2;
      }

      bitmap[i] = (col << 4) | col2;

      break;
      
    case 8:

      if (bitmap[i] == high_idx) {
        bitmap[i] = low_idx;
      }
      else if (bitmap[i] > high_idx) {
        --bitmap[i];
      }

      break;
      
    default:
      
      printf("ERROR: unexpected bit width: %u\n", 
             bmp->info_hdr.bit_count);
      return 1;

    }

  }
  
  return 0;

}

//************************************************************
// This function generates the color palettes
// the algorithm makes an attempt to pack in colors and
// reuse common colors between 16-color bmp images

int GenTiles::buildPalette() {

  printf("\n--- Building Palettes ---\n\n");

  // First build all of the 16 color palettes

  int i=16;
  for (; i > 1; --i) {
    
    int j = 0;
    for (; j < bmp_count; ++j) {

      BMP_Header* bmp = (BMP_Header*)bitmaps[j];
      
      if (bmp->info_hdr.cols_used == i) {
        if (build16(bmp, bitmap_names[j])) {
          return 1;
        }
      }

    }

  }

  // now try to build the 256 pallete

  for (i=0; i < bmp_count; ++i) {

    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
      
    if (bmp->info_hdr.cols_used > 16) {
      if (build256(bmp, bitmap_names[i])) {
        return 1;
      }
    }

  }

  return 0;

}

//************************************************************
// This function tries to merge the palette of the given
// .bmp file to the already defined set of color

int GenTiles::build16(BMP_Header* bmp, const char* name) {

  // A bit on the scoring
  // An empty palette scores as cols_used + 1
  // this is to encourage filling a partially used palette
  // over creating a new one.  therefore + 2 needs to
  // be used to allow + 1 to work

  int min_delta = bmp->info_hdr.cols_used + 2;
  int min_idx = -1;
  
  // Look for the palette that has the minimum number of differences

  unsigned int* pal = 
    (unsigned int*)(((unsigned char*)bmp) + sizeof(BMP_Header));

  int i=0;
  for (; i < 16; ++i) {
    
    int delta = getColorMapDelta(col_map[i], 
                                 col_count[i], pal, 
                                 bmp->info_hdr.cols_used);
    if (delta < min_delta) {
      min_delta = delta;
      min_idx = i;
    }

  }

  // Did we find anything?

  if (min_idx == -1) {
    printf("%s: Could not find space in any palette to include this 16 color bitmap\n", name);
    return 1;
  }

  // Ok, we did find something, adjust the palette as needed

  printf("%s: Using 16 color Palette %u for color map\n", 
         name, min_idx);

  bmp->file_hdr.palette_idx = min_idx;

  mergePalette16(col_map[min_idx], &(col_count[min_idx]), bmp);

  return 0;

}

//************************************************************
// This function looks for occurances of objc colors in pal
// The number returned is cols_used - matches
//
// 1 penalty point if the palette is empty

int GenTiles::getColorMapDelta(unsigned short* pal, int pal_len,
                               unsigned int* objc, int objc_len) {

  // check for the empty palette case

  if (pal_len == 0) {
    return objc_len + 1;
  } 

  int diff_score = objc_len;

  int i = 0;
  for (; i < objc_len; ++i) {
    int j = 0;
    for (; j < pal_len; ++j) {
      if (((short)objc[i]) == pal[j]) {
        --diff_score;
        j = pal_len;
      }
    }
  }

  // check for a palette overflow

  if ((pal_len + diff_score) > 16) {
    return 20;
  }

  return diff_score;

}

//************************************************************
// This function finishes the job started by the build16
// function

void GenTiles::mergePalette16(unsigned short* pal, 
                              unsigned char* pal_len, 
                              BMP_Header* bmp) {

  unsigned int* objc = 
    (unsigned int*)(((unsigned char*)bmp) + sizeof(BMP_Header));
  int num_cols = bmp->info_hdr.cols_used;

  unsigned char* bitmap = 
    ((unsigned char*)bmp) + bmp->file_hdr.data_offset;
  int length = bmp->info_hdr.image_size;

  // OK, remapping could feedback on itself (which is bad)
  // therefore, we will need to create a copy of the source bitmap
  // to give a clean copy source

  unsigned char* bitmap_orig = new unsigned char[length];
  memcpy(bitmap_orig, bitmap, length);

  // For the rest of the process, we find a match between objc
  // and pal.  If no match is found, we append to pal

  int i = 0;
  for (; i < num_cols; ++i) {

    int j = 0;
    for (; j < *pal_len; ++j) {
      if (pal[j] == (unsigned short)objc[i]) {
        //we have a match
        break;
      }
    }

    if (j == *pal_len) {
      //no match, do an append
      pal[*pal_len] = (unsigned short)objc[i];
      *pal_len = *pal_len + 1;
    }

    // Remap

    if (i != j) {
      int k = 0;
      for (; k < length; ++k) {
        unsigned char col1 = bitmap_orig[k] >> 4;
        unsigned char col2 = bitmap_orig[k] & 0x0F;  
 
        if (col1 == i) { 
          col1 = j; 
        }
        else {
          col1 = bitmap[k] >> 4;
        }

        if (col2 == i) { 
          col2 = j; 
        }
        else {
          col2 = bitmap[k] & 0x0F; 
        }

        bitmap[k] = (col1 << 4) | col2;
      }
    }

  }

  // free our working memory
  
  delete[] bitmap_orig;

}

//************************************************************
// This function tries to merge the defined colors of a 256
// color .bmp into existing palettes

int GenTiles::build256(BMP_Header* bmp, const char* name) {

  // The task here is simple:
  // for each defined color we try to find an existing match
  // if we can not find a match, we try to add to a non-full palette
  // if we can not do this, we generate an error

  unsigned int*  objc = 
    (unsigned int*)(((unsigned char*)bmp) + sizeof(BMP_Header));
  int num_cols = bmp->info_hdr.cols_used;

  unsigned char* bitmap = 
    ((unsigned char*)bmp) + bmp->file_hdr.data_offset;
  int length = bmp->info_hdr.image_size;

  unsigned char* bitmap_orig = new unsigned char[length];
  memcpy(bitmap_orig, bitmap, length);

  int i = 0;
  int j = 0;
  int k = 0;
  for (; i < num_cols; ++i) {

    int pal_number = -1;
    int pal_idx    = -1;

    // first look for an existing color in any of the 
    // 16 defined palletes that matches this entry

    for (j = 0; (pal_number < 0) && (j < 16); ++j) {
      for (k = 0; (pal_number < 0) && 
             (k < col_count[pal_number]); ++k) {
        if (col_map[j][k] == objc[i]) {
          pal_number = j;
          pal_idx    = k;
        }
      }
    }

    // if we still did not find a match, look for a "free slot"

    if (pal_number < 0) {

      for (j = 0; j < 16; ++j) {
        if (col_count[j] < 16) {
          pal_number = j;
          pal_idx = col_count[j];
          col_map[pal_number][pal_idx] = objc[i];
          ++col_count[j];
          break;
        }
      }

    }

    // If we STILL do not have a valid slot, we are officially
    // in trouble, return an error

    if (pal_number < 0) {
      printf("%s: Could not fit all %u colors into the palette\n", 
             name, num_cols);
      delete[] bitmap_orig;
      return 1;
    }

    // Ok now for the mapping

    int new_idx = (pal_number * 16) + pal_idx;

    if (new_idx != i) {

      for (j=0; j < length; ++j) {
        if (bitmap_orig[j] == i) {
          bitmap[j] = new_idx;
        }
      }

    }

  }

  printf("%s: Sucessfully merged %u color palette into main 256 color palette\n",
         name, num_cols);

  delete[] bitmap_orig;

  return 0;

}

//************************************************************
// When this function is called, the bitmaps have been
// read in, processed, merged, and are ready for
// wirting to cpp and hpp files

int GenTiles::createOutput(const char* output_file, 
                           int obj_offset) {

  char fstr[512];

  printf("\n--- Writing Output ---\n\n");

  printf("%s.hpp: Opening File For Writing... ", output_file);

  // (Try to) Open the cpp and hpp files for writing

  
  if (lang == LANG_CPP) { sprintf(fstr, "%s.hpp", output_file); }
  else { sprintf(fstr, "%s.h", output_file); }
  FILE* hf = fopen(fstr, "w");

  if (!hf) {
    printf("FAILED\n");
    return 1;
  }
  else {
    printf("OK\n");
  }

  printf("%s.cpp: Opening File For Writing... ", output_file);

  if (lang == LANG_CPP) { sprintf(fstr, "%s.cpp", output_file); }
  else { sprintf(fstr, "%s.c", output_file); }
  FILE* cf = fopen(fstr, "w");

  if (!cf) {
    printf("FAILED\n");
    return 1;
  }
  else {
    printf("OK\n");
  }
  

  printf("%s: Writing Data... ", output_file);

  // the cpp and hpp files share the same header so we call
  // outputHeader for each file

  if (outputHeader(hf, obj_offset)) { return 1; }
  if (outputHeader(cf, obj_offset)) { return 1; }

  // outputClass fills in the rest of the cpp and hpp data

  if (outputClass(hf, cf, output_file, obj_offset)) { return 1; }

  // All done, flush and close the files

  fflush(hf);
  fclose(hf);

  fflush(cf);
  fclose(cf);

  printf("OK\n");

  return 0;

}

//************************************************************
// This function writes out a common header for the cpp and
// hpp files

int GenTiles::outputHeader(FILE* f, int obj_offset) {

  fprintf(f, "%s************************************************************\n", begCom());
  fprintf(f, "%s\n", inCom()); 
  fprintf(f, "%s %s : Tile and Palette Data For GBA Development\n", inCom(), classname);
  fprintf(f, "%s\n", inCom());
  fprintf(f, "%s   This file was autogenerated by the program 'bmp2gbtile',\n", inCom());
  fprintf(f, "%s   written by Matthew Wachowski (mwachowski@hotmail.com)\n", inCom());
  fprintf(f, "%s   The bmp2gbtile program is free for non commercial use.\n", inCom());
  fprintf(f, "%s   Contact Matthew Wachowski to use bmp2gbtile for\n", inCom());
  fprintf(f, "%s   commercial purposes.\n", inCom());
  fprintf(f, "%s\n", inCom());
  fprintf(f, "%s This file contains data based off the following bitmaps:\n", inCom());
  fprintf(f, "%s\n", inCom());

  int tile_offset = obj_offset;

  int i = 0;
  for (; i < bmp_count; ++i) {

    BMP_Header* bmp = (BMP_Header*)bitmaps[i];

    fprintf(f, "%s   %2u (%4u): %-20s (%ux%u, %u bit, %u cols, pal %u)\n",
            inCom(),
            i,
            tile_offset,
            bitmap_names[i],
            bmp->info_hdr.width,
            bmp->info_hdr.height,
            bmp->info_hdr.bit_count, 
            bmp->info_hdr.cols_used, bmp->file_hdr.palette_idx);

    tile_offset += (bmp->info_hdr.width >> 3) * (bmp->info_hdr.height >> 3) * 
      (bmp->info_hdr.bit_count >> 2);

  }

  fprintf(f, "%s\n", inCom());
  fprintf(f, "%s The first object index in this file is: %u\n", 
          inCom(), obj_offset);
  fprintf(f, "%s\n", inCom());
  fprintf(f, "%s************************************************************%s\n\n", 
          inCom(), endCom());

  return 0;

}

//************************************************************
// This function outputs class data to the given
// cpp and hpp file streams

int GenTiles::outputClass(FILE* hf, FILE* cf, 
                          const char* output_file, int obj_offset) {

  char line_buff[1024];

  // determine the palette count

  int palette_count = 0;
  while ((palette_count < 16) && col_count[palette_count]) { ++palette_count; }

  // determine the tile data array size

  int tile_byte_count = 0;
  int i = 0;
  for (; i < bmp_count; ++i) {
    
    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
    tile_byte_count += 
      (bmp->info_hdr.width * bmp->info_hdr.height) * bmp->info_hdr.bit_count / 8; 

  }

  //
  // first the hpp file
  //

  if (lang == LANG_CPP) { fprintf(hf, "struct %s {\n\n", classname); }
  
  writeStructCom(hf, "This value is the number of bitmaps defined in this");
  writeStructCom(hf, "structure, and gives the offset of the first definition");

  fprintf(hf, "\n");

  writeStructConst(hf, "unsigned short", "bmp_count");
  writeStructConst(hf, "unsigned short", "tile_offset");

  // Output sprite attributes, if enabled

  if (no_attr == false) {

    fprintf(hf, "\n");

    writeStructCom(hf, "These values can be used to set up OBJ structures");
    writeStructCom(hf, "with tile data referred to in this file");
    writeStructCom(hf, "(this assumes that the tile data is loaded into");
    writeStructCom(hf, "OBJ Ram on the GBA at the correct offset)");

    fprintf(hf, "\n");
    
    writeStructConst(hf, "unsigned short", "attr0_and_mask");
    writeStructConst(hf, "unsigned short", "attr1_and_mask");
    writeStructConst(hf, "unsigned short", "attr2_and_mask");
    sprintf(line_buff, "attr0_or_mask[%u]", bmp_count);
    writeStructConst(hf, "unsigned short", line_buff);
    sprintf(line_buff, "attr1_or_mask[%u]", bmp_count);
    writeStructConst(hf, "unsigned short", line_buff);
    sprintf(line_buff, "attr2_or_mask[%u]", bmp_count);
    writeStructConst(hf, "unsigned short", line_buff);

  }

  // Output palette declarations, if enabled

  if (no_palette == false) {

    fprintf(hf, "\n");

    writeStructCom(hf, "This value gives color palette definitions");
    writeStructCom(hf, "for all defined palettes");

    fprintf(hf, "\n");

    writeStructConst(hf, "unsigned short", "palette_count");
    sprintf(line_buff, "palette_data[0x%02X]", palette_count * 0x20);
    writeStructConst(hf, "unsigned short", line_buff);

  }

  // output tile data declarations

  fprintf(hf, "\n");

  writeStructCom(hf, "This value gives the size of the tile data");
  writeStructCom(hf, "structure and values for the tile entrys");

  fprintf(hf, "\n");

  writeStructConst(hf, "unsigned short", "tile_byte_count");
  sprintf(line_buff, "tile_data[0x%04X]", tile_byte_count);
  writeStructConst(hf, "unsigned char", line_buff);

  if (lang == LANG_CPP) { fprintf(hf, "};\n"); }

  writeSep(hf);

  //
  //now for the cpp file
  //

  // We only need to include the header in C++

  if (lang == LANG_CPP) { 
    fprintf(cf, "#include \"%s.hpp\"\n", output_file);
    writeSep(cf);
  }

  // Write out values

  writeConstVal(cf, "unsigned short", "bmp_count", "%u", bmp_count);
  writeConstVal(cf, "unsigned short", "tile_offset", "%u", obj_offset);

  if (no_attr == false) {

    // Write out sprite attribute data to the C/C++ file

    writeSep(cf);
    
    writeConstVal(cf, "unsigned short", "attr0_and_mask", "0x%04X", 0x1FFF);
    writeConstVal(cf, "unsigned short", "attr1_and_mask", "0x%04X", 0x3FFF);
    writeConstVal(cf, "unsigned short", "attr2_and_mask", "0x%04X", 0x0C00);

    writeSep(cf);

    writeConstArray(cf, "unsigned short", "attr0_or_mask", bmp_count);
    if (outputAttr0(cf)) { return 1; }
    fprintf(cf, "};\n");

    writeSep(cf);

    writeConstArray(cf, "unsigned short", "attr1_or_mask", bmp_count);
    if (outputAttr1(cf)) { return 1; }
    fprintf(cf, "};\n");

    writeSep(cf);

    writeConstArray(cf, "unsigned short", "attr2_or_mask", bmp_count);
    if (outputAttr2(cf, obj_offset)) { return 1; }
    fprintf(cf, "};\n");

  }

  if (no_palette == false) {

    // Write out palette data

    writeSep(cf);

    writeConstVal(cf, "unsigned short", "palette_count", "%u", palette_count);
    writeConstArray(cf, "unsigned short", "palette_data", palette_count * 0x20);
    if (outputPalette(cf, palette_count)) { return 1; }
    fprintf(cf, "};\n");

  }

  writeSep(cf);

  writeConstVal(cf, "unsigned short", "tile_byte_count", "%u", tile_byte_count);
  writeConstArray(cf, "unsigned char", "tile_data", tile_byte_count);
  if (outputTiles(cf)) { return 1; }
  fprintf(cf, "};\n\n");

  fprintf(cf, "%s************************************************************\n", 
          begCom());
  fprintf(cf, "%s END OF FILE\n", inCom());
  fprintf(cf, "%s************************************************************%s\n", 
          inCom(), endCom());

  return 0;
  
}

//************************************************************
//
// GenTiles::outputAttr0
//
// The purpose of this function is to create the OR
// masks in the output file
//
// Note: attr0 is part of the OBJ structure defined in GBA
//       hardware.  This structure is used to define
//       sprite properties

int GenTiles::outputAttr0(FILE* f) {

  int i = 0;
  for (; i < bmp_count; ++i) {

    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
    unsigned short data = 0x0000;

    // First, OR in the shape parameter

    switch ((bmp->info_hdr.width << 8) | (bmp->info_hdr.height)) {

    case 0x0808:
    case 0x1010:
    case 0x2020:
    case 0x4040:
      break;

    case 0x1008:
    case 0x2008:
    case 0x2010:
    case 0x4020:
      data |= 0x4000;
      break;

    case 0x0810:
    case 0x0820:
    case 0x1020:
    case 0x2040:
      data |= 0x8000;
      break;

    default:
      printf("ERROR: %s : no good dimensions for SHAPE parm: %ux%u\n",
             bitmap_names[i], bmp->info_hdr.width, bmp->info_hdr.height);
      return 1;
    }
    
    // Now, OR in the color bit (4 / 8 bit color)

    switch (bmp->info_hdr.bit_count) {
    case 4:
      break;
    case 8:
      data |= 0x2000;
      break;
    default:
      printf("ERROR: bit width parm of bitmap %s not 4 or 8: %u\n",
             bitmap_names[i], bmp->info_hdr.bit_count);
      return 1;
    }

    // Write out the data



    if (i < (bmp_count - 1)) {
      fprintf(f, "  0x%04X,\n", data);
    }
    else {
      fprintf(f, "  0x%04X\n", data);
    }

  }

  return 0;

}

//************************************************************
//
// GenTiles::outputAttr1
//
// The purpose of this function is to create the OR
// masks in the output file
//
// Note: attr1 is part of the OBJ structure defined in GBA
//       hardware.  This structure is used to define
//       sprite properties

int GenTiles::outputAttr1(FILE* f) {

  int i = 0;
  for (; i < bmp_count; ++i) {

    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
    unsigned short data = 0x0000;

    // OR in the size parameter

    switch ((bmp->info_hdr.width << 8) | (bmp->info_hdr.height)) {

    case 0x0808:
    case 0x0810:
    case 0x1008:
      break;

    case 0x1010:
    case 0x0820:
    case 0x2008:
      data |= 0x4000;
      break;
 
    case 0x2020:
    case 0x1020:
    case 0x2010:
      data |= 0x8000;
      break;

    case 0x4040:
    case 0x2040:
    case 0x4020:
      data |= 0xC000;
      break;

    default:
      printf("ERROR: %s : no good dimensions for SIZE parm: %ux%u\n",
             bitmap_names[i], bmp->info_hdr.width, bmp->info_hdr.height);
      return 1;
    }
    
    // Write out the data

    if (i < (bmp_count - 1)) {
      fprintf(f, "  0x%04X,\n", data);
    }
    else {
      fprintf(f, "  0x%04X\n", data);
    }

  }

  return 0;

}

//************************************************************
//
// GenTiles::outputAttr2
//
// The purpose of this function is to create the OR
// masks in the output file
//
// Note: attr2 is part of the OBJ structure defined in GBA
//       hardware.  This structure is used to define
//       sprite properties

int GenTiles::outputAttr2(FILE* f, int obj_offset) {

  int tile_offset = obj_offset;

  int i = 0;
  for (; i < bmp_count; ++i) {

    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
    unsigned short data = 0x0000;

    // First, OR in the character name (idx)

    data |= tile_offset;

    // update obj_offset accordng to the number of tiles
    // we will be needing for this bitmap

    tile_offset += 
      (bmp->info_hdr.width >> 3) * (bmp->info_hdr.height >> 3) * 
      (bmp->info_hdr.bit_count >> 2);

    if (tile_offset > 1023) {

      //oh no, we used up all of our tile memory!

      printf("ERROR: Used up all available tile memory (32768 - %u offset bytes) at bitmap %s.\n",
             obj_offset * 0x20, bitmap_names[i]);
      printf("Ran out of memory processing %u of %u bitmaps\n", i, bmp_count);
      return 1;

    }
    
    // Fill in the palette number

    data |= bmp->file_hdr.palette_idx << 12;

    // Write out the data

    if (i < (bmp_count - 1)) {
      fprintf(f, "  0x%04X,\n", data);
    }
    else {
      fprintf(f, "  0x%04X\n", data);
    }

  }

  return 0;

}

//************************************************************
// This function writes out a palette definition that is
// directly compatible with GBA hardware (i.e. this
// structure can be memory copied into GBA hardware)

int GenTiles::outputPalette(FILE* f, int palette_count) {

  int i=0;
  for (; i < palette_count; ++i) {
    
    int j=0;
    for (; j < 16; ++j) {
      if ((j < 15) || (i < palette_count - 1)) {
        fprintf(f, "  0x%04X,\n", col_map[i][j]);
      }
      else {
        fprintf(f, "  0x%04X\n", col_map[i][j]);
      }
    }

    if (i < (palette_count - 1)) {
      fprintf(f, "\n");
    }

  }

  return 0;

}

//************************************************************
// This function outputs tile data that is compatible with
// GBA hardware.  Only '1D' style tile data is supported

int GenTiles::outputTiles(FILE* f) {

  int i = 0;
  for (; i < bmp_count; ++i) {
    
    BMP_Header* bmp = (BMP_Header*)bitmaps[i];
    unsigned char* data = bitmaps[i] + bmp->file_hdr.data_offset;

    bool put_comma = (i < (bmp_count - 1));

    // Here we simply decide if this is a 16 color
    // or 256 color tile, then call the appropriate
    // output function

    switch (bmp->info_hdr.bit_count) {
    case 4:
      if (outputTiles16(f, bmp, data, put_comma)) { return 1; }
      break;
    case 8:
      if (outputTiles256(f, bmp, data, put_comma)) { return 1; }
      break;
    default:
      printf("Error: can only output tiles of width 4, 8: %u given\n",
             bmp->info_hdr.bit_count);
      return 1;
    }
 
  }

  return 0;

}

//************************************************************
// This function is a helper function to outputTiles

int GenTiles::outputTiles16(FILE* f, BMP_Header* bmp, 
                            unsigned char* data, bool put_comma) {

  //
  // Tiles in the GBA 1D mode lay out like this:
  //
  // 123
  // 456
  // 789
  // 
  // Where each 16 color tile is a 8x8 pixel region that is 0x20
  // bytes in length
  //

  int width = bmp->info_hdr.width;
  int height = bmp->info_hdr.height;

  int byte_count = 0;
  int total_bytes = width * height / 2;

  // At this level, we are looking at tile ROWS

  int y = height - 8;
  for (; y >= 0; y -= 8) {

    // At this level, we are looking at tile COLUMNS

    int x=0;
    for (; x < width; x += 8) {

      // In here were are working at tile pixel rows

      int j = 7;
      for (; j >= 0; --j) {
        
        // Here we are looking at tile pixel columns
        
        fprintf(f, "  ");
        int i = 0;
        for (; i < 4; ++i) {

          unsigned char pixel_value = 
            data[((width / 2) * (y + j)) + (x / 2) + i];

          // For some reason, the GBA wants these pixels nibble swapped

          pixel_value = ((pixel_value & 0x0F) << 4) | (pixel_value >> 4);

          if ((byte_count < (total_bytes - 1)) || put_comma) {
            fprintf(f, "0x%02X, ", pixel_value);
          }
          else {
            fprintf(f, "0x%02X", pixel_value);
          }

          ++byte_count;

        }

        fprintf(f, "\n");

      }

      fprintf(f, "\n");

    }

  }

  return 0;

}

//************************************************************
// This function is a helper function to outputTiles

int GenTiles::outputTiles256(FILE* f, BMP_Header* bmp, 
                             unsigned char* data, bool put_comma) {

  //
  // Tiles in the GBA 1D mode lay out like this:
  //
  // 123
  // 456
  // 789
  // 
  // Where each 256 color tile is a 4x8 pixel region that is 0x20
  // bytes in length
  //

  int width = bmp->info_hdr.width;
  int height = bmp->info_hdr.height;

  int byte_count = 0;
  int total_bytes = width * height;

  // At this level, we are looking at tile ROWS

  int y = height - 8;
  for (; y >= 0; y -= 8) {

    // At this level, we are looking at tile COLUMNS

    int x=0;
    for (; x < width; x += 8) {

      // In here were are working at tile pixel rows

      int j = 7;
      for (; j >= 0; --j) {
        
        // Here we are looking at tile pixel columns
        
        fprintf(f, "  ");
        int i = 0;
        for (; i < 8; ++i) {

          unsigned char pixel_value = data[(width * (y + j)) + x + i];
          if ((byte_count < (total_bytes - 1)) || put_comma) {
            fprintf(f, "0x%02X, ", pixel_value);
          }
          else {
            fprintf(f, "0x%02X", pixel_value);
          }

          ++byte_count;

        }

        fprintf(f, "\n");

      }

      fprintf(f, "\n");

    }

  }

  return 0;

}

//************************************************************
// This function loads in a palette file that can be used to 
// constrain palette data in the resulting output
// this is useful for fixing palette entries

int GenTiles::loadPaletteFile(const char* palette_filename) {

  if (!palette_filename) { return 0; }

  int err = 0;

  // Try to open the palette file

  FILE* f = fopen(palette_filename, "r");

  if (!f) {
    fprintf(stderr, "could not find palette file: %s\n", 
            palette_filename);
    return 1;
  }

  //
  // parse the file
  //

  // allocate a buffer for getline

  size_t buff_size;
  buff_size = 1024;
  char* buff = (char*)malloc(buff_size);
  int pal = 0;

  while (!err && (buff = fgets(buff, buff_size, f))) {
    
    char* tbuff = buff;

    // skip white space

    while ((*tbuff) && ((*tbuff == ' ') || (*tbuff == '\t'))) ++tbuff;

    // check for a comment or empty line

    if (!(*tbuff) || (*tbuff == '#') || (*tbuff == '\n')) {
      continue;
    }

    // check for a 'pal'

    if (!strncmp(tbuff, "pal ", 4)) {
      pal = atoi(tbuff + 4);
      if ((pal < 0) || (pal > 15)) {
        fprintf(stderr, "loadPalette: invalid palette value: %s while parsing %s\n",
                tbuff + 4, buff);
        err = 1;
        break;
      }
    }
    else {

      // try to parse the data into hex

      unsigned short val = 0;
      int bit_shift = 12;
      for (; bit_shift >= 0; bit_shift -= 4) {
        if ((*tbuff >= '0') && (*tbuff <= '9')) {
          val |= (*tbuff - '0') << bit_shift;
        }
        else if ((*tbuff >= 'A') && (*tbuff <= 'F')) {
          val |= (*tbuff - 'A' + 10) << bit_shift;
        }
        else if ((*tbuff >= 'a') && (*tbuff <= 'f')) {
          val |= (*tbuff - 'a' + 10) << bit_shift;
        }
        else {
          fprintf(stderr, 
                  "loadPalette: bad parse while trying to extract hex data: %s\n",
                  buff);
          err = 1;
          break;
        }
        ++tbuff;
      }

      // put the color value in our palette

      col_map[pal][col_count[pal]] = val;
      ++col_count[pal];
      if (col_count[pal] == 16) {
        ++pal;
      }

    }

  }

  // all done

  free(buff);

  return err;

}

//************************************************************
// This function is called by source generation
// to start a comment

const char* GenTiles::begCom() {
  if (lang == LANG_C) {
    return "/*";
  }
  return "//";
}

//************************************************************
// This function is called by source generation while
// inside a comment block

const char* GenTiles::inCom() {
  if (lang == LANG_C) {
    return " *";
  }
  return "//";
}

//************************************************************
// This function is called by source generation at the end
// of a comment

const char* GenTiles::endCom() {
  if (lang == LANG_C) {
    return "*/";
  }
  return "";
}

//************************************************************
// This function is called by source generation for a normal
// one-line comment

void GenTiles::writeStructCom(FILE* f, const char* str) {
  if (lang == LANG_C) {
    fprintf(f, "/* %-58s */\n", str);
  }
  else {
    fprintf(f, "  // %s\n", str);
  }
}

//************************************************************
// This function is called by source generation to create
// a seperator

void GenTiles::writeSep(FILE* f) {
  if (lang == LANG_C) {
    fprintf(f, "\n/************************************************************/\n\n");
  }
  else {
    fprintf(f, "\n//************************************************************\n\n");
  }
}

//************************************************************
// This function is called by source generation to output
// a single value in the header file

void GenTiles::writeStructConst(FILE* f, const char* type, const char* name) {
  if (lang == LANG_C) {
    fprintf(f, "extern const %s %s_%s;\n", type, classname, name);
  }
  else {
    fprintf(f, "  const static %s %s;\n", type, name);
  }
}

//************************************************************
// This function is called by source generation to output
// a single value into the C/C++ file

void GenTiles::writeConstVal(FILE* f, const char* type, const char* name, 
                             const char* format, unsigned int val) {
  char line_buff[512];
  if (lang == LANG_C) {
    sprintf(line_buff, "const %s %s_%-19s = %s;\n", type, 
            classname, name, format);
  }
  else {
    sprintf(line_buff, "const %s %s::%-19s = %s;\n", type, 
            classname, name, format);
  }
  fprintf(f, line_buff, val);
}

//************************************************************
// This function is called by source generation to start
// an array of constant data

void GenTiles::writeConstArray(FILE* f, const char* type, const char* name, 
                               unsigned int size) {
  if (lang == LANG_C) {
    fprintf(f, "const %s %s_%s[%u] = {\n", type, classname, name, size);
  }
  else {
    fprintf(f, "const %s %s::%s[%u] = {\n", type, classname, name, size);
  }
}

//************************************************************
