/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for annotation of text in one document
/// \file "tokenMarkupContextInterface.hpp"
#ifndef _STRUS_STREAM_TOKEN_MARKUP_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_MARKUP_CONTEXT_INTERFACE_HPP_INCLUDED
#include <vector>

namespace strus
{

/// \brief Interface for annotation of text in one document
class TokenMarkupContextInterface
{
public:
	/// \brief Destructor
	virtual ~TokenMarkupContextInterface(){}

	/// \brief Declare a document segment
	/// \param[in] byte position of the segment in the original source
	/// \param[in] segment poiner to segment processed
	/// \param[in] segmentsize size of 'segment' in bytes
	virtual void declareSegment(
			std::size_t position,
			const char* segment,
			std::size_t segmentsize)=0;

	/// \brief Define a marker in the text
	/// \param[in] startpos absolute position where the token to mark has been defined (this position is not the absolute byte position in the document, but the absolute byte position of the segment where the token has been found plus the relative byte position of the token in the segment with all encoded entities or escaping resolved. The real absolute byte position can be calculated by this markup class by mapping the offset in the segment with encoded entities to the offset in the original segnment.)
	/// \param[in] size size of the token to mark in bytes (with all encoded entities decoded. The real size in the original document has to be calculated for a correct markup)
	/// \param[in] openMarker string to put as start of the markup
	/// \param[in] closeMarker string to put as end of the markup
	/// \param[in] level sort of priority (areas with a higher level markup are superseding the ones with lovel level. It is also used as criterion to resolve conflicts)
	virtual void putMarkup(
			std::size_t start_origseg,
			std::size_t start_origpos,
			std::size_t size,
			const std::string& openMarker,
			const std::string& closeMarker,
			unsigned int level)=0;

	/// \brief Get the original document content with all markups declared inserted
	/// \return the marked up document content
	virtual std::string getContent() const=0;
};

} //namespace
#endif

