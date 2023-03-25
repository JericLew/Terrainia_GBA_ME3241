//************************************************************
//
// bmp2gbtiles
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

//************************************************************
// Structure(s) that allow .bmp files to be interpreted

#pragma pack(1)

struct BMP_FileHeader {
  unsigned short type;
  unsigned int   size;
  unsigned short palette_idx;
  unsigned short reserved2;
  unsigned int   data_offset;
};

struct BMP_InfoHeader {
  unsigned int   size;
  unsigned int   width;
  unsigned int   height;
  unsigned short planes;
  unsigned short bit_count;
  unsigned int   compression;
  unsigned int   image_size;
  unsigned int   xpix_per_meter;
  unsigned int   ypix_per_meter;
  unsigned int   cols_used;
  unsigned int   cols_important;
};

struct BMP_Header {
  BMP_FileHeader file_hdr;
  BMP_InfoHeader info_hdr;
};

#pragma pack()

//************************************************************
// Structure that represents C or C++ as the generation
// format

enum lang_enum {
  LANG_C,
  LANG_CPP
};

//************************************************************
// A simple class to organize and hold data

#include <cstdio>

class GenTiles {
public:

  void init(int input_file_count, bool _tile_mode, 
            bool _no_attr, bool _no_palette, int _lang, 
            const char* _classname);
  int generate(const char* output_filename, const char** input_files,
               int input_file_count, int obj_offset, const char* palette_filename);
  void printHelp();

private:

  void cleanup();

  // Functions for loading and interpreting .bmp files
  
  int loadAllFiles(const char** input_files, int input_file_count);
  int loadFile(const char* filename);
  int checkBMP(BMP_Header* bmp, const char* filename, int filesize);
  int loadPaletteFile(const char* palette_filename);

  // Functions for reorganizing and reducing colors

  int reduceColors(BMP_Header* bmp, const char* filename);
  int deleteDuplicateColor(BMP_Header* bmp, int high_idx, int low_idx);
  int buildPalette();
  int build16(BMP_Header* bmp, const char* name);
  void mergePalette16(unsigned short* pal, unsigned char* pal_len, 
                      BMP_Header* bmp);
  int build256(BMP_Header* bmp, const char* name);
  int getColorMapDelta(unsigned short* pal, int pal_len,
                       unsigned int* objc, int objc_len);

  // Top-Level Functions for Code Generation 

  int createOutput(const char* output_file, int obj_offset);
  int outputHeader(FILE* f, int obj_offset);
  int outputClass(FILE* hf, FILE* cf, const char* output_file, int obj_offset);
  int outputAttr0(FILE* f);
  int outputAttr1(FILE* f);
  int outputAttr2(FILE* f, int obj_offset);
  int outputPalette(FILE* f, int palette_count);
  int outputTiles(FILE* f);
  int outputTiles16(FILE* f, BMP_Header* bmp, unsigned char* data, bool put_comma);
  int outputTiles256(FILE* f, BMP_Header* bmp, unsigned char* data, bool put_comma);

  // C/C++ Language difference functions

  const char* begCom();
  const char* inCom();
  const char* endCom();
  void writeStructCom(FILE* f, const char* str);
  void writeSep(FILE* f);
  void writeStructConst(FILE* f, const char* type, const char* name);
  void writeConstVal(FILE* f, const char* type, const char* name, 
                     const char* format, unsigned int val);
  void writeConstArray(FILE* f, const char* type, const char* name, 
                       unsigned int size);


  // Class Variables

  bool tile_mode;
  bool no_attr;
  bool no_palette;
  int lang;

  unsigned char** bitmaps;
  char**          bitmap_names;
  int             bmp_count;
  char*           classname;

  unsigned short  col_map[16][16];
  unsigned char   col_count[16];

};

//************************************************************
// EOF
