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
#include "strus/segmenterContextInterface.hpp"
#include "strus/documentClass.hpp"
#include <vector>
#include <string>

namespace strus
{
///\brief Forward declaration
class SegmenterInstanceInterface;

class TokenMarkup
{
public:
	class Attribute
	{
	public:
		Attribute( const std::string& name_, const std::string& value_)
			:m_name(name_),m_value(value_){}
		Attribute( const Attribute& o)
			:m_name(o.m_name),m_value(o.m_value){}

		const std::string& name() const			{return m_name;}
		const std::string& value() const		{return m_value;}

	private:
		std::string m_name;
		std::string m_value;
	};

	TokenMarkup( const std::string& name_)
		:m_name(name_),m_attributes(){}
	TokenMarkup( const std::string& name_, const std::vector<Attribute>& attributes_)
		:m_name(name_),m_attributes(attributes_){}
	TokenMarkup( const TokenMarkup& o)
		:m_name(o.m_name),m_attributes(o.m_attributes){}

	const std::string& name() const				{return m_name;}
	const std::vector<Attribute>& attributes() const	{return m_attributes;}

	TokenMarkup& operator()( const std::string& name_, const std::string& value_)
	{
		m_attributes.push_back( Attribute( name_, value_));
		return *this;
	}

private:
	std::string m_name;
	std::vector<Attribute> m_attributes;
};


/// \brief Interface for annotation of text in one document
class TokenMarkupContextInterface
{
public:
	/// \brief Destructor
	virtual ~TokenMarkupContextInterface(){}

	/// \brief Define a marker for a span in the text
	/// \param[in] start_segpos absolute segment position where the start of the token to mark has been defined (this position is not the absolute byte position in the document, but the absolute byte position of the segment where the token has been found plus the relative byte position of the token in the segment with all encoded entities or escaping resolved. The real absolute byte position can be calculated by this markup class by mapping the offset in the segment with encoded entities to the offset in the original segnment.)
	/// \param[in] start_ofs offset of the token to mark in bytes in the segment processed
	/// \param[in] end_segpos absolute segment position where the end of the to mark has been defined (this position is not the absolute byte position in the document, but the absolute byte position of the segment where the token has been found plus the relative byte position of the token in the segment with all encoded entities or escaping resolved. The real absolute byte position can be calculated by this markup class by mapping the offset in the segment with encoded entities to the offset in the original segnment.)
	/// \param[in] start_ofs offset of the token to mark in bytes in the segment processed
	/// \param[in] markup tag structure to use for markup
	/// \param[in] level sort of priority (areas with a higher level markup are superseding the ones with lovel level. It is also used as criterion to resolve conflicts)
	virtual void putMarkup(
			const SegmenterPosition& start_segpos,
			std::size_t start_ofs,
			const SegmenterPosition& end_segpos,
			std::size_t end_ofs,
			const TokenMarkup& markup,
			unsigned int level)=0;

	/// \brief Get the original document content with all markups declared inserted
	/// \param[in] segmenter segmenter to use for inserting document markup tags
	/// \param[in] dclass document class of document to markup
	/// \param[in] content content string of document to markup
	/// \return the marked up document content
	virtual std::string markupDocument(
			const SegmenterInstanceInterface* segmenter,
			const DocumentClass& dclass,
			const std::string& content) const=0;
};

} //namespace
#endif

