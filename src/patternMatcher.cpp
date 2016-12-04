/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of an automaton for detecting patterns of tokens in a document stream
/// \file "patternMatcher.cpp"
#include "patternMatcher.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "strus/analyzer/patternMatcherResultItem.hpp"
#include "strus/analyzer/patternMatcherResult.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/symbolTable.hpp"
#include "ruleMatcherAutomaton.hpp"
#include <map>
#include <limits>
#include <vector>
#include <cstring>
#include <iostream>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;
using namespace strus::analyzer;

struct PatternMatcherData
{
	PatternMatcherData(){}

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

class PatternMatcherContext
	:public PatternMatcherContextInterface
{
public:
	PatternMatcherContext( const PatternMatcherData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_),m_statemachine(&data_->programTable),m_nofEvents(0),m_curPosition(0){}

	virtual ~PatternMatcherContext()
	{}

	virtual void putInput( const analyzer::PatternLexem& term)
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
			else if (term.origsize() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("term event orig size out of range"));
			}
			else if (term.origseg() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("term event orig segment number out of range"));
			}
			else if (term.origpos() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("term event orig segment byte position out of range"));
			}
			uint32_t eventid = eventHandle( TermEvent, term.id());
			EventData data( term.origseg(), term.origpos(), term.origseg(), term.origpos() + term.origsize(), term.ordpos(), 0/*subdataref*/);
			m_statemachine.doTransition( eventid, data);
			++m_nofEvents;
		}
		CATCH_ERROR_MAP( _TXT("failed to feed input to pattern matcher: %s"), *m_errorhnd);
	}

	void gatherResultItems( std::vector<PatternMatcherResultItem>& resitemlist, uint32_t dataref) const
	{
		uint32_t itemList = m_statemachine.getEventDataItemListIdx( dataref);
		const EventItem* item;
		while (0!=(item=m_statemachine.nextResultItem( itemList)))
		{
			const char* itemName = m_data->variableMap.key( item->variable);
			PatternMatcherResultItem rtitem( itemName, item->data.ordpos, item->data.start_origseg, item->data.start_origpos, item->data.end_origseg, item->data.end_origpos, item->weight);
			resitemlist.push_back( rtitem);
			if (item->data.subdataref)
			{
				gatherResultItems( resitemlist, item->data.subdataref);
			}
		}
	}

	virtual std::vector<analyzer::PatternMatcherResult> fetchResults() const
	{
		try
		{
			std::vector<analyzer::PatternMatcherResult> rt;
			const StateMachine::ResultList& results = m_statemachine.results();
			rt.reserve( results.size());
			std::size_t ai = 0, ae = results.size();
			for (; ai != ae; ++ai)
			{
				const Result& result = results[ ai];
				const char* resultName = m_data->patternMap.key( result.resultHandle);
				std::vector<PatternMatcherResultItem> rtitemlist;
				if (result.eventDataReferenceIdx)
				{
					gatherResultItems( rtitemlist, result.eventDataReferenceIdx);
				}
				rt.push_back( PatternMatcherResult( resultName, result.ordpos, result.start_origseg, result.start_origpos, result.end_origseg, result.end_origpos, rtitemlist));
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to fetch pattern match result: %s"), *m_errorhnd, std::vector<analyzer::PatternMatcherResult>());
	}

	virtual analyzer::PatternMatcherStatistics getStatistics() const
	{
		try
		{
			PatternMatcherStatistics stats;
			stats.define( "nofProgramsInstalled", m_statemachine.nofProgramsInstalled());
			stats.define( "nofAltKeyProgramsInstalled", m_statemachine.nofAltKeyProgramsInstalled());
			stats.define( "nofSignalsFired", m_statemachine.nofSignalsFired());
			if (m_nofEvents)
			{
				stats.define( "nofTriggersAvgActive", m_statemachine.nofOpenPatterns() / m_nofEvents);
			}
			return stats;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to get pattern match statistics: %s"), *m_errorhnd, PatternMatcherStatistics());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const PatternMatcherData* m_data;
	StateMachine m_statemachine;
	unsigned int m_nofEvents;
	unsigned int m_curPosition;
};

/// \brief Interface for building the automaton for detecting patterns in a document stream
class PatternMatcherInstance
	:public PatternMatcherInstanceInterface
{
public:
	explicit PatternMatcherInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_expression_event_cnt(0){}

	virtual ~PatternMatcherInstance(){}

	virtual void defineTermFrequency( unsigned int termid, double df)
	{
		m_data.programTable.defineEventFrequency( eventHandle( TermEvent, termid), df);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ATM push term " << (int)termid << std::endl;
#endif
			uint32_t eventid = eventHandle( TermEvent, termid);
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( _TXT("failed to push term on the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void pushExpression(
			JoinOperation joinop,
			std::size_t argc, unsigned int range, unsigned int cardinality)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ATM push expression " << (int)joinop << ", args " << argc << ", range " << range << ", cardinality " << cardinality << std::endl;
#endif
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
				case OpSequenceImm:
					slot_sigtype = Trigger::SigSequenceImm;
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
					slot_initcount = cardinality?(uint32_t)cardinality:(uint32_t)1;
					break;
				case OpAnd:
					slot_sigtype = Trigger::SigAnd;
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
					case OpSequenceImm:
						if (ai == 0)
						{
							//... first element has no predecessor
							trigger_sigtype = Trigger::SigSequence;
						}
						trigger_sigval = argc-ai;
						isKeyEvent = (ai == 0);
						break;
					case OpWithin:
						trigger_sigval = 1 << (argc-ai-1);
						isKeyEvent = true;
						break;
					case OpAny:
					case OpAnd:
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
		CATCH_ERROR_MAP( _TXT("failed to push expression on the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void pushPattern( const std::string& name)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ATM push pattern '" << name << "'" << std::endl;
#endif
			uint32_t eventid = eventHandle( ReferenceEvent, m_data.patternMap.getOrCreate( name));
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( _TXT("failed to push pattern reference on the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void attachVariable( const std::string& name, float weight)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ATM attach variable '" << name << "' [" << weight << "]" << std::endl;
#endif
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
		CATCH_ERROR_MAP( _TXT("failed to attach variable to top element of the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void definePattern( const std::string& name, bool visible)
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
				ActionSlotDef actionSlotDef( 0/*val*/, 1/*count*/, resultEvent, resultHandle);
				program = m_data.programTable.createProgram( 0, actionSlotDef);
				m_data.programTable.createTrigger(
					program, elem.eventid, true/*isKeyEvent*/, Trigger::SigAny, 0, elem.variable, elem.weight);
				m_data.programTable.doneProgram( program);
			}
			else if (elem.variable)
			{
				throw strus::runtime_error(_TXT("variable assignments only allowed to subexpressions of pattern"));
			}
			m_data.programTable.defineProgramResult( program, resultEvent, visible?resultHandle:0);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ATM define token pattern '" << name << "'" << (visible?" (visible)":"") << " as program " << program << std::endl;
#endif
		}
		CATCH_ERROR_MAP( _TXT("failed to close pattern definition on the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual PatternMatcherContextInterface* createContext() const
	{
		try
		{
			return new PatternMatcherContext( &m_data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to create pattern match context: %s"), *m_errorhnd, 0);
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

	virtual bool compile( const analyzer::PatternMatcherOptions& opt)
	{
		try
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "automaton statistics before otimization:" << std::endl;
			printAutomatonStatistics();
#endif
			ProgramTable::OptimizeOptions popt;
			analyzer::PatternMatcherOptions::const_iterator oi = opt.begin(), oe = opt.end();
			for (; oi != oe; ++oi)
			{
				if (utils::caseInsensitiveEquals( oi->first, "stopwordOccurrenceFactor"))
				{
					popt.stopwordOccurrenceFactor = oi->second;
				}
				else if (utils::caseInsensitiveEquals( oi->first, "weightFactor"))
				{
					popt.weightFactor = oi->second;
				}
				else if (utils::caseInsensitiveEquals( oi->first, "maxRange"))
				{
					popt.maxRange = (unsigned int)(oi->second + std::numeric_limits<double>::epsilon());
				}
				else
				{
					throw strus::runtime_error(_TXT("unknown token pattern match option: '%s'"), oi->first.c_str());
				}
			}
			m_data.programTable.optimize( popt);

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "automaton statistics after otimization:" << std::endl;
			printAutomatonStatistics();
#endif
			return true;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to compile (optimize) pattern matching automaton: %s"), *m_errorhnd, false);
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
	PatternMatcherData m_data;
	std::vector<StackElement> m_stack;
	uint32_t m_expression_event_cnt;
};


std::vector<std::string> PatternMatcher::getCompileOptions() const
{
	std::vector<std::string> rt;
	static const char* ar[] = {"stopwordOccurrenceFactor","weightFactor","maxRange",0};
	for (std::size_t ai=0; ar[ai]; ++ai)
	{
		rt.push_back( ar[ ai]);
	}
	return rt;
}

PatternMatcherInstanceInterface* PatternMatcher::createInstance() const
{
	try
	{
		return new PatternMatcherInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("failed to create pattern match instance: %s"), *m_errorhnd, 0);
}

const char* PatternMatcher::getDescription() const
{
	return _TXT( "pattern matcher based on an event driven automaton");
}


