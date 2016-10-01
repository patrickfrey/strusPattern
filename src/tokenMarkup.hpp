/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation for annotation of spans in text
/// \file "tokenMarkup.hpp"
#ifndef _STRUS_STREAM_TOKEN_MARKUP_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_TOKEN_MARKUP_IMPLEMENTATION_HPP_INCLUDED
#include "strus/tokenMarkupContextInterface.hpp"
#include "strus/tokenMarkupInstanceInterface.hpp"
#include "strus/analyzer/tokenMarkup.hpp"

namespace strus
{
/// \brief Forward declaration
class SegmenterMarkupContextInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation for annotation of text in one document
class TokenMarkupContext
	:public TokenMarkupContextInterface
{
public:
	/// \brief Constructor
	/// \param[in] errorhnd_ error buffer interface
	explicit TokenMarkupContext(
			ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~TokenMarkupContext();

	virtual void putMarkup(
			const SegmenterPosition& start_segpos,
			std::size_t start_ofs,
			const SegmenterPosition& end_segpos,
			std::size_t end_ofs,
			const analyzer::TokenMarkup& markup,
			unsigned int level);

	virtual std::string markupDocument(
			const SegmenterInstanceInterface* segmenter,
			const analyzer::DocumentClass& dclass,
			const std::string& content) const;

private:
	static void writeOpenMarkup(
			SegmenterMarkupContextInterface* markupdoc,
			const SegmenterPosition& segpos, std::size_t ofs, const analyzer::TokenMarkup& markup);

private:
	struct MarkupElement
	{
		SegmenterPosition start_segpos;		///< segment byte address of the start of the markup in the original document
		std::size_t start_ofs;			///< segment byte offset of the start of the markup in the processed segment
		SegmenterPosition end_segpos;		///< segment byte address of the end of the markup in the original document
		std::size_t end_ofs;			///< segment byte offset of the end of the markup in the processed segment
		analyzer::TokenMarkup markup;		///< tag for markup in document
		unsigned int level;			///< level deciding what markup superseds others when they are overlapping
		unsigned int orderidx;			///< index to keep deterministic, stable ordering in sorted array

		MarkupElement( SegmenterPosition start_segpos_, std::size_t start_ofs_, SegmenterPosition end_segpos_, std::size_t end_ofs_, const analyzer::TokenMarkup& markup_, unsigned int level_, unsigned int orderidx_)
			:start_segpos(start_segpos_),start_ofs(start_ofs_),end_segpos(end_segpos_),end_ofs(end_ofs_),markup(markup_),level(level_),orderidx(orderidx_){}
		MarkupElement( const MarkupElement& o)
			:start_segpos(o.start_segpos),start_ofs(o.start_ofs),end_segpos(o.end_segpos),end_ofs(o.end_ofs),markup(o.markup),level(o.level),orderidx(o.orderidx){}

		bool operator<( const MarkupElement& o) const
		{
			if (start_segpos < o.start_segpos) return true;
			if (start_segpos > o.start_segpos) return false;
			if (start_ofs < o.start_ofs) return true;
			if (start_ofs > o.start_ofs) return false;
			if (end_segpos < o.end_segpos) return true;
			if (end_segpos > o.end_segpos) return false;
			if (end_ofs < o.end_ofs) return true;
			if (end_ofs > o.end_ofs) return false;
			if (level < o.level) return true;
			if (level > o.level) return false;
			if (orderidx < o.orderidx) return true;
			if (orderidx > o.orderidx) return false;
			return false;
		}
	};

private:
	std::vector<MarkupElement> m_markupar;
	ErrorBufferInterface* m_errorhnd;
};


class TokenMarkupInstance
	:public TokenMarkupInstanceInterface
{
public:
	/// \brief Constructor
	explicit TokenMarkupInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	/// \brief Destructor
	virtual ~TokenMarkupInstance(){}

	virtual TokenMarkupContextInterface* createContext() const;

private:
	ErrorBufferInterface* m_errorhnd;
};

} //namespace
#endif

