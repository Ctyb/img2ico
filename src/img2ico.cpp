/*
Copyright (c) 2013 Aaron Stone

Licensed under The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "img2ico.h"

using namespace std;

std::fstream& operator>>(std::fstream &in, sImage* image);

std::fstream& operator<<(std::fstream &out, const IconDirEntry icon_dir);
std::fstream& operator<<(std::fstream &out, const IconImage image);
std::fstream& operator<<(std::fstream &out, const sICO_Header ico_hdr);
std::fstream& operator<<(std::fstream &out, const sANI_Header ani_hdr);
std::fstream& operator<<(std::fstream &out, const sANI_Chunk ani_chunk);


sANI_Header::sANI_Header()
{
	s.HeaderID = 'ACON';
	s.HeaderSize = 36;
	s.NumFrames = 1;
	s.NumSteps = 1;
	s.Width = 1;
	s.Height = 1;
	s.BitsPerPixel = 8;
	s.NumPlanes = 1;
	s.DisplayRate = 30;
	s.Flags = 0 | I_ICON;
}

sANI_Chunk::sANI_Chunk()
{
	for (int i = 0; i < 8; i++)
	{
		bytes[i] = 0;
	}

	data = nullptr;	
}

sImage::sImage()
{
	img.and = nullptr;
	img.xor = nullptr;
	img.colors = nullptr;

	for (int i = 0; i < 40; i++)
	{
		img.header.h_bytes[i] = 0;

		if (i < 16)
		{
			dir.bytes[i] = 0;
		}
	}

	img.header.s.HeaderSize = 40;
	img.header.s.NumPlanes = 1;
}

CIMG2ICO::CIMG2ICO(const char* path, const char* name, int type)
{
	SetDirectoryPath(path);
	SetOutputFileName(name);
	SetOutputFileType(type);

	m_bSequenceData = false;
	m_bUseRawData = false;
	m_sImageArray = nullptr;
	m_sICO_Header.s.Reserved = 0;
	m_sICO_Header.s.Count = 1;
}

CIMG2ICO::~CIMG2ICO()
{
	// Free any allocated memory
	if (m_sImageArray != nullptr)
	{
		for (int i = 0; i < m_sICO_Header.s.Count; i++)
		{
			if (m_sImageArray[i].img.and != nullptr)
			{
				delete m_sImageArray[i].img.and;
			}

			if (m_sImageArray[i].img.xor != nullptr)
			{
				delete m_sImageArray[i].img.xor;
			}

			if (m_sImageArray[i].img.colors != nullptr)
			{
				delete m_sImageArray[i].img.colors;
			}
		}

		delete m_sImageArray;
	}
}


int	CIMG2ICO::LoadImage(const char* filename, struct sImage* image)
{
	int		retval = 0;
	uBuffer	buffer;
	fstream imagefile;

	buffer.dword = 0;
	imagefile.open(filename, ios::in | ios::binary);

	if (imagefile.is_open() == true)
	{
		imagefile.read(&buffer.byte[0], 4);

		if (buffer.dword == _PNG_HEADER_DWORD)
		{
			imagefile.read(&buffer.byte[0], 4);
			imagefile.read(&buffer.byte[0], 4);
			
			if (buffer.dword == _PNG_CHUNK_IHDR)
			{
				// Load image parameters
			}

			if (buffer.dword == _PNG_CHUNK_PLTE)
			{
				// Load image palette
			}

			// Load entire PNG into sImage.imagedata

		}
		else if (buffer.word[0] == _BMP_HEADER_WORD)
		{
			imagefile >> image;

			if (image->img.header.s.CompressionType == _BMP_BI_RGB)
			{
				imagefile.seekg(image->dir.s.Offset, imagefile.beg);
				image->img.xor = new __int8[image->img.XorSize];
				imagefile.read(image->img.xor, image->img.XorSize);
				image->img.and = new __int8[image->img.AndmaskSize];

				// Build AND mask
				if (image->img.xor != nullptr)
				{
					for (int i = 0; i < image->img.AndmaskSize; i++)
					{
						image->img.and[i]  = 0x00;

						image->img.and[i] |= (image->img.xor[i]       == 0) ? 0x01 : 0;
						image->img.and[i] |= (image->img.xor[i+(1*3)] == 0) ? 0x02 : 0;
						image->img.and[i] |= (image->img.xor[i+(2*3)] == 0) ? 0x04 : 0;
						image->img.and[i] |= (image->img.xor[i+(3*3)] == 0) ? 0x08 : 0;
						image->img.and[i] |= (image->img.xor[i+(4*3)] == 0) ? 0x10 : 0;
						image->img.and[i] |= (image->img.xor[i+(5*3)] == 0) ? 0x20 : 0;
						image->img.and[i] |= (image->img.xor[i+(6*3)] == 0) ? 0x40 : 0;
						image->img.and[i] |= (image->img.xor[i+(7*3)] == 0) ? 0x80 : 0;
					}
				}
			}
			else
			{
				retval = 1;
			}
		}
		else
		{
			retval = 1;		// Input File is not BMP or PNG
		}

		imagefile.close();
	}
	else
	{
		m_sANI_Header.s.NumFrames = 0;
		retval = -1;
	}

	return retval;
}

int	CIMG2ICO::ReadConfigFile(void)
{
	int		retval = 0;
	string	szConfigFilename = "";
	fstream	c_file;

	szConfigFilename.assign(m_szPath);
	szConfigFilename.append(_SZ_PATHSEPARATOR);
	szConfigFilename.append("config");
	c_file.open(szConfigFilename.data(), ios::in);

	if (c_file.is_open())
	{
		// Read config parameters into m_sANI_Header

		c_file.close();
	}
	else
	{
		if (m_sICO_Header.s.Type == T_ANI)
		{
			retval = 40;
		}
	}

	return retval;
}

int		CIMG2ICO::ReadInputFiles(void)
{
	int retval = 0;
	
	// Read config file if present
	ReadConfigFile();

	// Find out how many images are in the directory (PNG or BMP only)

		// update m_iCount

	// Allocate array for images

	// Load images into sImages

	// Populate all class variables



	
	//// Start test code
	m_sImageArray = new sImage;

	retval = LoadImage("0.bmp", &m_sImageArray[0]);	// temporary code

	//// End test code
	
	return retval;
}

void	CIMG2ICO::SetDirectoryPath(const char* path)
{
	if (m_szPath.length() == 0)
	{
		m_szPath.assign(".");
	}
	else
	{
		m_szPath.assign(path);
	}
}

void	CIMG2ICO::SetOutputFileName(const char* filename)
{
	if ((m_szName.length()) == 0)
	{
		m_szName.assign("icon.ico");
	}
	else
	{
		m_szName.assign(filename);
	}	
}

void	CIMG2ICO::SetOutputFileType(const int type)
{
	m_sICO_Header.s.Type = ( (type > 0) && (type <= 3) ) ? type : T_ICO;
}

int		CIMG2ICO::WriteOutputFile(void)
{
	int		retval = 0;
	fstream	file;
	string	szOutFilename = "";
		
	szOutFilename.assign(m_szPath);
	szOutFilename.append(_SZ_PATHSEPARATOR);
	szOutFilename.append(m_szName);

	file.open(szOutFilename.data(), ios::out | ios::binary);

	if (file.is_open())
	{
		switch(m_sICO_Header.s.Type)
		{
		case T_ANI:
			if (m_sANI_Header.s.NumFrames != 0)
			{
				file << m_sANI_Header;

				// Write frames
			
				if (m_bSequenceData == true)
				{
					// write sequence data
				}
				retval = 1;
			}
			else
			{
				retval = 1;
			}
			break;
		default:
		case T_CUR:
		case T_ICO:
			if (m_sICO_Header.s.Count != 0)
			{
				file << m_sICO_Header;

				// Build image directory
				for (int i = 0; i < m_sICO_Header.s.Count; i++)
				{
					m_sImageArray[i].dir.s.Offset = (i == 0) ? (6 + (16 * m_sICO_Header.s.Count) ) : (m_sImageArray[i-1].dir.s.Offset + m_sImageArray[i-1].dir.s.Size);
					file << m_sImageArray[i].dir;
				}

				// Write images
				for (int i = 0; i < m_sICO_Header.s.Count; i++)
				{
					file << m_sImageArray[i].img;
				}
			}
			else
			{
				retval = 1;
			}
		}

		file.close();
	}
	else
	{
		retval = 1;
	}

	return retval;
}

int		CIMG2ICO::ConvertFiles(void)
{
	int retval = 0;

	retval = ReadInputFiles();
	retval += WriteOutputFile();

	return retval;
}

std::fstream& operator>>(std::fstream &in, sImage* image)
{
	uBuffer buf[8];

	in.read(&buf[0].byte[2], sizeof(buf)-2);
	
	image->dir.s.Offset					= buf[2].dword;
	image->img.header.s.Width			= buf[4].dword;
	image->img.header.s.Height			= buf[5].dword;
	image->img.header.s.BitsPerPixel	= buf[6].word[1];
	image->img.header.s.ImageSize		= (buf[4].dword * buf[5].dword * buf[6].word[1] / 8);

	image->dir.s.Width					= image->img.header.s.Width;
	image->dir.s.Height					= image->img.header.s.Height;
	image->dir.s.BPP_Vcor				= image->img.header.s.BitsPerPixel;

	image->img.XorSize					= image->dir.s.Width * image->dir.s.Height * image->img.header.s.BitsPerPixel / 8;
	image->img.AndmaskSize				= image->img.XorSize / image->img.header.s.BitsPerPixel;
	image->img.header.s.Height		   *= 2;

	// header size + image size + andmask size
	image->dir.s.Size					= image->img.header.s.HeaderSize + image->img.header.s.ImageSize + image->img.AndmaskSize;
	
	return in;
}

std::fstream& operator<<(std::fstream &out, const IconDirEntry icon_dir)
{
	out.write(&icon_dir.bytes[0], 16 );

	return out;
}

std::fstream& operator<<(std::fstream &out, const IconImage image)
{
	out.write(&image.header.h_bytes[0], 40 );

	if (image.colors != nullptr)
	{
		out.write(&image.colors[0], image.header.s.ImageSize );
	}

	if (image.xor != nullptr)
	{
		out.write(&image.xor[0], (image.XorSize) );
	}

	if (image.and != nullptr)
	{
		out.write(&image.and[0], (image.AndmaskSize) );
	}

	return out;
}

std::fstream& operator<<(std::fstream &out, const sICO_Header ico_hdr)
{
	out.write(&ico_hdr.bytes[0], 6 );

	return out;
}

std::fstream& operator<<(std::fstream &out, const sANI_Header ani_hdr)
{
	out.write(&ani_hdr.bytes[0], 40);

	return out;
}

std::fstream& operator<<(std::fstream &out, const sANI_Chunk ani_chunk)
{
	// Ignore chunk if it has no data
	if (ani_chunk.data != nullptr)
	{
		out.write(&ani_chunk.bytes[0], 8);
		out.write(&ani_chunk.data[0], ani_chunk.s.size);
	}

	return out;
}