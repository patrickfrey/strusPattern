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
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "strus/analyzer/patternMatcherResultItem.hpp"
#include "strus/analyzer/patternMatcherResult.hpp"
#include "strus/patternMatcherInstanceInterface.hpp"
#include "strus/patternMatcherContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/base/symbolTable.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/reference.hpp"
#include "strus/lib/pattern_resultformat.hpp"
#include "ruleMatcherAutomaton.hpp"
#include <map>
#include <limits>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

#define STRUS_DBGTRACE_COMPONENT_NAME "pattern"
#define DEBUG_OPEN( NAME) if (m_debugtrace) m_debugtrace->open( NAME);
#define DEBUG_CLOSE() if (m_debugtrace) m_debugtrace->close();
#define DEBUG_EVENT1( NAME, FMT, X1)                            if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1);
#define DEBUG_EVENT2( NAME, FMT, X1, X2)                        if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1, X2);
#define DEBUG_EVENT3( NAME, FMT, X1, X2, X3)                    if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1, X2, X3);
#define DEBUG_EVENT4( NAME, FMT, X1, X2, X3, X4)                if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1, X2, X3, X4);
#define DEBUG_EVENT5( NAME, FMT, X1, X2, X3, X4, X5)            if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1, X2, X3, X4, X5);
#define DEBUG_EVENT7( NAME, FMT, X1, X2, X3, X4, X5, X6, X7)    if (m_debugtrace) m_debugtrace->event( NAME, FMT, X1, X2, X3, X4, X5, X6, X7);

using namespace strus;
using namespace strus::analyzer;

class VariableMap
	:public PatternResultFormatVariableMap, public SymbolTable
{
public:
	VariableMap(){}
	virtual ~VariableMap(){}

	virtual const char* getVariable( const std::string& name) const
	{
		int symid = SymbolTable::get( name);
		if (!symid) return NULL;
		return SymbolTable::key( symid);
	}
	const char* getOrCreateVariable( const std::string& name)
	{
		int symid = SymbolTable::getOrCreate( name);
		if (!symid) return NULL;
		return SymbolTable::key( symid);
	}
};


struct PatternMatcherData
{
	explicit PatternMatcherData( ErrorBufferInterface* errorhnd)
		:variableMap(),patternMap(),programTable(),resultFormatTable(0),resultFormatHandles(),exclusive(false),maxResultSize(100)
	{
		resultFormatTable = new PatternResultFormatTable( errorhnd, &variableMap);
	}

	VariableMap variableMap;
	SymbolTable patternMap;
	ProgramTable programTable;
	PatternResultFormatTable* resultFormatTable;
	std::vector<const PatternResultFormat*> resultFormatHandles;
	bool exclusive;
	unsigned int maxResultSize;

private:
	PatternMatcherData( const PatternMatcherData&){}	///... non copyable
	void operator=( const PatternMatcherData&){}		///... non copyable
};

enum PatternEventType {TermEvent=0, ExpressionEvent=1, ReferenceEvent=2};
static uint32_t eventHandle( PatternEventType type_, uint32_t idx)
{
	if (idx >= (1<<30)) throw strus::runtime_error( "%s",  _TXT("event handle out of range"));
	return idx | ((uint32_t)type_ << 30);
}

class PatternMatcherContext
	:public PatternMatcherContextInterface
{
public:
	PatternMatcherContext( const PatternMatcherData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_)
		,m_debugtrace(0)
		,m_data(data_)
		,m_resultFormatContext(errorhnd_)
		,m_statemachine(0)
		,m_nofEvents(0)
		,m_curPosition(0)
	{
		DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
		if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);
		m_statemachine = new StateMachine( &data_->programTable, m_debugtrace);
	}

	virtual ~PatternMatcherContext()
	{
		if (m_debugtrace) delete m_debugtrace;
		delete m_statemachine;
	}

	virtual void putInput( const analyzer::PatternLexem& term)
	{
		try
		{
			DEBUG_EVENT2( "input", "id=%u ordpos=%u", term.id(), (unsigned int)term.ordpos())
			if (m_curPosition > term.ordpos())
			{
				throw strus::runtime_error(_TXT("term events not fed in ascending order (%u > %u)"), m_curPosition, term.ordpos());
			}
			else if (m_curPosition < term.ordpos())
			{
				m_statemachine->setCurrentPos( m_curPosition = term.ordpos());
			}
			else if (term.origsize() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("term event orig size out of range"));
			}
			else if (term.origseg() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("term event orig segment number out of range"));
			}
			else if (term.origpos() >= (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("term event orig segment byte position out of range"));
			}
			uint32_t eventid = eventHandle( TermEvent, term.id());
			EventData data( term.origseg(), term.origpos(), term.origseg(), term.origpos() + term.origsize(), term.ordpos(), term.ordpos()+1, 0/*subdataref*/, 0/*formathandle*/);
			m_statemachine->doTransition( eventid, data);
			++m_nofEvents;
		}
		CATCH_ERROR_MAP( _TXT("failed to feed input to pattern matcher: %s"), *m_errorhnd);
	}

	void gatherResultItems( std::vector<PatternMatcherResultItem>& resitemlist, uint32_t dataref)
	{
		uint32_t itemList = m_statemachine->getEventDataItemListIdx( dataref);
		const EventItem* item;
		while (0!=(item=m_statemachine->nextResultItem( itemList)))
		{
			const char* itemName = m_data->variableMap.key( item->variable);
			const char* itemValue = 0;
			if (item->data.subdataref && item->data.formathandle)
			{
				const PatternResultFormat* fmt = m_data->resultFormatHandles[ item->data.formathandle-1];
				std::vector<PatternMatcherResultItem> subresitemlist;
				gatherResultItems( subresitemlist, item->data.subdataref);
				itemValue = m_resultFormatContext.map( fmt, subresitemlist.data(), subresitemlist.size());
			}
			PatternMatcherResultItem rtitem( itemName, itemValue, item->data.start_ordpos, item->data.end_ordpos, item->data.start_origseg, item->data.start_origpos, item->data.end_origseg, item->data.end_origpos);
			resitemlist.push_back( rtitem);

			if (item->data.subdataref && !item->data.formathandle)
			{
				gatherResultItems( resitemlist, item->data.subdataref);
			}
		}
	}

	std::vector<bool> getCoveredFlags( const StateMachine::ResultList& results) const
	{
		std::vector<bool> rt( results.size(), false);
		std::size_t ai = 0, ae = results.size();
		for (; ai != ae; ++ai)
		{
			const Result& result = results[ ai];
			std::size_t ni = ai;
			for (; ni != ae; ++ni)
			{
				const Result& follow_result = results[ ni];
				if (follow_result.start_origseg > result.end_origseg
				||  follow_result.start_origpos >= result.end_origpos + m_data->maxResultSize)
				{
					// ... follow_result is not overlapping with end 
					// of the result plus a maximum result length distance,
					// so there are no more left to check for result.
					break;
				}
				if (follow_result.start_origseg <= result.start_origseg
				&&  follow_result.start_origpos <= result.start_origpos
				&&  follow_result.end_origseg >= result.end_origseg
				&&  follow_result.end_origpos >= result.end_origpos)
				{
					// ... result is covered by follow_result
					if (follow_result.end_origseg != result.end_origseg
					||  follow_result.end_origpos != result.end_origpos
					||  follow_result.start_origseg != result.start_origseg
					||  follow_result.start_origpos != result.start_origpos)
					{
						// ... and not equal, so result is 
						// marked to be eliminated
						rt[ ai] = true;
					}
				}
				if (follow_result.start_origseg >= result.start_origseg
				&&  follow_result.start_origpos >= result.start_origpos
				&&  follow_result.end_origseg <= result.end_origseg
				&&  follow_result.end_origpos <= result.end_origpos)
				{
					// ... follow_result is covered by result
					if (follow_result.end_origseg != result.end_origseg
					||  follow_result.end_origpos != result.end_origpos
					||  follow_result.start_origseg != result.start_origseg
					||  follow_result.start_origpos != result.start_origpos)
					{
						// ... and not equal, so follow_result is 
						// marked to be eliminated
						rt[ ni] = true;
					}
				}
			}
		}
		return rt;
	}

	void pushResult( std::vector<analyzer::PatternMatcherResult>& res, const Result& result)
	{
		const char* resultName = m_data->patternMap.key( result.resultHandle);
		std::vector<PatternMatcherResultItem> rtitemlist;
		const char* resultValue = 0;
		if (result.eventDataReferenceIdx)
		{
			if (result.formatHandle)
			{
				const PatternResultFormat* fmt = m_data->resultFormatHandles[ result.formatHandle-1];
				std::vector<PatternMatcherResultItem> subrtitemlist;
				gatherResultItems( subrtitemlist, result.eventDataReferenceIdx);
				resultValue = m_resultFormatContext.map( fmt, subrtitemlist.data(), subrtitemlist.size());
			}
			else
			{
				gatherResultItems( rtitemlist, result.eventDataReferenceIdx);
			}
		}
		DEBUG_EVENT7( "result", "name=%s ordpos=%u ordend=%u start=[%u,%u] end=[%u,%u]", resultName, (unsigned int)result.start_ordpos, (unsigned int)result.end_ordpos, (unsigned int)result.start_origseg, (unsigned int)result.start_origpos, (unsigned int)result.end_origseg, (unsigned int)result.end_origpos);
		res.push_back( PatternMatcherResult( resultName, resultValue, result.start_ordpos, result.end_ordpos, result.start_origseg, result.start_origpos, result.end_origseg, result.end_origpos, rtitemlist));
	}

	virtual std::vector<analyzer::PatternMatcherResult> fetchResults()
	{
		try
		{
			std::vector<analyzer::PatternMatcherResult> rt;
			const StateMachine::ResultList& results = m_statemachine->results();
			rt.reserve( results.size());
			if (m_data->exclusive)
			{
				std::vector<bool> eliminate( getCoveredFlags( results));
				std::size_t ai = 0, ae = results.size();
				for (; ai != ae; ++ai)
				{
					if (!eliminate[ai])
					{
						pushResult( rt, results[ ai]);
					}
				}
			}
			else
			{
				std::size_t ai = 0, ae = results.size();
				for (; ai != ae; ++ai)
				{
					pushResult( rt, results[ ai]);
				}
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
			stats.define( "nofProgramsInstalled", m_statemachine->nofProgramsInstalled());
			stats.define( "nofAltKeyProgramsInstalled", m_statemachine->nofAltKeyProgramsInstalled());
			stats.define( "nofSignalsFired", m_statemachine->nofSignalsFired());
			if (m_nofEvents)
			{
				stats.define( "nofTriggersAvgActive", m_statemachine->nofOpenPatterns() / m_nofEvents);
			}
			return stats;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to get pattern match statistics: %s"), *m_errorhnd, PatternMatcherStatistics());
	}

	virtual void reset()
	{
		try
		{
			StateMachine* new_statemachine = new StateMachine( &m_data->programTable, m_debugtrace);
			delete m_statemachine;
			m_statemachine = new_statemachine;
			m_nofEvents = 0;
			m_curPosition = 0;
		}
		CATCH_ERROR_MAP( _TXT("failed to get reset pattern matcher context: %s"), *m_errorhnd);
	}

private:
	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
	const PatternMatcherData* m_data;
	PatternResultFormatContext m_resultFormatContext;
	StateMachine* m_statemachine;
	unsigned int m_nofEvents;
	unsigned int m_curPosition;
};


/// \brief Interface for building the automaton for detecting patterns in a document stream
class PatternMatcherInstance
	:public PatternMatcherInstanceInterface
{
public:
	explicit PatternMatcherInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_debugtrace(0),m_data(errorhnd_),m_stack(),m_expression_event_cnt(0),m_popt()
	{
		DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
		if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);
	}

	virtual ~PatternMatcherInstance()
	{
		if (m_debugtrace) delete m_debugtrace;
	}

	virtual void defineTermFrequency( unsigned int termid, double df)
	{
		m_data.programTable.defineEventFrequency( eventHandle( TermEvent, termid), df);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
			DEBUG_EVENT2( "term", "id=%d stack=%u", (int)termid, (unsigned int)m_stack.size()+1)
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
			DEBUG_EVENT4( "expression", "op=%d args=%u range=%d cardinality=%d", (int)joinop, (unsigned int)argc, range, cardinality)
			if (range > std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("proximity range value out of range or negative"));
			}
			if (argc > m_stack.size() || argc > (std::size_t)std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("expression references more arguments than nodes on the stack"));
			}
			if (cardinality > (unsigned int)std::numeric_limits<uint32_t>::max())
			{
				throw std::runtime_error( _TXT("illegal value for cardinality"));
			}
			uint32_t slot_initsigval = 0;
			uint32_t slot_initcount = cardinality?(uint32_t)cardinality:(uint32_t)argc;
			uint32_t slot_event = eventHandle( ExpressionEvent, ++m_expression_event_cnt);
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
						throw strus::runtime_error( _TXT("operator '%s': number of arguments %d out of range (%d)"), "within", (int)argc, 32);
					}
					slot_initsigval = 0xffFFffFF;
					break;
				case OpWithinStruct:
					slot_sigtype = Trigger::SigWithin;
					if (argc > 32)
					{
						throw strus::runtime_error( _TXT("operator '%s': number of arguments %d out of range (%d)"), "within_struct", (int)argc, 32);
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
			ActionSlotDef actionSlotDef( slot_initsigval, slot_initcount, slot_event, 0/*resultHandle*/, 0/*formatHandle*/);
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
					trigger_sigval, elem.variable);
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
			DEBUG_EVENT2( "pattern", "name=%s stack=%u", name.c_str(), (unsigned int)m_stack.size())
			uint32_t eventid = eventHandle( ReferenceEvent, m_data.patternMap.getOrCreate( name));
			if (eventid == 0) throw std::runtime_error( _TXT("failed to define pattern symbol"));
			m_stack.push_back( StackElement( eventid));
		}
		CATCH_ERROR_MAP( _TXT("failed to push pattern reference on the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void attachVariable( const std::string& name)
	{
		try
		{
			DEBUG_EVENT1( "variable", "name=%s", name.c_str())
			if (m_stack.empty())
			{
				throw std::runtime_error( _TXT( "illegal operation attach variable when no node on the stack"));
			}
			StackElement& elem = m_stack.back();
			if (elem.variable)
			{
				throw std::runtime_error( _TXT( "more than one variable assignment to a node"));
			}
			elem.variable = m_data.variableMap.getOrCreate( name);
			if (elem.variable == 0)
			{
				throw std::runtime_error( _TXT("failed to define variable symbol"));
			}
		}
		CATCH_ERROR_MAP( _TXT("failed to attach variable to top element of the pattern match expression stack: %s"), *m_errorhnd);
	}

	virtual void definePattern( const std::string& name, const std::string& formatstring, bool visible)
	{
		try
		{
			if (m_stack.empty())
			{
				throw std::runtime_error( _TXT("illegal operation close pattern when no node on the stack"));
			}
			StackElement& elem = m_stack.back();
			uint32_t resultHandle = m_data.patternMap.getOrCreate( name);
			if (resultHandle == 0)
			{
				throw std::runtime_error( _TXT("failed to define result symbol"));
			}
			uint32_t resultEvent = eventHandle( ReferenceEvent, resultHandle);
			uint32_t program = elem.program;
			uint32_t formatHandle = 0;
			if (!formatstring.empty())
			{
				m_data.resultFormatHandles.push_back( m_data.resultFormatTable->createResultFormat( formatstring.c_str()));
				formatHandle = m_data.resultFormatHandles.size();
			}
			if (!program)
			{
				//... atomic event we have to envelope into a program
				ActionSlotDef actionSlotDef( 0/*val*/, 1/*count*/, resultEvent, resultHandle, formatHandle);
				program = m_data.programTable.createProgram( 0, actionSlotDef);
				m_data.programTable.createTrigger(
					program, elem.eventid, true/*isKeyEvent*/, Trigger::SigAny, 0, elem.variable);
				m_data.programTable.doneProgram( program);
			}
			else if (elem.variable)
			{
				throw std::runtime_error( _TXT("variable assignments only allowed to subexpressions of pattern"));
			}
			m_data.programTable.defineProgramResult( program, resultEvent, visible?resultHandle:0, formatHandle);
			DEBUG_EVENT4( "pattern", "name=%s format='%s' visible=%s stack=%u", name.c_str(), formatstring.c_str(), visible?"true":"false", (unsigned int)m_stack.size())
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

	void printAutomatonStatistics( std::ostream& out)
	{
		ProgramTable::Statistics stats = m_data.programTable.getProgramStatistics();
		std::vector<uint32_t>::const_iterator di = stats.keyEventDist.begin(), de = stats.keyEventDist.end();
		out << "occurrence dist:";
		for (int didx=0; di != de; ++di,++didx)
		{
			out << " " << *di;
		}
		out << std::endl;
		out << "stop event list:";
		std::vector<uint32_t>::const_iterator si = stats.stopWordSet.begin(), se = stats.stopWordSet.end();
		for (; si != se; ++si)
		{
			out << " " << *si;
		}
		out << std::endl;
	}

	virtual void defineOption( const std::string& name, double value)
	{
		try
		{
			if (strus::caseInsensitiveEquals( name, "stopwordOccurrenceFactor"))
			{
				m_popt.stopwordOccurrenceFactor = value;
			}
			else if (strus::caseInsensitiveEquals( name, "weightFactor"))
			{
				m_popt.weightFactor = value;
			}
			else if (strus::caseInsensitiveEquals( name, "maxRange"))
			{
				m_popt.maxRange = (unsigned int)(value + std::numeric_limits<double>::epsilon());
			}
			else if (strus::caseInsensitiveEquals( name, "maxResultSize"))
			{
				m_data.maxResultSize = (unsigned int)(value + std::numeric_limits<double>::epsilon());
			}
			else if (strus::caseInsensitiveEquals( name, "exclusive"))
			{
				m_data.exclusive = true;
			}
			else
			{
				throw strus::runtime_error(_TXT("unknown token pattern match option: '%s'"), name.c_str());
			}
		}
		CATCH_ERROR_MAP( _TXT("failed to define pattern matching automaton option: %s"), *m_errorhnd);
	}

	virtual bool compile()
	{
		try
		{
			if (m_debugtrace)
			{
				std::ostringstream out;
				out << "automaton statistics before otimization:" << std::endl;
				printAutomatonStatistics( out);
				std::string outstr( out.str());
				DEBUG_EVENT1( "statistics", "%s", outstr.c_str())
			}
			m_data.programTable.optimize( m_popt);

			if (m_debugtrace)
			{
				std::ostringstream out;
				out << "automaton statistics after otimization:" << std::endl;
				printAutomatonStatistics( out);
				std::string outstr( out.str());
				DEBUG_EVENT1( "statistics", "%s", outstr.c_str())
			}
			return true;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("failed to compile (optimize) pattern matching automaton: %s"), *m_errorhnd, false);
	}

	virtual analyzer::FunctionView view() const
	{
		return analyzer::FunctionView();
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
	DebugTraceContextInterface* m_debugtrace;
	PatternMatcherData m_data;
	std::vector<StackElement> m_stack;
	uint32_t m_expression_event_cnt;
	ProgramTable::OptimizeOptions m_popt;
};


std::vector<std::string> PatternMatcher::getCompileOptionNames() const
{
	std::vector<std::string> rt;
	static const char* ar[] = {"stopwordOccurrenceFactor","weightFactor","maxRange","exclusive","maxResultSize",0};
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


