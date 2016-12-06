/*
===============================================================================

  FILE:  lasreader_bil.cpp
  
  CONTENTS:
  
    see corresponding header file
  
  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com

  COPYRIGHT:

    (c) 2007-2012, martin isenburg, rapidlasso - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the LICENSE.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/
#include "lasreader_bil.hpp"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

BOOL LASreaderBIL::open(const CHAR* file_name)
{
  if (file_name == 0)
  {
    throw std::runtime_error(std::string("ERROR: fine name pointer is zero"));
    return FALSE;
  }

  // clean header

  clean();

  // read the mandatory hdr file

  if (!read_hdr_file(file_name))
  {
    throw std::runtime_error(std::string("ERROR: reading the *.hdr file for '"));
    return FALSE;
  }

  // read the optional bwl file

  if (!read_blw_file(file_name))
  {
    throw std::runtime_error(std::string("WARNING: reading the *.blw file for '"));
  }

  // check that we have all the needed info

  if (xdim <= 0)
  {
    xdim = 1;
    throw std::runtime_error(std::string("WARNING: xdim was not set. setting to "));
  }

  if (ydim <= 0)
  {
    ydim = 1;
    throw std::runtime_error(std::string("WARNING: ydim was not set. setting to "));
  }

  if (ulxcenter == F64_MAX)
  {
    ulxcenter = 0.5*xdim;
    throw std::runtime_error(std::string("WARNING: ulxcenter was not set. setting to "));
  }

  if (ulycenter == F64_MAX)
  {
    ulycenter = (-0.5+nrows)*ydim;
    throw std::runtime_error(std::string("WARNING: ulycenter was not set. setting to "));
  }

  // open the BIL file

  file = fopen(file_name, "rb");
  if (file == 0)
  {
    throw std::runtime_error(std::string("ERROR: cannot open file '"));
    return FALSE;
  }

  if (setvbuf(file, NULL, _IOFBF, 2*LAS_TOOLS_IO_IBUFFER_SIZE) != 0)
  {
    throw std::runtime_error(std::string("WARNING: setvbuf() failed with buffer size "));
  }

  // populate the header as much as it makes sense

  sprintf(header.system_identifier, "LAStools (c) by rapidlasso GmbH");
  sprintf(header.generating_software, "via LASreaderBIL (%d)", LAS_TOOLS_VERSION);

  // maybe set creation date

#ifdef _WIN32
  WIN32_FILE_ATTRIBUTE_DATA attr;
	SYSTEMTIME creation;
  GetFileAttributesEx(file_name, GetFileExInfoStandard, &attr);
	FileTimeToSystemTime(&attr.ftCreationTime, &creation);
  int startday[13] = {-1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  header.file_creation_day = startday[creation.wMonth] + creation.wDay;
  header.file_creation_year = creation.wYear;
  // leap year handling
  if ((((creation.wYear)%4) == 0) && (creation.wMonth > 2)) header.file_creation_day++;
#else
  header.file_creation_day = 333;
  header.file_creation_year = 2012;
#endif

  // initialize point format in header

  header.point_data_format = 0;
  header.point_data_record_length = 20;

  // initialize point

  point.init(&header, header.point_data_format, header.point_data_record_length, &header);

  // init the bounding box x y

  header.min_x = ulxcenter;
  header.min_y = ulycenter - (nrows-1)*ydim;
  header.max_x = ulxcenter + (ncols-1)*xdim;
  header.max_y = ulycenter;
  header.min_z = F64_MAX;
  header.max_z = F64_MIN;

  // init the bounding box z and count the rasters

  F32 elevation = 0;
  npoints = 0;

  if (nbits == 32)
  {
    if (floatpixels)
    {
      for (col = 0; col < ncols; col++)
      {
        for (row = 0; row < nrows; row++)
        {
          if (fread((void*)&elevation, 4, 1, file) == 1)
          {
            if (elevation != nodata)
            {
              if (header.max_z < elevation) header.max_z = elevation;
              if (header.min_z > elevation) header.min_z = elevation;
              npoints++;
            }
          }
          else
          {
            col = ncols;
            row = nrows;
          }
        }
      }
    }
    else
    {
      I32 elev;
      for (col = 0; col < ncols; col++)
      {
        for (row = 0; row < nrows; row++)
        {
          if (fread((void*)&elev, 4, 1, file) == 1)
          {
            elevation = (F32)elev;
            if (elevation != nodata)
            {
              if (header.max_z < elevation) header.max_z = elevation;
              if (header.min_z > elevation) header.min_z = elevation;
              npoints++;
            }
          }
          else
          {
            col = ncols;
            row = nrows;
          }
        }
      }
    }
  }
  else if (nbits == 16)
  {
    I16 elev;
    for (col = 0; col < ncols; col++)
    {
      for (row = 0; row < nrows; row++)
      {
        if (fread((void*)&elev, 2, 1, file) == 1)
        {
          elevation = (F32)elev;
          if (elevation != nodata)
          {
            if (header.max_z < elevation) header.max_z = elevation;
            if (header.min_z > elevation) header.min_z = elevation;
            npoints++;
          }
        }
        else
        {
          col = ncols;
          row = nrows;
        }
      }
    }
  }
  else
  {
    I8 rgb[3];
    for (col = 0; col < ncols; col++)
    {
      for (row = 0; row < nrows; row++)
      {
        if (fread((void*)&rgb, 1, nbands, file) == (U32)nbands)
        {
          elevation = (F32)(rgb[0]);
          if (elevation != nodata)
          {
            if (header.max_z < elevation) header.max_z = elevation;
            if (header.min_z > elevation) header.min_z = elevation;
            npoints++;
          }
        }
        else
        {
          col = ncols;
          row = nrows;
        }
      }
    }
  }

  // close the BIL file

  close();

  // check the header values

  header.number_of_point_records = (U32)npoints;

  if (npoints)
  {
    // populate scale and offset

    populate_scale_and_offset();

    // check bounding box for this scale and offset

    populate_bounding_box();
  }
  else
  {
    throw std::runtime_error(std::string("WARNING: BIL raster contains only no data values"));
    header.min_z = 0;
    header.max_z = 0;
  }

  // reopen

  return reopen(file_name);
}

BOOL LASreaderBIL::read_hdr_file(const CHAR* file_name)
{
  if (file_name == 0)
  {
    throw std::runtime_error(std::string("ERROR: fine name pointer is zero"));
    return FALSE;
  }

  // create *.hdr file name

  I32 len = strlen(file_name) - 3;
  CHAR* file_name_hdr = strdup(file_name);

  while ((len > 0) && (file_name_hdr[len] != '.')) len--;

  if ((len == 0) && (file_name_hdr[len] != '.'))
  {
    throw std::runtime_error(std::string("ERROR: file name '"));
    return FALSE;
  }

  file_name_hdr[len+1] = 'h';
  file_name_hdr[len+2] = 'd';
  file_name_hdr[len+3] = 'r';

  FILE* file = fopen(file_name_hdr, "r");

  if (file == 0)
  {
    file_name_hdr[len+1] = 'H';
    file_name_hdr[len+2] = 'D';
    file_name_hdr[len+3] = 'R';

    file = fopen(file_name_hdr, "r");
    free(file_name_hdr);

    if (file == 0)
    {
      file_name_hdr[len] = '\0';
      throw std::runtime_error(std::string("ERROR: cannot open files '"));
      return FALSE;
    }
  }
  else
  {
    free(file_name_hdr);
  }

  CHAR line[512];
  CHAR dummy[32];
  col = 0;
  row = 0;
  ncols = 0;
  nrows = 0;
  nbands = 0;
  nbits = 0;
  F64 ulxmap = F64_MAX;
  F64 ulymap = F64_MAX;
  xdim = 0;
  ydim = 0;
  nodata = -9999;
  floatpixels = FALSE;

  while (fgets(line, 256, file))
  {
    if (line[0] == '#')
    {
      continue;
    }
    else if (strstr(line, "ncols") || strstr(line, "NCOLS"))
    {
      sscanf(line, "%s %d", dummy, &ncols);
    }
    else if (strstr(line, "nrows") || strstr(line, "NROWS"))
    {
      sscanf(line, "%s %d", dummy, &nrows);
    }
    else if (strstr(line, "nbands") || strstr(line, "NBANDS"))
    {
      sscanf(line, "%s %d", dummy, &nbands);
    }
    else if (strstr(line, "nbits") || strstr(line, "NBITS"))
    {
      sscanf(line, "%s %d", dummy, &nbits);
    }
    else if (strstr(line, "layout") || strstr(line, "LAYOUT"))
    {
      CHAR layout[32];
      if (sscanf(line, "%s %s", dummy, layout) == 2)
      {
        if (strcmp(layout, "bil") && strcmp(layout, "BIL"))
        {
          throw std::runtime_error(std::string("WARNING: "));
        }
      }
      else
      {
        throw std::runtime_error(std::string("WARNING: argument of "));
      }
    }
    else if (strstr(line, "pixeltype") || strstr(line, "PIXELTYPE"))
    {
      CHAR pixeltype[32];
      sscanf(line, "%s %s", dummy, pixeltype);
      if (strcmp(pixeltype, "float") && strcmp(pixeltype, "FLOAT"))
      {
        throw std::runtime_error(std::string("WARNING: pixeltype '"));
      }
      else
      {
        floatpixels = TRUE;
      }
    }
    else if (strstr(line, "nodata") || strstr(line, "NODATA"))
    {
      sscanf(line, "%s %f", dummy, &nodata);
    }
    else if (strstr(line, "byteorder") || strstr(line, "BYTEORDER")) // if little or big endian machine (i == intel, m == motorola)
    {
      CHAR byteorder[32];
      sscanf(line, "%s %s", dummy, byteorder);
      if (strcmp(byteorder, "i") && strcmp(byteorder, "I"))
      {
        throw std::runtime_error(std::string("WARNING: byteorder '"));
      }
    }
    else if (strstr(line, "ulxmap") || strstr(line, "ULXMAP"))
    {
      sscanf(line, "%s %lf", dummy, &ulxmap);
    }
    else if (strstr(line, "ulymap") || strstr(line, "ULYMAP"))
    {
      sscanf(line, "%s %lf", dummy, &ulymap);
    }
    else if (strstr(line, "xdim") || strstr(line, "XDIM"))
    {
      sscanf(line, "%s %f", dummy, &xdim);
    }
    else if (strstr(line, "ydim") || strstr(line, "YDIM"))
    {
      sscanf(line, "%s %f", dummy, &ydim);
    }
  }

  fclose(file);

  if (ulxmap < F64_MAX)
  {
    ulxcenter = ulxmap + 0.5*xdim;
  }

  if (ulymap < F64_MAX)
  {
    ulycenter = ulymap - 0.5*ydim;
  }

  if ((ncols <= 0) || (nrows <= 0) || (nbands <= 0) || (nbits <= 0))
  {
    throw std::runtime_error(std::string("WARNING: not able to find all entries in HDR file"));
    throw std::runtime_error(std::string("       ncols  = "));
    throw std::runtime_error(std::string("       nrows  = "));
    throw std::runtime_error(std::string("       nbands = "));
    throw std::runtime_error(std::string("       nbits  = "));
    return FALSE;
  }

  return TRUE;
}

BOOL LASreaderBIL::read_blw_file(const CHAR* file_name)
{
  if (file_name == 0)
  {
    throw std::runtime_error(std::string("ERROR: fine name pointer is zero"));
    return FALSE;
  }

  // create *.blw file name

  I32 len = strlen(file_name) - 3;
  CHAR* file_name_bwl = strdup(file_name);

  while ((len > 0) && (file_name_bwl[len] != '.')) len--;

  if ((len == 0) && (file_name_bwl[len] != '.'))
  {
    throw std::runtime_error(std::string("ERROR: file name '"));
    return FALSE;
  }

  file_name_bwl[len+1] = 'b';
  file_name_bwl[len+2] = 'l';
  file_name_bwl[len+3] = 'w';

  FILE* file = fopen(file_name_bwl, "r");

  if (file == 0)
  {
    file_name_bwl[len+1] = 'B';
    file_name_bwl[len+2] = 'L';
    file_name_bwl[len+3] = 'W';

    file = fopen(file_name_bwl, "r");

    if (file == 0)
    {
      file_name_bwl[len] = '\0';
      throw std::runtime_error(std::string("WARNING: cannot open files '"));
      free(file_name_bwl);
      return FALSE;
    }
  }

  free(file_name_bwl);

  CHAR line[512];

  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  sscanf(line, "%f", &xdim);
  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  sscanf(line, "%f", &ydim);
  ydim = -1*ydim;
  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  sscanf(line, "%lf", &ulxcenter);
  if (!fgets(line, 256, file))
  {
    throw std::runtime_error(std::string("WARNING: corrupt world file"));
    return FALSE;
  }
  sscanf(line, "%lf", &ulycenter);

  fclose(file);

  return TRUE;
}

void LASreaderBIL::set_scale_factor(const F64* scale_factor)
{
  if (scale_factor)
  {
    if (this->scale_factor == 0) this->scale_factor = new F64[3];
    this->scale_factor[0] = scale_factor[0];
    this->scale_factor[1] = scale_factor[1];
    this->scale_factor[2] = scale_factor[2];
  }
  else if (this->scale_factor)
  {
    delete [] this->scale_factor;
    this->scale_factor = 0;
  }
}

void LASreaderBIL::set_offset(const F64* offset)
{
  if (offset)
  {
    if (this->offset == 0) this->offset = new F64[3];
    this->offset[0] = offset[0];
    this->offset[1] = offset[1];
    this->offset[2] = offset[2];
  }
  else if (this->offset)
  {
    delete [] this->offset;
    this->offset = 0;
  }
}


BOOL LASreaderBIL::seek(const I64 p_index)
{
  return FALSE;
}

BOOL LASreaderBIL::read_point_default()
{
  F32 elevation;
  while (p_count < npoints)
  {
    if (col == ncols)
    {
      col = 0;
      row++;
    }

    if (nbits == 32)
    {
      if (floatpixels)
      {
        if (fread((void*)&elevation, 4, 1, file) != 1)
        {
#ifdef _WIN32
          throw std::runtime_error(std::string("WARNING: end-of-file after "));
#else
          throw std::runtime_error(std::string("WARNING: end-of-file after "));
#endif
          npoints = p_count;
          return FALSE;
        }
      }
      else
      {
        I32 elev;
        if (fread((void*)&elev, 4, 1, file) != 1)
        {
#ifdef _WIN32
          throw std::runtime_error(std::string("WARNING: end-of-file after "));
#else
          throw std::runtime_error(std::string("WARNING: end-of-file after "));
#endif
          npoints = p_count;
          return FALSE;
        }
        elevation = (F32)elev;
      }
    }
    else if (nbits == 16)
    {
      I16 elev;
      if (fread((void*)&elev, 2, 1, file) != 1)
      {
#ifdef _WIN32
        throw std::runtime_error(std::string("WARNING: end-of-file after "));
#else
        throw std::runtime_error(std::string("WARNING: end-of-file after "));
#endif
        npoints = p_count;
        return FALSE;
      }
      elevation = (F32)elev;
    }
    else
    {
      U8 rgb[3];
      if (fread((void*)rgb, 1, nbands, file) != (U32)nbands)
      {
#ifdef _WIN32
        throw std::runtime_error(std::string("WARNING: end-of-file after "));
#else
        throw std::runtime_error(std::string("WARNING: end-of-file after "));
#endif
        npoints = p_count;
        return FALSE;
      }
      elevation = rgb[0];
    }

    if (elevation != nodata)
    {
      point.set_x(ulxcenter + col * xdim);
      point.set_y(ulycenter - row * ydim);
      point.set_z(elevation);
      p_count++;
      col++;
      return TRUE;
    }
    else
    {
      col++;
    }
  }
  return FALSE;
}

ByteStreamIn* LASreaderBIL::get_stream() const
{
  return 0;
}

void LASreaderBIL::close(BOOL close_stream)
{
  if (file)
  {
    fclose(file);
    file = 0;
  }
}

BOOL LASreaderBIL::reopen(const CHAR* file_name)
{
  if (file_name == 0)
  {
    throw std::runtime_error(std::string("ERROR: fine name pointer is zero"));
    return FALSE;
  }

  file = fopen(file_name, "rb");
  if (file == 0)
  {
    throw std::runtime_error(std::string("ERROR: cannot reopen file '"));
    return FALSE;
  }

  if (setvbuf(file, NULL, _IOFBF, 2*LAS_TOOLS_IO_IBUFFER_SIZE) != 0)
  {
    throw std::runtime_error(std::string("WARNING: setvbuf() failed with buffer size "));
  }

  col = 0;
  row = 0;
  p_count = 0;

  return TRUE;
}

void LASreaderBIL::clean()
{
  if (file)
  {
    fclose(file);
    file = 0;
  }
  col = 0;
  row = 0;
  ncols = 0;
  nrows = 0;
  nbands = 0;
  nbits = 0;
  ulxcenter = F64_MAX;
  ulycenter = F64_MAX;
  xdim = 0;
  ydim = 0;
  nodata = -9999;
  floatpixels = FALSE;
}

LASreaderBIL::LASreaderBIL()
{
  file = 0;
  scale_factor = 0;
  offset = 0;
  clean();
}

LASreaderBIL::~LASreaderBIL()
{
  clean();
  if (scale_factor)
  {
    delete [] scale_factor;
    scale_factor = 0;
  }
  if (offset)
  {
    delete [] offset;
    offset = 0;
  }
}

void LASreaderBIL::populate_scale_and_offset()
{
  // if not specified in the command line, set a reasonable scale_factor
  if (scale_factor)
  {
    header.x_scale_factor = scale_factor[0];
    header.y_scale_factor = scale_factor[1];
    header.z_scale_factor = scale_factor[2];
  }
  else
  {
    if (-360 < header.min_x  && -360 < header.min_y && header.max_x < 360 && header.max_y < 360) // do we have longitude / latitude coordinates
    {
      header.x_scale_factor = 1e-7;
      header.y_scale_factor = 1e-7;
    }
    else // then we assume utm or mercator / lambertian projections
    {
      header.x_scale_factor = 0.01;
      header.y_scale_factor = 0.01;
    }
    header.z_scale_factor = 0.01;
  }

  // if not specified in the command line, set a reasonable offset
  if (offset)
  {
    header.x_offset = offset[0];
    header.y_offset = offset[1];
    header.z_offset = offset[2];
  }
  else
  {
    if (F64_IS_FINITE(header.min_x) && F64_IS_FINITE(header.max_x))
      header.x_offset = ((I64)((header.min_x + header.max_x)/header.x_scale_factor/20000000))*10000000*header.x_scale_factor;
    else
      header.x_offset = 0;

    if (F64_IS_FINITE(header.min_y) && F64_IS_FINITE(header.max_y))
      header.y_offset = ((I64)((header.min_y + header.max_y)/header.y_scale_factor/20000000))*10000000*header.y_scale_factor;
    else
      header.y_offset = 0;

    if (F64_IS_FINITE(header.min_z) && F64_IS_FINITE(header.max_z))
      header.z_offset = ((I64)((header.min_z + header.max_z)/header.z_scale_factor/20000000))*10000000*header.z_scale_factor;
    else
      header.z_offset = 0;
  }
}

void LASreaderBIL::populate_bounding_box()
{
  // compute quantized and then unquantized bounding box

  F64 dequant_min_x = header.get_x(header.get_X(header.min_x));
  F64 dequant_max_x = header.get_x(header.get_X(header.max_x));
  F64 dequant_min_y = header.get_y(header.get_Y(header.min_y));
  F64 dequant_max_y = header.get_y(header.get_Y(header.max_y));
  F64 dequant_min_z = header.get_z(header.get_Z(header.min_z));
  F64 dequant_max_z = header.get_z(header.get_Z(header.max_z));

  // make sure there is not sign flip

  if ((header.min_x > 0) != (dequant_min_x > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for min_x from "));
    throw std::runtime_error(std::string("         set scale factor for x coarser than "));
  }
  else
  {
    header.min_x = dequant_min_x;
  }
  if ((header.max_x > 0) != (dequant_max_x > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for max_x from "));
    throw std::runtime_error(std::string("         set scale factor for x coarser than "));
  }
  else
  {
    header.max_x = dequant_max_x;
  }
  if ((header.min_y > 0) != (dequant_min_y > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for min_y from "));
    throw std::runtime_error(std::string("         set scale factor for y coarser than "));
  }
  else
  {
    header.min_y = dequant_min_y;
  }
  if ((header.max_y > 0) != (dequant_max_y > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for max_y from "));
    throw std::runtime_error(std::string("         set scale factor for y coarser than "));
  }
  else
  {
    header.max_y = dequant_max_y;
  }
  if ((header.min_z > 0) != (dequant_min_z > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for min_z from "));
    throw std::runtime_error(std::string("         set scale factor for z coarser than "));
  }
  else
  {
    header.min_z = dequant_min_z;
  }
  if ((header.max_z > 0) != (dequant_max_z > 0))
  {
    throw std::runtime_error(std::string("WARNING: quantization sign flip for max_z from "));
    throw std::runtime_error(std::string("         set scale factor for z coarser than "));
  }
  else
  {
    header.max_z = dequant_max_z;
  }
}

LASreaderBILrescale::LASreaderBILrescale(F64 x_scale_factor, F64 y_scale_factor, F64 z_scale_factor) : LASreaderBIL()
{
  scale_factor[0] = x_scale_factor;
  scale_factor[1] = y_scale_factor;
  scale_factor[2] = z_scale_factor;
}

BOOL LASreaderBILrescale::open(const CHAR* file_name)
{
  LASreaderBIL::set_scale_factor(scale_factor);
  if (!LASreaderBIL::open(file_name)) return FALSE;
  return TRUE;
}

LASreaderBILreoffset::LASreaderBILreoffset(F64 x_offset, F64 y_offset, F64 z_offset) : LASreaderBIL()
{
  this->offset[0] = x_offset;
  this->offset[1] = y_offset;
  this->offset[2] = z_offset;
}

BOOL LASreaderBILreoffset::open(const CHAR* file_name)
{
  LASreaderBIL::set_offset(offset);
  if (!LASreaderBIL::open(file_name)) return FALSE;
  return TRUE;
}

LASreaderBILrescalereoffset::LASreaderBILrescalereoffset(F64 x_scale_factor, F64 y_scale_factor, F64 z_scale_factor, F64 x_offset, F64 y_offset, F64 z_offset) : LASreaderBILrescale(x_scale_factor, y_scale_factor, z_scale_factor), LASreaderBILreoffset(x_offset, y_offset, z_offset)
{
}

BOOL LASreaderBILrescalereoffset::open(const CHAR* file_name)
{
  LASreaderBIL::set_scale_factor(scale_factor);
  LASreaderBIL::set_offset(offset);
  if (!LASreaderBIL::open(file_name)) return FALSE;
  return TRUE;
}