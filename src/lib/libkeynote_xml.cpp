/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libkeynote project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "libkeynote_xml.h"

namespace libkeynote
{

namespace
{

struct XMLException {};

}

void moveToNextNode(xmlTextReaderPtr reader)
{
  int type = 0;
  do
  {
    const int ret = xmlTextReaderRead(reader);
    if (ret != 1)
      throw XMLException();
    type = xmlTextReaderNodeType(reader);
  }
  while ((XML_READER_TYPE_ELEMENT != type) || (XML_READER_TYPE_END_ELEMENT != type) || (XML_READER_TYPE_TEXT != type));
}

bool skipElement(xmlTextReaderPtr reader)
{
  int level = 1;

  int ret = xmlTextReaderRead(reader);
  while ((1 == ret) && (0 < level))
  {
    switch (xmlTextReaderNodeType(reader))
    {
    case XML_READER_TYPE_ELEMENT :
      if (!xmlTextReaderIsEmptyElement(reader))
        ++level;
      break;
    case XML_READER_TYPE_END_ELEMENT :
      --level;
      break;
    }

    ret = xmlTextReaderRead(reader);
  }

  return (-1 != ret) && (0 == level);
}

bool isEmptyElement(xmlTextReaderPtr reader)
{
  return xmlTextReaderIsEmptyElement(reader);
}

bool isStartElement(xmlTextReaderPtr reader)
{
  return XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType(reader);
}

bool isEndElement(xmlTextReaderPtr reader)
{
  return XML_READER_TYPE_END_ELEMENT == xmlTextReaderNodeType(reader);
}

const char *getName(xmlTextReaderPtr reader)
{
  return reinterpret_cast<const char *>(xmlTextReaderConstLocalName(reader));
}

const char *getNamespace(xmlTextReaderPtr reader)
{
  return reinterpret_cast<const char *>(xmlTextReaderConstNamespaceUri(reader));
}

const char *getText(xmlTextReaderPtr reader)
{
  return reinterpret_cast<const char *>(xmlTextReaderConstValue(reader));
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
