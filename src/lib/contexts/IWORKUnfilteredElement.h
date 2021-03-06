/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IWORKUNFILTEREDELEMENT_H_INCLUDED
#define IWORKUNFILTEREDELEMENT_H_INCLUDED

#include "IWORKXMLContextBase.h"

#include "IWORKImageContext.h"
#include "IWORKTypes_fwd.h"

namespace libetonyek
{

class IWORKUnfilteredElement : public IWORKImageContext
{
public:
  IWORKUnfilteredElement(IWORKXMLParserState &state, IWORKMediaContentPtr_t &content);

private:
  virtual void endOfElement();

private:
  IWORKMediaContentPtr_t &m_content;
};

}

#endif // IWORKUNFILTEREDELEMENT_H_INCLUDED

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
