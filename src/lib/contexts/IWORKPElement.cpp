/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libetonyek project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IWORKPElement.h"

#include "IWORKBrContext.h"
#include "IWORKCollector.h"
#include "IWORKDictionary.h"
#include "IWORKLinkElement.h"
#include "IWORKSpanElement.h"
#include "IWORKStyle.h"
#include "IWORKTabElement.h"
#include "IWORKToken.h"
#include "IWORKXMLParserState.h"

namespace libetonyek
{

IWORKPElement::IWORKPElement(IWORKXMLParserState &state)
  : IWORKXMLMixedContextBase(state)
  , m_style()
  , m_opened(false)
{
}

void IWORKPElement::attribute(const int name, const char *const value)
{
  switch (name)
  {
  case IWORKToken::NS_URI_SF | IWORKToken::style :
  {
    const IWORKStyleMap_t::const_iterator it = getState().getDictionary().m_paragraphStyles.find(value);
    if (getState().getDictionary().m_paragraphStyles.end() != it)
      m_style = it->second;
    break;
  }
  }
}

IWORKXMLContextPtr_t IWORKPElement::element(const int name)
{
  ensureOpened();

  switch (name)
  {
  case IWORKToken::NS_URI_SF | IWORKToken::crbr :
  case IWORKToken::NS_URI_SF | IWORKToken::intratopicbr :
  case IWORKToken::NS_URI_SF | IWORKToken::lnbr :
    return makeContext<IWORKBrContext>(getState());
  case IWORKToken::NS_URI_SF | IWORKToken::span :
    return makeContext<IWORKSpanElement>(getState());
  case IWORKToken::NS_URI_SF | IWORKToken::tab :
    return makeContext<IWORKTabElement>(getState());
  case IWORKToken::NS_URI_SF | IWORKToken::link :
    return makeContext<IWORKLinkElement>(getState());
  }

  return IWORKXMLContextPtr_t();
}

void IWORKPElement::text(const char *const value)
{
  ensureOpened();
  if (isCollector())
    getCollector().collectText(value);
}

void IWORKPElement::endOfElement()
{
  ensureOpened();
  if (isCollector())
    getCollector().endParagraph();
}

void IWORKPElement::ensureOpened()
{
  if (!m_opened)
  {
    if (isCollector())
      getCollector().startParagraph(m_style);
    m_opened = true;
  }
}

}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
