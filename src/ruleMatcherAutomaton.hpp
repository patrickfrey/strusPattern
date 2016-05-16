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

namespace strus
{

class Trigger
{
public:
	enum SigType {SigAll=0x0,SigSeq=0x1,SigAny=0x2,SigDel=0x3};

	Trigger( uint32_t slot_, SigType sigtype_, uint32_t sigval_, float weight_=1.0)
		:m_slot(slot_),m_sigtype(sigtype_),m_sigval(sigval_),m_weight(weight_){}
	Trigger( const Trigger& o)
		:m_slot(o.m_slot),m_sigtype(o.m_sigtype),m_sigval(o.m_sigval),m_weight(o.m_weight){}
	Trigger( const Trigger& o, uint32_t slot_)
		:m_slot(slot_),m_sigtype(o.m_sigtype),m_sigval(o.m_sigval),m_weight(o.m_weight){}

	uint32_t slot() const		{return m_slot;}
	SigType sigtype() const		{return (SigType)m_sigtype;}
	uint32_t sigval() const		{return (uint32_t)m_sigval;}
	float weight() const		{return m_weight;}

private:
	uint32_t m_slot;
	unsigned int m_sigtype:3;
	unsigned int m_sigval:29;
	float m_weight;
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

struct WeightedEvent
{
	uint32_t event;
	float weight;

	WeightedEvent( uint32_t event_, float weight_)
		:event(event_),weight(weight_){}
	WeightedEvent( const WeightedEvent& o)
		:event(o.event),weight(o.weight){}
};

typedef PodStructArrayBase<WeightedEvent,std::size_t> WeightedEventList;


struct ActionSlot
{
	uint32_t value;
	uint32_t event;
	float weight;

	ActionSlot( uint32_t value_, uint32_t event_, float weight_)
		:value(value_),event(event_),weight(weight_){}
	ActionSlot( const ActionSlot& o)
		:value(o.value),event(o.event),weight(o.weight){}

	WeightedEvent fire( Trigger::SigType sigtype, uint32_t sigval, float sigweight);
};

struct ActionSlotTableFreeListElem {uint32_t _;uint32_t next;};

class ActionSlotTable
	:public PodStructTableBase<ActionSlot,uint32_t,ActionSlotTableFreeListElem>
{
public:
	typedef PodStructTableBase<ActionSlot,uint32_t,ActionSlotTableFreeListElem> Parent;

	ActionSlotTable(){}
	ActionSlotTable( const ActionSlotTable& o) :Parent(o){}

	WeightedEvent fire( const Trigger& trigger)
	{
		return (*this)[ trigger.slot()].fire( trigger.sigtype(), trigger.sigval(), trigger.weight());
	}
};


class EventTriggerTable
{
public:
	EventTriggerTable();
	EventTriggerTable( const EventTriggerTable& o);

	uint32_t add( const EventTrigger& et);
	void remove( uint32_t idx);

	void doTransition( ActionSlotTable& slottab, const WeightedEvent& event) const;

private:
	void doTransition( WeightedEventList& followevents, ActionSlotTable& slottab, const WeightedEvent& event) const;
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
	uint32_t actionSlotListIdx;
	uint32_t eventTriggerListIdx;

	Rule()
		:actionSlotListIdx(0),eventTriggerListIdx(0){}
	Rule( uint32_t actionSlotListIdx_, uint32_t eventTriggerListIdx_)
		:actionSlotListIdx(actionSlotListIdx_),eventTriggerListIdx(eventTriggerListIdx_){}
	Rule( const Rule& o)
		:actionSlotListIdx(o.actionSlotListIdx),eventTriggerListIdx(o.eventTriggerListIdx){}
};

struct RuleTableFreeListElem {uint32_t _;uint32_t next;};

class RuleTable
	:public PodStructTableBase<Rule,uint32_t,RuleTableFreeListElem>
{
public:
	typedef PodStructTableBase<Rule,uint32_t,RuleTableFreeListElem> Parent;

	RuleTable(){}
	RuleTable( const RuleTable& o) :Parent(o){}
};

struct EventData
{
	std::size_t bytepos;
	std::size_t bytesize;
	uint32_t ordpos;
	uint32_t eventid;

	EventData( std::size_t bytepos_, std::size_t bytesize_, uint32_t ordpos_, uint32_t eventid_)
		:bytepos(bytepos_),bytesize(bytesize_),ordpos(ordpos_),eventid(eventid_){}
	EventData( const EventData& o)
		:bytepos(o.bytepos),bytesize(o.bytesize),ordpos(o.ordpos),eventid(o.eventid){}
};

class StateMachine
{
public:
	StateMachine(){}
	StateMachine( const StateMachine& o)
		:m_eventTriggerTable(o.m_eventTriggerTable)
		,m_actionSlotTable(o.m_actionSlotTable)
		,m_actionSlotList(o.m_actionSlotList)
		,m_eventTriggerList(o.m_eventTriggerList)
		,m_ruleTable(o.m_ruleTable){}

	uint32_t createRule();
	uint32_t createSlot( uint32_t rule, uint32_t initvalue, uint32_t event, float initweight);
	void createTrigger( uint32_t rule, uint32_t event, uint32_t slot,
				Trigger::SigType sigtype, uint32_t sigval, float weight);
	void disposeRule( uint32_t rule);

private:
	enum {MaxActionSlots=1024};
	EventTriggerTable m_eventTriggerTable;
	ActionSlotTable m_actionSlotTable;
	PodStackPoolBase<uint32_t,uint32_t> m_actionSlotList;
	PodStackPoolBase<uint32_t,uint32_t> m_eventTriggerList;
	RuleTable m_ruleTable;
};

} //namespace
#endif

