/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief StrusStream program implementation for loading pattern definitions from source
/// \file "patternMatcherProgram.hpp"
#ifndef _STRUS_STREAM_PATTERN_MATCHER_PROGRAM_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STREAM_PATTERN_MATCHER_PROGRAM_IMPLEMENTATION_HPP_INCLUDED
#include "strus/reference.hpp"
#include "strus/patternMatcherProgramInterface.hpp"
#include "strus/patternMatcherProgramInstanceInterface.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherInterface.hpp"
#include "strus/analyzer/patternMatcherOptions.hpp"
#include "strus/analyzer/patternLexerOptions.hpp"
#include "strus/patternLexerInstanceInterface.hpp"
#include "strus/patternLexerInterface.hpp"
#include "symbolTable.hpp"
#include <string>
#include <vector>
#include <set>
#include <cstddef>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief StrusStream program loader for loading pattern definitions from source
class PatternMatcherProgramInstance
	:public PatternMatcherProgramInstanceInterface
{
public:
	/// \brief Constructor
	PatternMatcherProgramInstance(
			const PatternMatcherInterface* tpm,
			const PatternLexerInterface* crm,
			ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~PatternMatcherProgramInstance(){}

	virtual bool load( const std::string& source);
	virtual bool compile();

	virtual const PatternLexerInstanceInterface* getPatternLexerInstance() const;
	virtual const PatternMatcherInstanceInterface* getPatternMatcherInstance() const;

	virtual const char* tokenName( unsigned int id) const;

private:
	struct SubExpressionInfo
	{
		explicit SubExpressionInfo( unsigned int minrange_=0)
			:minrange(minrange_){}
		SubExpressionInfo( const SubExpressionInfo& o)
			:minrange(o.minrange){}

		unsigned int minrange;
	};

	void loadOption( char const*& si);
	void loadExpression( char const*& si, SubExpressionInfo& exprinfo);
	void loadExpressionNode( const std::string& name, char const*& si, SubExpressionInfo& exprinfo);
	uint32_t getOrCreateSymbol( unsigned int regexid, const std::string& name);
	const char* getSymbolRegexId( unsigned int id) const;

private:
	enum {MaxRegularExpressionNameId=(1<<24)};
	ErrorBufferInterface* m_errorhnd;
	std::vector<std::string> m_patternMatcherOptionNames;
	std::vector<std::string> m_patternLexerOptionNames;
	analyzer::PatternMatcherOptions m_patternMatcherOptions;
	analyzer::PatternLexerOptions m_patternLexerOptions;
	Reference<PatternMatcherInstanceInterface> m_patternMatcher;
	Reference<PatternLexerInstanceInterface> m_patternLexer;
	SymbolTable m_regexNameSymbolTab;
	SymbolTable m_patternNameSymbolTab;
	std::vector<uint32_t> m_symbolRegexIdList;
	std::set<uint32_t> m_unresolvedPatternNameSet;
};


/// \brief StrusStream interface to load pattern definitions from source
class PatternMatcherProgram
	:public PatternMatcherProgramInterface
{
public:
	PatternMatcherProgram( const PatternMatcherInterface* tpm_, const PatternLexerInterface* crm_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_tpm(tpm_),m_crm(crm_){}

	virtual ~PatternMatcherProgram(){}

	virtual PatternMatcherProgramInstanceInterface* createInstance() const;

private:
	ErrorBufferInterface* m_errorhnd;
	const PatternMatcherInterface* m_tpm;
	const PatternLexerInterface* m_crm;
};




} //namespace
#endif

