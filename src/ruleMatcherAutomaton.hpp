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
#include "podStructArrayBase.hpp"
#include "podStructTableBase.hpp"
#include "podStackPoolBase.hpp"
#include <vector>

namespace strus
{

enum
{
	BaseAddrGroupTable =		(1000000 *  1),
	BaseAddrRuleTable =		(1000000 *  2),
	BaseAddrActionSlotDefTable =	(1000000 *  3),
	BaseAddrTriggerDefTable =	(1000000 *  4),
	BaseAddrProgramTable =		(1000000 *  5),
	BaseAddrActionSlotTable =	(1000000 *  6),
	BaseAddrActionSlotList =	(1000000 *  7),
	BaseAddrEventTriggerList =	(1000000 *  8),
	BaseAddrEventDataReferenceTable=(1000000 *  9),
	BaseAddrGroupList =		(1000000 * 10),
	BaseAddrEventItemList =		(1000000 * 11),
	BaseAddrResultList =		(1000000 * 12)
};

class Trigger
{
public:
	enum SigType {SigAll=0x0,SigSeq=0x1,SigAny=0x2,SigDel=0x3};

	Trigger( uint32_t slot_, SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:m_slot(slot_),m_sigtype(sigtype_),m_variable(variable_),m_sigval(sigval_){}
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
	uint32_t count;
	uint32_t event;
	uint32_t program;
	uint32_t rule;
	uint32_t group;
	uint32_t start_ordpos;
	std::size_t start_bytepos;
	bool isComplete;

	ActionSlot( uint32_t value_, uint32_t count_, uint32_t event_, uint32_t program_, uint32_t rule_, uint32_t group_, bool isComplete_)
		:value(value_),count(count_),event(event_),program(program_),rule(rule_),group(group_),start_ordpos(0),start_bytepos(0),isComplete(isComplete_){}
	ActionSlot( const ActionSlot& o)
		:value(o.value),count(o.count),event(o.event),program(o.program),rule(o.rule),group(o.group),start_ordpos(o.start_ordpos),start_bytepos(o.start_bytepos),isComplete(o.isComplete){}
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

	void getTriggers( PodStructArrayBase<Trigger*,std::size_t,0>& triggers, uint32_t event) const;

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

struct Group
{
	uint32_t actionSlotListIdx;
	uint32_t eventTriggerListIdx;

	Group()
		:actionSlotListIdx(0),eventTriggerListIdx(0){}
	Group( uint32_t actionSlotListIdx_, uint32_t eventTriggerListIdx_)
		:actionSlotListIdx(actionSlotListIdx_),eventTriggerListIdx(eventTriggerListIdx_){}
	Group( const Group& o)
		:actionSlotListIdx(o.actionSlotListIdx),eventTriggerListIdx(o.eventTriggerListIdx){}
};

struct GroupTableFreeListElem {uint32_t _;uint32_t next;};

class GroupTable
	:public PodStructTableBase<Group,uint32_t,GroupTableFreeListElem,BaseAddrGroupTable>
{
public:
	typedef PodStructTableBase<Group,uint32_t,GroupTableFreeListElem,BaseAddrGroupTable> Parent;

	GroupTable(){}
	GroupTable( const GroupTable& o) :Parent(o){}
};

class Rule
{
public:
	uint32_t groupListIdx;
	uint32_t eventDataReferenceIdx;
	uint32_t lastpos;
	bool isComplete;

	Rule()
		:groupListIdx(0),eventDataReferenceIdx(0),lastpos(0),isComplete(false){}
	Rule( uint32_t groupListIdx_, uint32_t eventDataReferenceIdx_, uint32_t lastpos_)
		:groupListIdx(groupListIdx_),eventDataReferenceIdx(eventDataReferenceIdx_),lastpos(lastpos_),isComplete(false){}
	Rule( const Rule& o)
		:groupListIdx(o.groupListIdx),eventDataReferenceIdx(o.eventDataReferenceIdx),lastpos(o.lastpos),isComplete(o.isComplete){}
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
	std::size_t bytepos;
	std::size_t bytesize;
	uint32_t ordpos;
	uint32_t eventid;
	uint32_t subdataref;

	EventData()
		:bytepos(0),bytesize(0),ordpos(0),eventid(0),subdataref(0){}
	EventData( std::size_t bytepos_, std::size_t bytesize_, uint32_t ordpos_, uint32_t eventid_, uint32_t subdataref_)
		:bytepos(bytepos_),bytesize(bytesize_),ordpos(ordpos_),eventid(eventid_),subdataref(subdataref_){}
	EventData( const EventData& o)
		:bytepos(o.bytepos),bytesize(o.bytesize),ordpos(o.ordpos),eventid(o.eventid),subdataref(o.subdataref){}
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
	uint32_t rule;
	uint32_t eventItemListIdx;

	Result( uint32_t rule_, uint32_t eventItemListIdx_)
		:rule(rule_),eventItemListIdx(eventItemListIdx_){}
	Result( const Result& o)
		:rule(o.rule),eventItemListIdx(o.eventItemListIdx){}
};

struct ActionSlotDef
{
	uint32_t initsigval;
	uint32_t initcount;
	uint32_t event;
	uint32_t program;
	bool isComplete;

	ActionSlotDef( uint32_t initsigval_, uint32_t initcount_, uint32_t event_, uint32_t program_, bool isComplete_)
		:initsigval(initsigval_),initcount(initcount_),event(event_),program(program_),isComplete(isComplete_){}
	ActionSlotDef( const ActionSlotDef& o)
		:initsigval(o.initsigval),initcount(o.initcount),event(o.event),program(o.program),isComplete(o.isComplete){}
};

struct TriggerDef
{
	uint32_t event;
	uint32_t slot;
	Trigger::SigType sigtype;
	uint32_t sigval;
	uint32_t variable;

	TriggerDef( uint32_t event_, uint32_t slot_, Trigger::SigType sigtype_, uint32_t sigval_, uint32_t variable_)
		:event(event_),slot(slot_),sigtype(sigtype_),sigval(sigval_),variable(variable_){}
	TriggerDef( const TriggerDef& o)
		:event(o.event),slot(o.slot),sigtype(o.sigtype),sigval(o.sigval),variable(o.variable){}
};

struct Program
{
	uint32_t actionSlotListIdx;
	uint32_t triggerListIdx;
	uint32_t positionRange;

	explicit Program( uint32_t positionRange_)
		:actionSlotListIdx(0),triggerListIdx(0),positionRange(positionRange_){}
	Program( const Program& o)
		:actionSlotListIdx(o.actionSlotListIdx),triggerListIdx(o.triggerListIdx),positionRange(o.positionRange){}
};

struct ProgramTableFreeListElem {uint32_t _;uint32_t next;};

class ProgramTable
{
public:
	typedef PodStackPoolBase<ActionSlotDef,uint32_t,BaseAddrActionSlotDefTable> ActionSlotDefList;
	typedef PodStackPoolBase<TriggerDef,uint32_t,BaseAddrTriggerDefTable> TriggerDefList;

	uint32_t createProgram( uint32_t positionRange_);
	void createSlot( uint32_t program, uint32_t initsigval, uint32_t initcount, uint32_t follow_event, uint32_t follow_program, bool isComplete);
	void createTrigger( uint32_t program, uint32_t event, uint32_t slot, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable);

	const Program& operator[]( uint32_t programidx) const	{return m_programTable[ programidx-1];}
	const ActionSlotDefList& actionSlotList() const		{return m_actionSlotList;}
	const TriggerDefList& triggerList() const		{return m_triggerList;}

private:
	ActionSlotDefList m_actionSlotList;
	TriggerDefList m_triggerList;
	PodStructTableBase<Program,uint32_t,ProgramTableFreeListElem,BaseAddrProgramTable> m_programTable;
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

	uint32_t createRule( uint32_t positionRange);
	uint32_t createGroup( uint32_t rule, uint32_t positionRange);
	uint32_t createSlot( uint32_t rule, uint32_t group, uint32_t initsigval, uint32_t initcount, uint32_t event, uint32_t program, bool isComplete);
	void createTrigger( uint32_t rule, uint32_t group, uint32_t event, uint32_t slot,
				Trigger::SigType sigtype, uint32_t sigval, uint32_t variable);

	void doTransition( uint32_t event, const EventData& data);
	void setCurrentPos( uint32_t pos);
	void installProgram( uint32_t ruleidx, uint32_t programidx);

private:
	void disposeRule( uint32_t rule);
	void deactivateGroup( uint32_t group);
	void disposeEventDataReference( uint32_t eventdataref);
	void referenceEventData( uint32_t eventdataref);
	void appendEventData( uint32_t eventdataref, const EventItem& item);

private:
	const ProgramTable* m_programTable;
	enum {MaxActionSlots=1024};
	EventTriggerTable m_eventTriggerTable;
	ActionSlotTable m_actionSlotTable;
	GroupTable m_groupTable;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrActionSlotList> m_actionSlotList;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrEventTriggerList> m_eventTriggerList;
	PodStackPoolBase<uint32_t,uint32_t,BaseAddrGroupList> m_groupList;
	PodStackPoolBase<EventItem,uint32_t,BaseAddrEventItemList> m_eventItemList;
	EventDataReferenceTable m_eventDataReferenceTable;
	RuleTable m_ruleTable;
	PodStructArrayBase<Result,std::size_t,BaseAddrResultList> m_results;
	uint32_t m_curpos;

	std::vector<DisposeEvent> m_groupDisposeQueue;
	std::vector<DisposeEvent> m_ruleDisposeQueue;
};

} //namespace
#endif

