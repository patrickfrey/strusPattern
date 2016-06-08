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
#include <iostream>

#undef STRUS_LOWLEVEL_DEBUG

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
	if (idx >= (1<<30)) throw strus::runtime_error( _TXT("event handle out of range"));
	return idx | ((uint32_t)type_ << 30);
}

class StreamPatternMatchContext
	:public StreamPatternMatchContextInterface
{
public:
	StreamPatternMatchContext( const StreamPatternMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_statemachine(&data_->programTable),m_nofEvents(0),m_curPosition(0){}

	virtual ~StreamPatternMatchContext()
	{}

	virtual void putInput( const PatternMatchTerm& term)
	{
		try
		{
			if (m_curPosition > term.ordpos())
			{
				throw strus::runtime_error(_TXT("term events not fed in ascending order (%u > %u)"), m_curPosition, term.ordpos());
			}
			else if (m_curPosition < term.ordpos())
			{
				m_statemachine.setCurrentPos( m_curPosition = term.ordpos());
			}
			else if (term.origpos() + term.origsize() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("term event orig position and length out of range"));
			}
			uint32_t eventid = eventHandle( TermEvent, term.id());
			EventData data( term.origpos(), term.origsize(), term.ordpos(), 0/*subdataref*/);
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
			PatternMatchResultItem rtitem( itemName, item->data.ordpos, item->data.origpos, item->data.origsize, item->weight);
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
			rt.reserve( results.size());
			std::size_t ai = 0, ae = results.size();
			for (; ai != ae; ++ai)
			{
				const Result& result = results[ ai];
				const char* resultName = m_data->patternMap.key( result.resultHandle);
				std::vector<PatternMatchResultItem> rtitemlist;
				if (result.eventDataReferenceIdx)
				{
					gatherResultItems( rtitemlist, result.eventDataReferenceIdx);
				}
				rt.push_back( PatternMatchResult( resultName, rtitemlist));
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( "failed to fetch pattern match result: %s", *m_errorhnd, std::vector<stream::PatternMatchResult>());
	}

	virtual PatternMatchStatistics getStatistics() const
	{
		try
		{
			PatternMatchStatistics stats;
			stats.define( "nofProgramsInstalled", m_statemachine.nofProgramsInstalled());
			stats.define( "nofAltKeyProgramsInstalled", m_statemachine.nofAltKeyProgramsInstalled());
			stats.define( "nofTriggersFired", m_statemachine.nofTriggersFired());
			stats.define( "nofTriggersAvgActive", m_statemachine.nofOpenPatterns() / m_nofEvents);
			return stats;
		}
		CATCH_ERROR_MAP_RETURN( "failed to get pattern match statistics: %s", *m_errorhnd, PatternMatchStatistics());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const StreamPatternMatchData* m_data;
	StateMachine m_statemachine;
	unsigned int m_nofEvents;
	unsigned int m_curPosition;
};

/// \brief Interface for building the automaton for detecting patterns in a document stream
class StreamPatternMatchInstance
	:public StreamPatternMatchInstanceInterface
{
public:
	explicit StreamPatternMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_expression_event_cnt(0){}

	virtual ~StreamPatternMatchInstance(){}

	virtual void defineTermFrequency( unsigned int termid, double df)
	{
		m_data.programTable.defineEventFrequency( eventHandle( TermEvent, termid), df);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
			uint32_t eventid = eventHandle( TermEvent, termid);
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push term on the pattern match expression stack: %s", *m_errorhnd);
	}

	virtual void pushExpression(
			JoinOperation joinop,
			std::size_t argc, unsigned int range, unsigned int cardinality)
	{
		try
		{
			if (range > std::numeric_limits<uint32_t>::max())
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
				case OpWithin:
					slot_sigtype = Trigger::SigWithin;
					if (argc > 32)
					{
						throw strus::runtime_error(_TXT("number of arguments out of range"));
					}
					slot_initsigval = 0xffFFffFF;
					break;
				case OpWithinStruct:
					slot_sigtype = Trigger::SigWithin;
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
			uint32_t program = m_data.programTable.createProgram( range, actionSlotDef);

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
							trigger_sigval = argc-ai;
							isKeyEvent = (ai == 1);
						}
						break;
					case OpWithinStruct:
						if (ai == 0)
						{
							//... structure delimiter
							trigger_sigtype = Trigger::SigDel;
						}
						else
						{
							trigger_sigval = 1 << (argc-ai);
							isKeyEvent = true;
						}
						break;
					case OpSequence:
						trigger_sigval = argc-ai;
						isKeyEvent = (ai == 0);
						break;
					case OpWithin:
						trigger_sigval = 1 << (argc-ai-1);
						isKeyEvent = true;
						break;
					case OpAny:
						isKeyEvent = true;
						break;
				}
				StackElement& elem = m_stack[ m_stack.size() - argc + ai];
				m_data.programTable.createTrigger(
					program, elem.eventid, isKeyEvent, trigger_sigtype,
					trigger_sigval, elem.variable, elem.weight);
			}
			m_data.programTable.doneProgram( program);
			m_stack.resize( m_stack.size() - argc);
			m_stack.push_back( StackElement( slot_event, program));
		}
		CATCH_ERROR_MAP( "failed to push expression on the pattern match expression stack: %s", *m_errorhnd);
	}

	virtual void pushPattern( const std::string& name)
	{
		try
		{
			uint32_t eventid = eventHandle( ReferenceEvent, m_data.patternMap.getOrCreate( name));
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( "failed to push pattern reference on the pattern match expression stack: %s", *m_errorhnd);
	}

	virtual void attachVariable( const std::string& name, float weight)
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
			elem.variable = m_data.variableMap.getOrCreate( name);
			elem.weight = weight;
		}
		CATCH_ERROR_MAP( "failed to attach variable to top element of the pattern match expression stack: %s", *m_errorhnd);
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
					program, elem.eventid, true/*isKeyEvent*/, Trigger::SigAny, 0, elem.variable, elem.weight);
				m_data.programTable.doneProgram( program);
			}
			m_data.programTable.defineProgramResult( program, resultEvent, visible?resultHandle:0);
		}
		CATCH_ERROR_MAP( "failed to close pattern definition on the pattern match expression stack: %s", *m_errorhnd);
	}

	virtual StreamPatternMatchContextInterface* createContext() const
	{
		try
		{
			return new StreamPatternMatchContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern match context: %s", *m_errorhnd, 0);
	}

#ifdef STRUS_LOWLEVEL_DEBUG
	void printAutomatonStatistics()
	{
		ProgramTable::Statistics stats = m_data.programTable.getProgramStatistics();
		std::vector<uint32_t>::const_iterator di = stats.keyEventDist.begin(), de = stats.keyEventDist.end();
		std::cout << "occurrence dist:";
		for (int didx=0; di != de; ++di,++didx)
		{
			std::cout << " " << *di;
		}
		std::cout << std::endl;
		std::cout << "stop event list:";
		std::vector<uint32_t>::const_iterator si = stats.stopWordSet.begin(), se = stats.stopWordSet.end();
		for (; si != se; ++si)
		{
			std::cout << " " << *si;
		}
		std::cout << std::endl;
	}
#endif

	virtual void optimize( const OptimizeOptions& opt)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "automaton statistics before otimization:" << std::endl;
			printAutomatonStatistics();
#endif
			ProgramTable::OptimizeOptions popt;
			popt.stopwordOccurrenceFactor = opt.stopwordOccurrenceFactor;
			popt.weightFactor = opt.weightFactor;
			popt.maxRange = opt.maxRange;
			m_data.programTable.optimize( popt);

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "automaton statistics after otimization:" << std::endl;
			printAutomatonStatistics();
#endif
		}
		CATCH_ERROR_MAP( "failed to optimize pattern matching automaton: %s", *m_errorhnd);
	}

private:
	struct StackElement
	{
		uint32_t eventid;
		uint32_t program;
		uint32_t variable;
		float weight;

		StackElement()
			:eventid(0),program(0),variable(0),weight(0.0f){}
		explicit StackElement( uint32_t eventid_, uint32_t program_=0)
			:eventid(eventid_),program(program_),variable(0),weight(0.0f){}
		StackElement( const StackElement& o)
			:eventid(o.eventid),program(o.program),variable(o.variable),weight(o.weight){}
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

