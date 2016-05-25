/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a rule matcher automaton
#ifndef _STRUS_RULE_MATCHER_AUTOMATON_HPP_INCLUDED
#define _STRUS_RULE_MATCHER_AUTOMATON_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "utils.hpp"
#include "podStructArrayBase.hpp"
#include "podStructTableBase.hpp"
#include "podStackPoolBase.hpp"
#include <vector>

namespace strus
{

enum
{
	BaseAddrRuleTable =		(10000000 *  1),
	BaseAddrTriggerDefTable =	(10000000 *  2),
	BaseAddrProgramTable =		(10000000 *  3),
	BaseAddrActionSlotTable =	(10000000 *  4),
	BaseAddrEventTriggerList =	(10000000 *  5),
	BaseAddrEventDataReferenceTable=(10000000 *  6),
	BaseAddrEventItemList =		(10000000 *  7),
	BaseAddrProgramList =		(10000000 *  9),
	BaseAddrActionSlotDefTable =	(10000000 * 10)
};

class Trigger
{
public:
	enum SigType {SigAny=0x0,SigSequence=0x1,SigInRange=0x2,SigDel=0x3};

	Trigger( uint32_t slot_, SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:m_slot(slot_),m_sigtype(sigtype_),m_variable(variable_),m_sigval(sigval_)
	{}
	Trigger( const Trigger& o)
		:m_slot(o.m_slot),m_sigtype(o.m_sigtype),m_variable(o.m_variable),m_sigval(o.m_sigval){}
	Trigger( const Trigger& o, uint32_t slot_)
		:m_slot(slot_),m_sigtype(o.m_sigtype),m_variable(o.m_variable),m_sigval(o.m_sigval){}

	uint32_t slot() const		{return m_slot;}
	SigType sigtype() const		{return (SigType)m_sigtype;}
	uint32_t sigval() const		{return (uint32_t)m_sigval;}
	uint32_t variable() const	{return (uint32_t)m_variable;}

private:
	uint32_t m_slot;
	SigType m_sigtype;
	uint32_t m_variable;
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
	uint32_t count;
	uint32_t event;
	uint32_t rule;
	uint32_t resultHandle;
	uint32_t start_ordpos;
	std::size_t start_origpos;

	ActionSlot( uint32_t value_, uint32_t count_, uint32_t event_, uint32_t rule_, uint32_t resultHandle_)
		:value(value_),count(count_),event(event_),rule(rule_),resultHandle(resultHandle_),start_ordpos(0),start_origpos(0){}
	ActionSlot( const ActionSlot& o)
		:value(o.value),count(o.count),event(o.event),rule(o.rule),resultHandle(o.resultHandle),start_ordpos(o.start_ordpos),start_origpos(o.start_origpos){}
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


class EventTriggerTable
{
public:
	EventTriggerTable();
	EventTriggerTable( const EventTriggerTable& o);

	uint32_t add( const EventTrigger& et);
	void remove( uint32_t idx);

	typedef PodStructArrayBase<Trigger*,std::size_t,0> TriggerRefList;
	void getTriggers( TriggerRefList& triggers, uint32_t event) const;
	uint32_t size() const	{return m_size;}

private:
	void expand( uint32_t newallocsize);

private:
	enum {BlockSize=128};
	uint32_t* m_eventAr;
	Trigger* m_triggerAr;
	uint32_t m_allocsize;
	uint32_t m_size;
	uint32_t m_freelistidx;
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
	std::size_t origpos;
	std::size_t origsize;
	uint32_t ordpos;
	uint32_t eventid;
	uint32_t subdataref;

	EventData()
		:origpos(0),origsize(0),ordpos(0),eventid(0),subdataref(0){}
	EventData( std::size_t origpos_, std::size_t origsize_, uint32_t ordpos_, uint32_t eventid_, uint32_t subdataref_)
		:origpos(origpos_),origsize(origsize_),ordpos(ordpos_),eventid(eventid_),subdataref(subdataref_){}
	EventData( const EventData& o)
		:origpos(o.origpos),origsize(o.origsize),ordpos(o.ordpos),eventid(o.eventid),subdataref(o.subdataref){}
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

	Result( uint32_t resultHandle_, uint32_t eventDataReferenceIdx_)
		:resultHandle(resultHandle_),eventDataReferenceIdx(eventDataReferenceIdx_){}
	Result( const Result& o)
		:resultHandle(o.resultHandle),eventDataReferenceIdx(o.eventDataReferenceIdx){}
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
	Trigger::SigType sigtype;
	uint32_t sigval;
	uint32_t variable;

	TriggerDef( uint32_t event_, Trigger::SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:event(event_),sigtype(sigtype_),sigval(sigval_),variable(variable_){}
	TriggerDef( const TriggerDef& o)
		:event(o.event),sigtype(o.sigtype),sigval(o.sigval),variable(o.variable){}
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

struct ProgramTableFreeListElem {uint32_t _;uint32_t next;};


class ProgramTable
{
public:
	typedef PodStackPoolBase<ActionSlotDef,uint32_t,BaseAddrActionSlotDefTable> ActionSlotDefList;
	typedef PodStackPoolBase<TriggerDef,uint32_t,BaseAddrTriggerDefTable> TriggerDefList;

	uint32_t createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_);
	void createTrigger( uint32_t program, uint32_t event, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable);

	const Program& operator[]( uint32_t programidx) const	{return m_programTable[ programidx-1];}
	const TriggerDefList& triggerList() const		{return m_triggerList;}

	void defineProgramResult( uint32_t programidx, uint32_t eventid, uint32_t resultHandle);

	void defineEventProgram( uint32_t eventid, uint32_t programidx);
	uint32_t getEventProgramList( uint32_t eventid) const;
	bool nextProgram( uint32_t& programlist, uint32_t& program) const;

private:
	ActionSlotDefList m_actionSlotArray;
	TriggerDefList m_triggerList;
	PodStructTableBase<Program,uint32_t,ProgramTableFreeListElem,BaseAddrProgramTable> m_programTable;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrProgramList> programList;
	utils::UnorderedMap<uint32_t,uint32_t> eventProgamMap;
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
	explicit StateMachine( const ProgramTable* programTable_);
	StateMachine( const StateMachine& o);

	void doTransition( uint32_t event, const EventData& data);
	void setCurrentPos( uint32_t pos);
	void installProgram( uint32_t programidx);

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

public://getStatistics
	unsigned int nofPatternsTriggered() const	{return m_nofPatternsTriggered;}
	double nofOpenPatterns() const			{return m_nofOpenPatterns;}

private:
	uint32_t createRule( uint32_t positionRange);
	void disposeRule( uint32_t rule);
	void deactivateRule( uint32_t rule);
	void disposeEventDataReference( uint32_t eventdataref);
	void referenceEventData( uint32_t eventdataref);
	uint32_t createEventData();
	void appendEventData( uint32_t eventdataref, const EventItem& item);
	void joinEventData( uint32_t eventdataref_dest, uint32_t eventdataref_src);

private:
	const ProgramTable* m_programTable;
	EventTriggerTable m_eventTriggerTable;
	ActionSlotTable m_actionSlotTable;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrEventTriggerList> m_eventTriggerList;
	PodStackPoolBase<EventItem,uint32_t,BaseAddrEventItemList> m_eventItemList;
	EventDataReferenceTable m_eventDataReferenceTable;
	RuleTable m_ruleTable;
	ResultList m_results;
	uint32_t m_curpos;
	std::vector<DisposeEvent> m_ruleDisposeQueue;
	unsigned int m_nofPatternsTriggered;
	double m_nofOpenPatterns;
};

} //namespace
#endif

