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
	SymbolTable termMap;
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
		buf[ tidx] = 0;
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
		:m_errorhnd(errorhnd_),m_data(data_),m_statemachine(&data_->programTable),m_nofEvents(0),m_curPosition(0){}

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
				return m_data->termMap.get( keybuf, keysize);
			}
			else if (type.empty())
			{
				return m_data->termMap.get( value);
			}
			else
			{
				return m_data->termMap.get( utils::tolower(type)+" "+value);
			}
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier: %s", *m_errorhnd, 0);
	}

	virtual void putInput( unsigned int termid, unsigned int ordpos, unsigned int origpos, unsigned int origsize)
	{
		try
		{
			if (m_curPosition > ordpos)
			{
				throw strus::runtime_error(_TXT("term events not fed in ascending order (%u > %u)"), m_curPosition, ordpos);
			}
			else if (m_curPosition < ordpos)
			{
				m_statemachine.setCurrentPos( m_curPosition = ordpos);
			}
			uint32_t eventid = eventHandle( TermEvent, termid);
			EventData data( origpos, origsize, ordpos, eventid, 0/*subdataref*/);
			m_statemachine.doTransition( eventid, data);
			++m_nofEvents;
		}
		CATCH_ERROR_MAP( "failed to feed input to pattern matcher: %s", *m_errorhnd);
	}

	void gatherResultItems( std::vector<PatternMatchResultItem>& resitemlist, uint32_t dataref) const
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

	virtual std::vector<stream::PatternMatchResult> fetchResults() const
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
		CATCH_ERROR_MAP_RETURN( "failed to fetch pattern match result: %s", *m_errorhnd, std::vector<stream::PatternMatchResult>());
	}

	virtual void getStatistics( Statistics& stats) const
	{
		stats.nofPatternsTriggered = m_statemachine.nofPatternsTriggered();
		stats.nofOpenPatterns = m_statemachine.nofOpenPatterns() / m_nofEvents;
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const StreamPatternMatchData* m_data;
	StateMachine m_statemachine;
	unsigned int m_nofEvents;
	unsigned int m_curPosition;
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
				return m_data.termMap.getOrCreate( keybuf, keysize);
			}
			else if (type.empty())
			{
				return m_data.termMap.getOrCreate( value);
			}
			else
			{
				return m_data.termMap.getOrCreate( utils::tolower(type)+" "+value);
			}
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier: %s", *m_errorhnd, 0);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
			uint32_t eventid = eventHandle( TermEvent, termid);
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push term on pattern match expression stack: %s", *m_errorhnd);
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
			if (argc > m_stack.size() || argc > (std::size_t)std::numeric_limits<uint32_t>::max())
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
					slot_initsigval = argc-1;
					--slot_initcount;
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
					--slot_initcount;
					break;
				case OpAny:
					slot_sigtype = Trigger::SigAny;
					slot_initcount = 1;
					break;
			}
			ActionSlotDef actionSlotDef( slot_initsigval, slot_initcount, slot_event, slot_resultHandle);
			uint32_t program = m_data.programTable.createProgram( range_, actionSlotDef);

			std::size_t ai = 0;
			for (; ai != argc; ++ai)
			{
				bool isKeyEvent = false;
				uint32_t trigger_sigval = 0;
				Trigger::SigType trigger_sigtype = slot_sigtype;
				switch (joinop)
				{
					case OpSequenceStruct:
						if (ai == 0)
						{
							//... structure delimiter
							trigger_sigtype = Trigger::SigDel;
						}
						else
						{
							trigger_sigval = argc-ai-1;
							isKeyEvent = (ai == 1);
						}
						break;
					case OpInRangeStruct:
						if (ai == 0)
						{
							//... structure delimiter
							trigger_sigtype = Trigger::SigDel;
						}
						else
						{
							trigger_sigval = 1 << (argc-ai-1);
							isKeyEvent = true;
						}
						break;
					case OpSequence:
						trigger_sigval = argc-ai-1;
						isKeyEvent = (ai == 0);
						break;
					case OpInRange:
						trigger_sigval = 1 << (argc-ai-1);
						isKeyEvent = true;
						break;
					case OpAny:
						isKeyEvent = true;
						break;
				}
				StackElement& elem = m_stack[ m_stack.size() - argc + ai];
				m_data.programTable.createTrigger(
					program, elem.eventid, trigger_sigtype,
					trigger_sigval, elem.variable);
				if (isKeyEvent)
				{
					m_data.programTable.defineEventProgram( elem.eventid, program);
				}
			}
			m_stack.resize( m_stack.size() - argc);
			m_stack.push_back( StackElement( slot_event, program));
		}
		CATCH_ERROR_MAP( "failed to push expression on pattern match expression stack: %s", *m_errorhnd);
	}

	virtual void pushPattern( const std::string& name)
	{
		try
		{
			uint32_t eventid = eventHandle( ReferenceEvent, m_data.patternMap.getOrCreate( name));
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push pattern reference on pattern match expression stack: %s", *m_errorhnd);
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
		CATCH_ERROR_MAP( "failed to attach variable to top element of pattern match expression stack: %s", *m_errorhnd);
	}

	virtual void closePattern( const std::string& name, bool visible)
	{
		try
		{
			if (m_stack.empty())
			{
				throw strus::runtime_error(_TXT("illegal operation close pattern when no node on the stack"));
			}
			StackElement& elem = m_stack.back();
			uint32_t resultHandle = m_data.patternMap.getOrCreate( name);
			uint32_t resultEvent = eventHandle( ReferenceEvent, resultHandle);
			uint32_t program = elem.program;
			if (!program)
			{
				//... atomic event we have to envelope into a program
				ActionSlotDef actionSlotDef( 0, 0, resultEvent, resultHandle);
				program = m_data.programTable.createProgram( 0, actionSlotDef);
				m_data.programTable.createTrigger(
					program, elem.eventid, Trigger::SigAny, 0, elem.variable);
				m_data.programTable.defineEventProgram( elem.eventid, program);
			}
			m_data.programTable.defineProgramResult( program, resultEvent, visible?resultHandle:0);
		}
		CATCH_ERROR_MAP( "failed to close pattern definition on pattern match expression stack: %s", *m_errorhnd);
	}

	virtual StreamPatternMatchContextInterface* createContext() const
	{
		try
		{
			return new StreamPatternMatchContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern match context: %s", *m_errorhnd, 0);
	}

private:
	struct StackElement
	{
		uint32_t eventid;
		uint32_t program;
		uint32_t variable;

		StackElement()
			:eventid(0),program(0),variable(0){}
		explicit StackElement( uint32_t eventid_, uint32_t program_=0)
			:eventid(eventid_),program(program_),variable(0){}
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
	CATCH_ERROR_MAP_RETURN( "failed to create pattern match instance: %s", *m_errorhnd, 0);
}

