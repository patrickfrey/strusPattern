/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of an automaton for detecting patterns in a document stream
/// \file "streamPatternMatch.cpp"
#include "streamPatternMatch.hpp"
#include "symbolTable.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "strus/stream/patternMatchResultItem.hpp"
#include "strus/stream/patternMatchResult.hpp"
#include "strus/streamPatternMatchInstanceInterface.hpp"
#include "strus/streamPatternMatchContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "ruleMatcherAutomaton.hpp"
#include <map>
#include <limits>
#include <vector>
#include <cstring>


using namespace strus;
using namespace strus::stream;

struct StreamPatternMatchData
{
	StreamPatternMatchData(){}

	SymbolTable variableMap;
	SymbolTable patternMap;
	ProgramTable programTable;
};

enum PatternEventType {TermEvent=0, ExpressionEvent=1, ReferenceEvent=2};
static uint32_t eventHandle( PatternEventType type_, uint32_t idx)
{
	if (idx >= (1<<30)) throw std::bad_alloc();
	return idx | ((uint32_t)type_ << 30);
}

static bool getKeyValueStrBuf( char* buf, std::size_t bufsize, std::size_t& size, const std::string& type, const std::string& value)
{
	if (type.size() + value.size() < bufsize -2)
	{
		std::string::const_iterator ti = type.begin(), te = type.end();
		std::size_t tidx = 0;
		for (; ti != te; ++ti,++tidx)
		{
			buf[ tidx] = tolower( *ti);
		}
		if (type.size()) buf[ tidx++] = ' ';
		std::memcpy( buf+tidx, value.c_str(), value.size());
		tidx += value.size();
		size = tidx;
		return true;
	}
	return false;
}

class StreamPatternMatchContext
	:public StreamPatternMatchContextInterface
{
public:
	StreamPatternMatchContext( const StreamPatternMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_statemachine(&data_->programTable){}

	virtual ~StreamPatternMatchContext(){}

	virtual unsigned int termId( const std::string& type, const std::string& value) const
	{
		try
		{
			enum {StrBufSize=256};
			char keybuf[ StrBufSize];
			std::size_t keysize = 0;
			if (getKeyValueStrBuf( keybuf, StrBufSize, keysize, type, value))
			{
				return m_data->variableMap.get( keybuf, keysize);
			}
			else if (type.empty())
			{
				return m_data->variableMap.get( value);
			}
			else
			{
				return m_data->variableMap.get( utils::tolower(type)+" "+value);
			}
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier", *m_errorhnd, 0);
	}

	virtual void putInput( unsigned int termid, unsigned int ordpos, unsigned int origpos, unsigned int origsize)
	{
		try
		{
			uint32_t eventid = eventHandle( TermEvent, termid);
			EventData data( origpos, origsize, ordpos, eventid, 0/*subdataref*/);
			m_statemachine.doTransition( eventid, data);
		}
		CATCH_ERROR_MAP( "failed to feed input to pattern patcher", *m_errorhnd);
	}

	void gatherResultItems( std::vector<PatternMatchResultItem>& resitemlist, uint32_t dataref)
	{
		uint32_t itemList = m_statemachine.getEventDataItemListIdx( dataref);
		const EventItem* item;
		while (0!=(item=m_statemachine.nextResultItem( itemList)))
		{
			const char* itemName = m_data->variableMap.key( item->variable);
			PatternMatchResultItem rtitem( itemName, item->data.ordpos, item->data.origpos, item->data.origsize);
			resitemlist.push_back( rtitem);
			if (item->data.subdataref)
			{
				gatherResultItems( resitemlist, item->data.subdataref);
			}
		}
	}

	virtual std::vector<stream::PatternMatchResult> fetchResult()
	{
		try
		{
			std::vector<stream::PatternMatchResult> rt;
			const StateMachine::ResultList& results = m_statemachine.results();
			std::size_t ai = 0, ae = results.size();
			for (; ai != ae; ++ai)
			{
				const Result& result = results[ ai];
				const char* resultName = m_data->patternMap.key( result.resultHandle);
				std::vector<PatternMatchResultItem> rtitemlist;
				gatherResultItems( rtitemlist, result.eventDataReferenceIdx);
				rt.push_back( PatternMatchResult( resultName, rtitemlist));
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( "failed to fetch pattern match result", *m_errorhnd, std::vector<stream::PatternMatchResult>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const StreamPatternMatchData* m_data;
	StateMachine m_statemachine;
};

///\brief Join operations (same meaning as in query evaluation
enum JoinOperation
{
	OpSequence,		///< A subset specified by cardinality of the trigger events must appear in the specified order for then completion of the rule (objects must not overlap)
	OpSequenceStruct,	///< A subset specified by cardinality of the trigger events must appear in the specified order without a structure element appearing before the last element for then completion of the rule (objects must not overlap)
	OpInRange,		///< A subset specified by cardinality of the trigger events must appear for then completion of the rule (objects may overlap)
	OpInRangeStruct,	///< A subset specified by cardinality of the trigger events must appear without a structure element appearing before the last element for then completion of the rule (objects may overlap)
	OpAny			///< Any of the trigger events leads for the completion of the rule
};

static JoinOperation joinOperationName( const char* joinopstr)
{
	static const char* ar[] = {"sequence","sequence_struct","inrange","inrange_struct","any",0};
	std::size_t ai = 0;
	for (; ar[ai]; ++ai)
	{
		if (strus::utils::caseInsensitiveEquals( joinopstr, ar[ai]))
		{
			return (JoinOperation)ai;
		}
	}
	throw strus::runtime_error(_TXT("unknown join operation of nodes '%s'"), joinopstr);
}


/// \brief Interface for building the automaton for detecting patterns in a document stream
class StreamPatternMatchInstance
	:public StreamPatternMatchInstanceInterface
{
public:
	explicit StreamPatternMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_expression_event_cnt(0){}

	virtual ~StreamPatternMatchInstance(){}

	virtual unsigned int getTermId( const std::string& type, const std::string& value)
	{
		try
		{
			enum {StrBufSize=256};
			char keybuf[ StrBufSize];
			std::size_t keysize = 0;
			if (getKeyValueStrBuf( keybuf, StrBufSize, keysize, type, value))
			{
				return m_data.variableMap.getOrCreate( keybuf, keysize);
			}
			else if (type.empty())
			{
				return m_data.variableMap.getOrCreate( value);
			}
			else
			{
				return m_data.variableMap.getOrCreate( utils::tolower(type)+" "+value);
			}
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier", *m_errorhnd, 0);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
			uint32_t eventid = eventHandle( TermEvent, termid);
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push term on pattern match expression stack", *m_errorhnd);
	}

	virtual void pushExpression(
			const char* operationstr,
			std::size_t argc, unsigned int range_, unsigned int cardinality)
	{
		try
		{
			if (range_ > std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("proximity range value out of range or negative"));
			}
			if (argc > m_stack.size() || (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("expression references more arguments than nodes on the stack"));
			}
			if (cardinality > (unsigned int)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("illegal value for cardinality"));
			}
			JoinOperation joinop = joinOperationName( operationstr);
			uint32_t slot_initsigval = 0;
			uint32_t slot_initcount = cardinality?(uint32_t)cardinality:(uint32_t)argc;
			uint32_t slot_event = eventHandle( ExpressionEvent, ++m_expression_event_cnt);
			uint32_t slot_resultHandle = 0;
			Trigger::SigType slot_sigtype = Trigger::SigAny;

			switch (joinop)
			{
				case OpSequence:
					slot_sigtype = Trigger::SigSequence;
					slot_initsigval = argc;
					break;
				case OpSequenceStruct:
					slot_sigtype = Trigger::SigSequence;
					slot_initsigval = argc;
					break;
				case OpInRange:
					slot_sigtype = Trigger::SigInRange;
					if (argc > 32)
					{
						throw strus::runtime_error(_TXT("number of arguments out of range"));
					}
					slot_initsigval = 0xffFFffFF;
					break;
				case OpInRangeStruct:
					slot_sigtype = Trigger::SigInRange;
					if (argc > 32)
					{
						throw strus::runtime_error(_TXT("number of arguments out of range"));
					}
					slot_initsigval = 0xffFFffFF;
					break;
				case OpAny:
					slot_sigtype = Trigger::SigAny;
					break;
			}
			ActionSlotDef actionSlotDef( slot_initsigval, slot_initcount, slot_event, slot_resultHandle);
			uint32_t program = m_data.programTable.createProgram( 0, actionSlotDef);

			std::size_t ai = 0;
			for (; ai != argc; ++ai)
			{
				bool isKeyEvent = false;
				StackElement& elem = m_stack[ m_stack.size() - argc + ai];
				switch (joinop)
				{
					case OpSequenceStruct:
					case OpInRangeStruct:
						if (ai == 0)
						{
							//... structure delimiter
							m_data.programTable.createTrigger(
								program, elem.eventid, Trigger::SigDel,
								0/*sigval*/, elem.variable);
						}
						else
						{
							isKeyEvent = joinop != OpSequenceStruct || (ai == 1);
							m_data.programTable.createTrigger(
								program, elem.eventid, slot_sigtype,
								argc-ai-1, elem.variable);
							
						}
					case OpSequence:
					case OpInRange:
					case OpAny:
						isKeyEvent = joinop != OpSequence || (ai == 0);
						isKeyEvent = (ai == 0);
						m_data.programTable.createTrigger(
							program, elem.eventid, slot_sigtype,
							argc-ai-1, elem.variable);
				}
				if (isKeyEvent)
				{
					m_data.programTable.defineEventProgram( elem.eventid, program);
				}
			}
			m_stack.resize( m_stack.size() - argc);
			m_stack.push_back( StackElement( slot_event));
		}
		CATCH_ERROR_MAP( "failed to push expression on pattern match expression stack", *m_errorhnd);
	}

	virtual void pushPattern( const std::string& name)
	{
		try
		{
			uint32_t eventid = eventHandle( ReferenceEvent, m_data.patternMap.getOrCreate( name));
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push pattern reference on pattern match expression stack", *m_errorhnd);
	}

	virtual void attachVariable( const std::string& name)
	{
		try
		{
			if (m_stack.empty())
			{
				throw strus::runtime_error(_TXT( "illegal operation attach variable when no node on the stack"));
			}
			StackElement& elem = m_stack.back();
			if (elem.variable)
			{
				throw strus::runtime_error(_TXT( "more than one variable assignment to a node"));
			}
			m_stack.back().variable = m_data.variableMap.getOrCreate( name);
		}
		CATCH_ERROR_MAP( "failed to attach variable to top element of pattern match expression stack", *m_errorhnd);
	}

	virtual void closePattern( const std::string& name, bool visible)
	{
		try
		{
			if (m_stack.empty())
			{
				throw strus::runtime_error(_TXT("illegal operation close pattern when no node on the stack"));
			}
			uint32_t resultHandle = m_data.patternMap.getOrCreate( name);
			uint32_t resultEvent = eventHandle( ReferenceEvent, resultHandle);
			m_data.programTable.defineProgramResult( 
				m_stack.back().program, resultEvent, visible?resultHandle:0);
		}
		CATCH_ERROR_MAP( "failed to close pattern definition on pattern match expression stack", *m_errorhnd);
	}

	virtual StreamPatternMatchContextInterface* createContext() const
	{
		try
		{
			return new StreamPatternMatchContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern match context", *m_errorhnd, 0);
	}

private:
	struct StackElement
	{
		uint32_t eventid;
		uint32_t program;
		uint32_t variable;

		StackElement()
			:eventid(0),program(0),variable(0){}
		explicit StackElement( uint32_t eventid_)
			:eventid(eventid_),program(0),variable(0){}
		StackElement( const StackElement& o)
			:eventid(o.eventid),program(o.program),variable(o.variable){}
	};

private:
	ErrorBufferInterface* m_errorhnd;
	StreamPatternMatchData m_data;
	std::vector<StackElement> m_stack;
	uint32_t m_expression_event_cnt;
};


StreamPatternMatchInstanceInterface* StreamPatternMatch::createInstance() const
{
	try
	{
		return new StreamPatternMatchInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( "failed to create pattern match instance", *m_errorhnd, 0);
}

