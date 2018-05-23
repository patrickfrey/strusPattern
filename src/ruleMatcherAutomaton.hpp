/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a rule matcher automaton
#ifndef _STRUS_RULE_MATCHER_AUTOMATON_HPP_INCLUDED
#define _STRUS_RULE_MATCHER_AUTOMATON_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "strus/base/unordered_map.hpp"
#include "strus/debugTraceInterface.hpp"
#include "podStructArrayBase.hpp"
#include "podStructTableBase.hpp"
#include "podStackPoolBase.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <stdexcept>

namespace strus
{

enum
{
	BaseAddrRuleTable =		(10000000 *  1),
	BaseAddrTriggerDefTable =	(10000000 *  2),
	BaseAddrProgramTable =		(10000000 *  3),
	BaseAddrActionSlotTable =	(10000000 *  4),
	BaseAddrLinkedTriggerTable =	(10000000 *  5),
	BaseAddrEventTriggerList =	(10000000 *  6),
	BaseAddrEventDataReferenceTable=(10000000 *  7),
	BaseAddrEventItemList =		(10000000 *  8),
	BaseAddrEventStructList =	(10000000 *  9),
	BaseAddrProgramList =		(10000000 * 10),
	BaseAddrActionSlotDefTable =	(10000000 * 12),
	BaseAddrDisposeRuleList =	(10000000 * 13),
	BaseAddrDisposeEventList =	(10000000 * 14)
};

class Trigger
{
public:
	enum SigType {SigAny=0x0,SigSequence=0x1,SigSequenceImm=0x2,SigWithin=0x3,SigDel=0x4,SigAnd=0x5};
	static const char* sigTypeName( SigType i)
	{
		static const char* ar[] = {"Any","Sequence","SequenceImm","Within","Del","And"};
		return ar[i];
	}

	Trigger( uint32_t slot_, SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:m_slot(slot_),m_sigtype(sigtype_),m_variable(variable_),m_sigval(sigval_)
	{
		if (variable_ > MaxVariableId) throw std::runtime_error( _TXT("too many variables defined"));
	}
	Trigger( const Trigger& o)
		:m_slot(o.m_slot),m_sigtype(o.m_sigtype),m_variable(o.m_variable),m_sigval(o.m_sigval){}

	uint32_t slot() const		{return m_slot;}
	SigType sigtype() const		{return (SigType)m_sigtype;}
	uint32_t sigval() const		{return (uint32_t)m_sigval;}
	uint32_t variable() const	{return (uint32_t)m_variable;}

private:
	enum {MaxVariableId=(1<<28)-1};
	uint32_t m_slot;
	unsigned int m_sigtype:4;
	unsigned int m_variable:28;
	uint32_t m_sigval;
};

struct EventTrigger
{
	EventTrigger( uint32_t event_, const Trigger& trigger_)
		:event(event_),trigger(trigger_){}
	EventTrigger( const EventTrigger& o)
		:event(o.event),trigger(o.trigger){}

	uint32_t event;
	Trigger trigger;
};

struct ActionSlot
{
	uint32_t value;
	uint32_t event;
	uint32_t rule;
	uint32_t resultHandle;
	uint32_t start_ordpos;
	uint32_t end_ordpos;
	uint32_t start_origseg;
	uint32_t start_origpos;
	uint16_t count;

	ActionSlot( uint32_t value_, uint16_t count_, uint32_t event_, uint32_t rule_, uint32_t resultHandle_)
		:value(value_),event(event_),rule(rule_),resultHandle(resultHandle_),start_ordpos(0),end_ordpos(0),start_origseg(0),start_origpos(0),count(count_){}
	ActionSlot( const ActionSlot& o)
		:value(o.value),event(o.event),rule(o.rule),resultHandle(o.resultHandle),start_ordpos(o.start_ordpos),end_ordpos(o.end_ordpos),start_origseg(o.start_origseg),start_origpos(o.start_origpos),count(o.count){}
};

struct ActionSlotTableFreeListElem {uint32_t _;uint32_t next;};

class ActionSlotTable
	:public PodStructTableBase<ActionSlot,uint32_t,ActionSlotTableFreeListElem,BaseAddrActionSlotTable>
{
public:
	typedef PodStructTableBase<ActionSlot,uint32_t,ActionSlotTableFreeListElem,BaseAddrActionSlotTable> Parent;

	ActionSlotTable(){}
	ActionSlotTable( const ActionSlotTable& o) :Parent(o){}
};

struct LinkedTrigger
{
	LinkedTrigger( uint32_t link_, const Trigger& trigger_)
		:link(link_),trigger(trigger_){}
	LinkedTrigger( const LinkedTrigger& o)
		:link(o.link),trigger(o.trigger){}

	uint32_t link;
	Trigger trigger;
};

struct LinkedTriggerTableFreeListElem {uint32_t _[3]; uint32_t next;};
typedef PodStructTableBase<LinkedTrigger,uint32_t,LinkedTriggerTableFreeListElem,BaseAddrLinkedTriggerTable> LinkedTriggerTable;

class EventTriggerTable
{
public:
	~EventTriggerTable(){}
	EventTriggerTable();
	EventTriggerTable( const EventTriggerTable& o);

	uint32_t add( const EventTrigger& et);
	void remove( uint32_t idx);
	uint32_t getTriggerEventId( uint32_t idx) const;
	Trigger const* getTriggerPtr( uint32_t idx) const;

	typedef PodStructArrayBase<Trigger const*,std::size_t,0> TriggerRefList;
	void getTriggers( TriggerRefList& triggers, uint32_t event) const;
	uint32_t nofTriggers() const			{return m_nofTriggers;}
	void clear();

public:
	enum {BlockSize=1024,EventHashTabSize=16,EventHashTabIdxShift=28,EventHashTabIdxMask=15};
private:
	void expandEventAr( uint32_t htidx, uint32_t newallocsize);
private:
	struct TriggerInd
	{
		uint32_t* m_eventAr;
		uint32_t* m_ar;
		uint32_t m_allocsize;
		uint32_t m_size;

		TriggerInd() :m_eventAr(0),m_ar(0),m_allocsize(0),m_size(0){}
		~TriggerInd();
		void expand( uint32_t newallocsize);
		void clear();
	};
	TriggerInd m_triggerIndAr[ EventHashTabSize];
	LinkedTriggerTable m_triggerTab;
	uint32_t m_nofTriggers;
};

class Rule
{
public:
	uint32_t actionSlotIdx;
	uint32_t eventTriggerListIdx;
	uint32_t eventDataReferenceIdx;
	unsigned int done:1;
	unsigned int lastpos:31;

	explicit Rule( uint32_t lastpos_=0)
		:actionSlotIdx(0),eventTriggerListIdx(0),eventDataReferenceIdx(0),done(0),lastpos(lastpos_){}
	Rule( const Rule& o)
		:actionSlotIdx(o.actionSlotIdx),eventTriggerListIdx(o.eventTriggerListIdx),eventDataReferenceIdx(o.eventDataReferenceIdx),done(o.done),lastpos(o.lastpos){}

	bool isActive() const	{return actionSlotIdx!=0;}
};

struct RuleTableFreeListElem {uint32_t _;uint32_t next;};

class RuleTable
	:public PodStructTableBase<Rule,uint32_t,RuleTableFreeListElem,BaseAddrRuleTable>
{
public:
	typedef PodStructTableBase<Rule,uint32_t,RuleTableFreeListElem,BaseAddrRuleTable> Parent;

	RuleTable(){}
	RuleTable( const RuleTable& o) :Parent(o){}
};

struct EventData
{
	uint32_t start_origseg;
	uint32_t end_origseg;
	uint32_t start_origpos;
	uint32_t end_origpos;
	uint32_t start_ordpos;
	uint32_t end_ordpos;
	uint32_t subdataref;

	EventData()
		:start_origseg(0),end_origseg(0),start_origpos(0),end_origpos(0),start_ordpos(0),end_ordpos(0),subdataref(0){}
	EventData( uint32_t start_origseg_, uint32_t start_origpos_, uint32_t end_origseg_, uint32_t end_origpos_, uint32_t start_ordpos_, uint32_t end_ordpos_, uint32_t subdataref_)
		:start_origseg(start_origseg_),end_origseg(end_origseg_),start_origpos(start_origpos_),end_origpos(end_origpos_),start_ordpos(start_ordpos_),end_ordpos(end_ordpos_),subdataref(subdataref_){}
	EventData( const EventData& o)
		:start_origseg(o.start_origseg),end_origseg(o.end_origseg),start_origpos(o.start_origpos),end_origpos(o.end_origpos),start_ordpos(o.start_ordpos),end_ordpos(o.end_ordpos),subdataref(o.subdataref){}
};

struct EventStruct
{
	EventData data;
	uint32_t eventid;

	EventStruct()
		:data(),eventid(0){}
	EventStruct( const EventData& data_, uint32_t eventid_)
		:data(data_),eventid(eventid_){}
	EventStruct( const EventStruct& o)
		:data(o.data),eventid(o.eventid){}
};
typedef PodStructArrayBase<EventStruct,std::size_t,BaseAddrEventStructList> EventStructList;

struct EventLog
{
	EventData data;
	unsigned int timestmp;

	EventLog()
		:data(),timestmp(0){}
	EventLog( const EventData& data_, unsigned int timestmp_)
		:data(data_),timestmp(timestmp_){}
	EventLog( const EventLog& o)
		:data(o.data),timestmp(o.timestmp){}
};

struct EventDataReference
{
	uint32_t eventItemListIdx;
	uint32_t referenceCount;

	EventDataReference()
		:eventItemListIdx(0),referenceCount(0){}
	EventDataReference( uint32_t eventItemListIdx_, uint32_t referenceCount_)
		:eventItemListIdx(eventItemListIdx_),referenceCount(referenceCount_){}
	EventDataReference( const EventDataReference& o)
		:eventItemListIdx(o.eventItemListIdx),referenceCount(o.referenceCount){}
};
struct EventDataReferenceTableFreeListElem {uint32_t _;uint32_t next;};
typedef PodStructTableBase<EventDataReference,uint32_t,EventDataReferenceTableFreeListElem,BaseAddrEventDataReferenceTable> EventDataReferenceTable;

struct EventItem
{
	uint32_t variable;
	EventData data;

	EventItem( uint32_t variable_, const EventData& data_)
		:variable(variable_),data(data_){}
	EventItem( const EventItem& o)
		:variable(o.variable),data(o.data){}
};

struct Result
{
	uint32_t resultHandle;
	uint32_t eventDataReferenceIdx;
	uint32_t start_ordpos;
	uint32_t end_ordpos;
	uint32_t start_origseg;
	uint32_t end_origseg;
	uint32_t start_origpos;
	uint32_t end_origpos;

	Result( uint32_t resultHandle_, uint32_t eventDataReferenceIdx_, uint32_t start_ordpos_, uint32_t end_ordpos_, uint32_t start_origseg_, uint32_t start_origpos_, uint32_t end_origseg_, uint32_t end_origpos_)
		:resultHandle(resultHandle_),eventDataReferenceIdx(eventDataReferenceIdx_),start_ordpos(start_ordpos_),end_ordpos(end_ordpos_),start_origseg(start_origseg_),end_origseg(end_origseg_),start_origpos(start_origpos_),end_origpos(end_origpos_){}
	Result( const Result& o)
		:resultHandle(o.resultHandle),eventDataReferenceIdx(o.eventDataReferenceIdx),start_ordpos(o.start_ordpos),end_ordpos(o.end_ordpos),start_origseg(o.start_origseg),end_origseg(o.end_origseg),start_origpos(o.start_origpos),end_origpos(o.end_origpos){}
};

struct ActionSlotDef
{
	uint32_t initsigval;
	uint32_t initcount;
	uint32_t event;
	uint32_t resultHandle;

	ActionSlotDef( uint32_t initsigval_, uint32_t initcount_, uint32_t event_, uint32_t resultHandle_)
		:initsigval(initsigval_),initcount(initcount_),event(event_),resultHandle(resultHandle_){}
	ActionSlotDef( const ActionSlotDef& o)
		:initsigval(o.initsigval),initcount(o.initcount),event(o.event),resultHandle(o.resultHandle){}
};

struct TriggerDef
{
	uint32_t event;
	unsigned char isKeyEvent;
	unsigned char sigtype;
	uint32_t sigval;
	uint32_t variable;

	TriggerDef( uint32_t event_, bool isKeyEvent_, Trigger::SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:event(event_),isKeyEvent((unsigned char)isKeyEvent_),sigtype((unsigned char)sigtype_),sigval(sigval_),variable(variable_){}
	TriggerDef( const TriggerDef& o)
		:event(o.event),isKeyEvent(o.isKeyEvent),sigtype(o.sigtype),sigval(o.sigval),variable(o.variable){}
};

struct Program
{
	ActionSlotDef slotDef;
	uint32_t triggerListIdx;
	uint32_t positionRange;

	Program( uint32_t positionRange_, const ActionSlotDef& slotDef_)
		:slotDef(slotDef_),triggerListIdx(0),positionRange(positionRange_){}
	Program( const Program& o)
		:slotDef(o.slotDef),triggerListIdx(o.triggerListIdx),positionRange(o.positionRange){}
};

struct ProgramTrigger
{
	uint32_t programidx;
	uint32_t past_eventid;

	ProgramTrigger( uint32_t programidx_, uint32_t past_eventid_)
		:programidx(programidx_),past_eventid(past_eventid_){}
	ProgramTrigger( const ProgramTrigger& o)
		:programidx(o.programidx),past_eventid(o.past_eventid){}
};

struct ProgramTableFreeListElem {uint32_t _;uint32_t next;};

class ProgramTable
{
public:
	ProgramTable()
		:m_totalNofPrograms(0){}

	typedef PodStackPoolBase<ActionSlotDef,uint32_t,BaseAddrActionSlotDefTable> ActionSlotDefList;
	typedef PodStackPoolBase<TriggerDef,uint32_t,BaseAddrTriggerDefTable> TriggerDefList;

	void defineEventFrequency( uint32_t eventid, double df);

	uint32_t createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_);
	void createTrigger( uint32_t program, uint32_t event, bool isKeyEvent, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable);
	void doneProgram( uint32_t program);

	const Program& operator[]( uint32_t programidx) const	{return m_programMap[ programidx-1];}
	const TriggerDefList& triggerList() const		{return m_triggerList;}

	void defineProgramResult( uint32_t programidx, uint32_t eventid, uint32_t resultHandle);

	uint32_t getEventProgramList( uint32_t eventid) const;
	const ProgramTrigger* nextProgramPtr( uint32_t& programlist) const;

	struct OptimizeOptions
	{
		float stopwordOccurrenceFactor;
		float weightFactor;
		uint32_t maxRange;
	
		OptimizeOptions()		
			:stopwordOccurrenceFactor(0.01f),weightFactor(10.0f),maxRange(5){}
		OptimizeOptions( const OptimizeOptions& o)
			:stopwordOccurrenceFactor(o.stopwordOccurrenceFactor),weightFactor(o.weightFactor),maxRange(o.maxRange){}
	};
	void optimize( OptimizeOptions& opt);

	struct Statistics
	{
		std::vector<uint32_t> keyEventDist;
		std::vector<uint32_t> stopWordSet;
	};

	Statistics getProgramStatistics() const;
	bool isStopWord( uint32_t eventid) const		{return m_stopWordSet.find(eventid) != m_stopWordSet.end();}

private:
	void defineEventProgramAlt( uint32_t eventid, uint32_t programidx, uint32_t past_eventid);
	double calcEventWeight( uint32_t eventid) const;
	uint32_t getAltEventId( uint32_t eventid, uint32_t triggerListIdx) const;
	void getDelimTokenStopWordSet( uint32_t triggerListIdx);
	void defineEventProgram( uint32_t eventid, uint32_t programidx);
	void defineDisposeRule( uint32_t pos, uint32_t ruleidx);
	void eliminateUnusedEvents();

private:
	ActionSlotDefList m_actionSlotArray;
	TriggerDefList m_triggerList;
	typedef PodStructTableBase<Program,uint32_t,ProgramTableFreeListElem,BaseAddrProgramTable> ProgramMap;
	ProgramMap m_programMap;
	PodStackPoolBase<ProgramTrigger,uint32_t,BaseAddrProgramList> m_programTriggerList;
	typedef strus::unordered_map<uint32_t,uint32_t> EventProgamTriggerMap;
	EventProgamTriggerMap m_eventProgamTriggerMap;
	std::set<uint32_t> m_stopWordSet;
	typedef std::map<uint32_t,uint32_t> EventOccurrenceMap;
	EventOccurrenceMap m_keyOccurrenceMap;
	EventOccurrenceMap m_eventOccurrenceMap;
	typedef std::map<uint32_t,double> FrequencyMap;
	FrequencyMap m_frequencyMap;
	uint32_t m_totalNofPrograms;
};

struct DisposeEvent
{
	uint32_t pos;
	uint32_t idx;

	DisposeEvent( uint32_t pos_, uint32_t idx_)
		:pos(pos_),idx(idx_){}
	DisposeEvent( const DisposeEvent& o)
		:pos(o.pos),idx(o.idx){}

	bool operator<( const DisposeEvent& o) const
	{
		return pos > o.pos;
	}
};

class StateMachine
{
public:
	StateMachine( const ProgramTable* programTable_, DebugTraceContextInterface* debugtrace_);
	StateMachine( const StateMachine& o);

	void addObserveEvent( uint32_t event);
	bool isObservedEvent( uint32_t event) const;

	void doTransition( uint32_t event, const EventData& data);
	void setCurrentPos( uint32_t pos);

	typedef PodStructArrayBase<Result,std::size_t,0> ResultList;
	const ResultList& results() const
	{
		return m_results;
	}
	uint32_t getEventDataItemListIdx( uint32_t dataref) const
	{
		return m_eventDataReferenceTable[ dataref].eventItemListIdx;
	}
	const EventItem* nextResultItem( uint32_t& list) const
	{
		return m_eventItemList.nextptr( list);
	}
	void clear();

public://getStatistics
	unsigned int nofProgramsInstalled() const	{return m_nofProgramsInstalled;}
	unsigned int nofAltKeyProgramsInstalled() const	{return m_nofAltKeyProgramsInstalled;}
	unsigned int nofSignalsFired() const		{return m_nofSignalsFired;}
	double nofOpenPatterns() const			{return m_nofOpenPatterns;}

private:
	typedef PodStructArrayBase<uint32_t,std::size_t,BaseAddrDisposeRuleList> DisposeRuleList;
	void fireSignal( ActionSlot& slot, const Trigger& trigger, const EventData& data,
				DisposeRuleList& disposeRuleList, EventStructList& followList);
	uint32_t createRule( uint32_t expiryOrdpos);
	void disposeRule( uint32_t rule);
	void deactivateRule( uint32_t rule);
	void disposeEventDataReference( uint32_t eventdataref);
	void referenceEventData( uint32_t eventdataref);
	uint32_t createEventData();
	void appendEventData( uint32_t eventdataref, const EventItem& item);
	void joinEventData( uint32_t eventdataref_dest, uint32_t eventdataref_src);
	void replayPastEvent( uint32_t eventid, const Rule& rule, uint32_t positionRange);
	void installProgram( uint32_t keyevent, const ProgramTrigger& programTrigger, const EventData& data, EventStructList& followList, DisposeRuleList& disposeRuleList);
	void installEventPrograms( uint32_t keyevent, const EventData& data, EventStructList& followList, DisposeRuleList& disposeRuleList);
	void defineDisposeRule( uint32_t pos, uint32_t ruleidx);

private:
	DebugTraceContextInterface* m_debugtrace;
	const ProgramTable* m_programTable;
	EventTriggerTable m_eventTriggerTable;
	ActionSlotTable m_actionSlotTable;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrEventTriggerList> m_eventTriggerList;
	PodStackPoolBase<EventItem,uint32_t,BaseAddrEventItemList> m_eventItemList;
	EventDataReferenceTable m_eventDataReferenceTable;
	RuleTable m_ruleTable;
	ResultList m_results;
	uint32_t m_curpos;
	enum {DisposeWindowSize=64};
	uint32_t m_disposeWindow[ DisposeWindowSize];
	typedef PodStackPoolBase<uint32_t,uint32_t,BaseAddrDisposeEventList> DisposeEventList;
	DisposeEventList m_disposeRuleList;
	std::vector<DisposeEvent> m_ruleDisposeQueue;
	std::map<uint32_t,EventLog> m_stopWordsEventLogMap;
	unsigned int m_nofProgramsInstalled;
	unsigned int m_nofAltKeyProgramsInstalled;
	unsigned int m_nofSignalsFired;
	double m_nofOpenPatterns;
	unsigned int m_timestmp;
	enum {MaxNofObserveEvents=8};
	uint32_t m_observeEvents[ MaxNofObserveEvents];
};

} //namespace
#endif

