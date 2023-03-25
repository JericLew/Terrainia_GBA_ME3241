
BMP2Tile version 1.0
Noble Engineering
--------------------

Purpose: Creates palette and tile data from a Windows bitmap
Input: 256-color Windows BMP file
Output: C file that defines a "tiles_Palette" array and a "tiles_Data" array
Notes:

- Supports 256-color Windows BMP files only
- Supports uncompressed bitmaps only
- Bitmap can be any size
- Tiles should be 8 pixels wide by 8 pixels high
- No spacing allowed in between tiles
- Tiles will be read from left-to-right and top-to-bottom