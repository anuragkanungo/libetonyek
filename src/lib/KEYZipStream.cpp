/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>
#include <zlib.h>
#include <map>

#include "KEYMemoryStream.h"
#include "KEYZipStream.h"

namespace
{

struct LocalFileHeader
{
  unsigned short general_flag;
  unsigned short compression;
  unsigned crc32;
  unsigned compressed_size;
  unsigned uncompressed_size;
  std::string filename;
  LocalFileHeader()
    : general_flag(0), compression(0), crc32(0), compressed_size(0), uncompressed_size(0), filename() {}
  ~LocalFileHeader() {}
};

struct CentralDirectoryEntry
{
  unsigned short general_flag;
  unsigned short compression;
  unsigned crc32;
  unsigned compressed_size;
  unsigned uncompressed_size;
  unsigned offset;
  std::string filename;
  CentralDirectoryEntry()
    : general_flag(0), compression(0), crc32(0), compressed_size(0), uncompressed_size(0), offset(0), filename() {}
  ~CentralDirectoryEntry() {}
};

struct CentralDirectoryEnd
{
  unsigned cdir_size;
  unsigned cdir_offset;
  CentralDirectoryEnd()
    : cdir_size(0), cdir_offset(0) {}
  ~CentralDirectoryEnd() {}
};

} // anonymous namespace

namespace libetonyek
{

struct KEYZipStreamImpl
{
  WPXInputStreamPtr_t m_input;
  unsigned m_cdir_offset;
  std::map<std::string, CentralDirectoryEntry> m_cdir;
  bool m_initialized;
  KEYZipStreamImpl(const WPXInputStreamPtr_t &input)
    : m_input(input), m_cdir_offset(0), m_cdir(), m_initialized(false) {}
  ~KEYZipStreamImpl() {}

  bool isZipStream();
  WPXInputStream *getSubstream(const char *name);
private:
  KEYZipStreamImpl(const KEYZipStreamImpl &);
  KEYZipStreamImpl &operator=(const KEYZipStreamImpl &);

  bool findCentralDirectoryEnd();
  bool readCentralDirectoryEnd(CentralDirectoryEnd &end);
  bool readCentralDirectory(const CentralDirectoryEnd &end);
  bool readLocalFileHeader(LocalFileHeader &header);
  bool areHeadersConsistent(const LocalFileHeader &header, const CentralDirectoryEntry &entry);
};

KEYZipStream::KEYZipStream(const WPXInputStreamPtr_t &input) :
  WPXInputStream(),
  m_pImpl(new KEYZipStreamImpl(input))
{
}

KEYZipStream::~KEYZipStream()
{
  if (m_pImpl)
    delete m_pImpl;
}

const unsigned char *KEYZipStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
  return m_pImpl->m_input->read(numBytes, numBytesRead);
}

int KEYZipStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
  return m_pImpl->m_input->seek(offset, seekType);
}

long KEYZipStream::tell()
{
  return m_pImpl->m_input->tell();
}

bool KEYZipStream::atEOS()
{
  return m_pImpl->m_input->atEOS();
}

bool KEYZipStream::isOLEStream()
{
  return m_pImpl->isZipStream();
}

WPXInputStream *KEYZipStream::getDocumentOLEStream(const char *name)
{
  if (!m_pImpl->isZipStream())
    return 0;
  return m_pImpl->getSubstream(name);
}

#define CDIR_ENTRY_SIG 0x02014b50
#define LOC_FILE_HEADER_SIG 0x04034b50
#define CDIR_END_SIG 0x06054b50

bool KEYZipStreamImpl::findCentralDirectoryEnd()
{
  if (m_cdir_offset || m_input->seek(-1024, WPX_SEEK_END))
    m_input->seek(m_cdir_offset, WPX_SEEK_SET);
  try
  {
    while (!m_input->atEOS())
    {
      unsigned signature = readU32(m_input);
      if (signature == CDIR_END_SIG)
      {
        m_input->seek(-4, WPX_SEEK_CUR);
        m_cdir_offset = m_input->tell();
        return true;
      }
      else
        m_input->seek(-3, WPX_SEEK_CUR);
    }
  }
  catch (...)
  {
    return false;
  }
  return false;
}

bool KEYZipStreamImpl::isZipStream()
{
  if (m_cdir_offset)
  {
    if(m_cdir.empty())
      return false;
    return true;
  }
  if (m_initialized)
    return false;
  m_initialized = true;
  if (!findCentralDirectoryEnd())
    return false;
  CentralDirectoryEnd end;
  if (!readCentralDirectoryEnd(end))
    return false;
  if (!readCentralDirectory(end))
    return false;
  if (m_cdir.empty())
    return false;
  CentralDirectoryEntry entry = m_cdir.begin()->second;
  m_input->seek(entry.offset, WPX_SEEK_SET);
  LocalFileHeader header;
  if (!readLocalFileHeader(header))
    return false;
  if (!areHeadersConsistent(header, entry))
    return false;
  return true;
}

bool KEYZipStreamImpl::readCentralDirectory(const CentralDirectoryEnd &end)
{
  try
  {
    m_input->seek(end.cdir_offset, WPX_SEEK_SET);
    while (!m_input->atEOS())
    {
      unsigned signature = readU32(m_input);
      if (signature != CDIR_ENTRY_SIG)
      {
        if (m_cdir.empty())
          return false;
        else
          return true;
      }

      CentralDirectoryEntry entry;
      m_input->seek(4, WPX_SEEK_CUR);
      entry.general_flag = readU16(m_input);
      entry.compression = readU16(m_input);
      m_input->seek(4, WPX_SEEK_CUR);
      entry.crc32 = readU32(m_input);
      entry.compressed_size = readU32(m_input);
      entry.uncompressed_size = readU32(m_input);
      unsigned short filename_size = readU16(m_input);
      unsigned short extra_field_size = readU16(m_input);
      unsigned short file_comment_size = readU16(m_input);
      m_input->seek(8, WPX_SEEK_CUR);
      entry.offset = readU32(m_input);
      entry.filename.clear();
      entry.filename.reserve(filename_size);
      unsigned long bytesRead = 0;
      const unsigned char *buffer = m_input->read(filename_size, bytesRead);
      entry.filename.assign((const char *)buffer, bytesRead);
      m_input->seek(extra_field_size+file_comment_size, WPX_SEEK_CUR);

      m_cdir[entry.filename] = entry;
    }
  }
  catch (...)
  {
    return false;
  }
  return true;
}

WPXInputStream *KEYZipStreamImpl::getSubstream(const char *name)
{
  if (m_cdir.empty())
    return 0;
  std::map<std::string, CentralDirectoryEntry>::const_iterator iter = m_cdir.lower_bound(name);
  if (iter == m_cdir.end())
    return 0;
  if (m_cdir.key_comp()(name, iter->first))
  {
    size_t name_length = strlen(name);
    if (iter->first.compare(0, name_length, name))
      return 0;
  }
  CentralDirectoryEntry entry = iter->second;
  m_input->seek(entry.offset, WPX_SEEK_SET);
  LocalFileHeader header;
  if (!readLocalFileHeader(header))
    return 0;
  if (!areHeadersConsistent(header, entry))
    return 0;
  if (!entry.compression)
    return new KEYMemoryStream(m_input, entry.compressed_size);
  else
  {
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,-MAX_WBITS);
    if (ret != Z_OK)
      return 0;

    unsigned long numBytesRead = 0;
    const unsigned char *compressedData = m_input->read(entry.compressed_size, numBytesRead);
    if (numBytesRead != entry.compressed_size)
      return 0;

    strm.avail_in = numBytesRead;
    strm.next_in = (Bytef *)compressedData;

    std::vector<unsigned char>data(entry.uncompressed_size);

    strm.avail_out = entry.uncompressed_size;
    strm.next_out = reinterpret_cast<Bytef *>(&data[0]);
    ret = inflate(&strm, Z_FINISH);
    switch (ret)
    {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      (void)inflateEnd(&strm);
      data.clear();
      return 0;
    }
    (void)inflateEnd(&strm);
    return new KEYMemoryStream(data);
  }
}

bool KEYZipStreamImpl::readCentralDirectoryEnd(CentralDirectoryEnd &end)
{
  try
  {
    unsigned signature = readU32(m_input);
    if (signature != CDIR_END_SIG)
      return false;

    m_input->seek(8, WPX_SEEK_CUR);
    end.cdir_size = readU32(m_input);
    end.cdir_offset = readU32(m_input);
    unsigned short comment_size = readU16(m_input);
    m_input->seek(comment_size, WPX_SEEK_CUR);
  }
  catch (...)
  {
    return false;
  }
  return true;
}

bool KEYZipStreamImpl::readLocalFileHeader(LocalFileHeader &header)
{
  try
  {
    unsigned signature = readU32(m_input);
    if (signature != LOC_FILE_HEADER_SIG)
      return false;

    m_input->seek(2, WPX_SEEK_CUR);
    header.general_flag = readU16(m_input);
    header.compression = readU16(m_input);
    m_input->seek(4, WPX_SEEK_CUR);
    header.crc32 = readU32(m_input);
    header.compressed_size = readU32(m_input);
    header.uncompressed_size = readU32(m_input);
    unsigned short filename_size = readU16(m_input);
    unsigned short extra_field_size = readU16(m_input);
    header.filename.clear();
    header.filename.reserve(filename_size);
    unsigned long bytesRead = 0;
    const unsigned char *buffer = m_input->read(filename_size, bytesRead);
    header.filename.assign((const char *)buffer, bytesRead);
    m_input->seek(extra_field_size, WPX_SEEK_CUR);
  }
  catch (...)
  {
    return false;
  }
  return true;
}

bool KEYZipStreamImpl::areHeadersConsistent(const LocalFileHeader &header, const CentralDirectoryEntry &entry)
{
  if (header.general_flag != entry.general_flag)
    return false;
  if (header.compression != entry.compression)
    return false;
  if (!(header.general_flag & 0x08))
  {
    if (header.crc32 != entry.crc32)
      return false;
    if (header.compressed_size != entry.compressed_size)
      return false;
    if (header.uncompressed_size != entry.uncompressed_size)
      return false;
  }
  return true;
}

} // namespace libetonyek

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */