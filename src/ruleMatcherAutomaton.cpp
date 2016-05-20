/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ruleMatcherAutomaton.hpp"
#include <limits>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

using namespace strus;

struct EventTriggerFreeListElem
{
	uint32_t next;
};

EventTriggerTable::EventTriggerTable()
	:m_eventAr(0),m_triggerAr(0),m_allocsize(0),m_size(0),m_freelistidx(0){}
EventTriggerTable::EventTriggerTable( const EventTriggerTable& o)
	:m_eventAr(0),m_triggerAr(0),m_allocsize(0),m_size(o.m_size),m_freelistidx(o.m_freelistidx)
{
	if (o.m_allocsize)
	{
		expand( o.m_allocsize);
		std::memcpy( m_eventAr, o.m_eventAr, m_size * sizeof(*m_eventAr));
		std::memcpy( m_triggerAr, o.m_triggerAr, m_size * sizeof(*m_triggerAr));
	}
}

void EventTriggerTable::expand( uint32_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::logic_error( "illegal call of EventTriggerTable::expand");
	}
	uint32_t* war = (uint32_t*)std::realloc( m_eventAr, newallocsize * sizeof(uint32_t));
	Trigger* far = (Trigger*)std::realloc( m_triggerAr, newallocsize * sizeof(Trigger));
	if (!war || !far)
	{
		if (war) m_eventAr = war;
		if (far) m_triggerAr = far;
		throw std::bad_alloc();
	}
	m_eventAr = war;
	m_triggerAr = far;
	m_allocsize = newallocsize;
}

uint32_t EventTriggerTable::add( const EventTrigger& et)
{
	uint32_t newidx;
	if (m_freelistidx)
	{
		newidx = m_freelistidx;
		m_freelistidx = ((EventTriggerFreeListElem*)(void*)(&m_triggerAr[ m_freelistidx-1]))->next;
	}
	else
	{
		if (m_size == m_allocsize)
		{
			if (m_allocsize >= (1U<<31))
			{
				throw std::bad_alloc();
			}
			expand( m_allocsize?(m_allocsize*2):BlockSize);
		}
		++newidx = m_size;
	}
	m_eventAr[ newidx-1] = et.event;
	m_triggerAr[ newidx-1] = et.trigger;
	return newidx;
}

void EventTriggerTable::remove( uint32_t idx)
{
	if (idx > m_size || idx == 0)
	{
		throw strus::runtime_error( _TXT("bad trigger index (remove trigger)"));
	}
	m_eventAr[ idx-1] = 0;
	((EventTriggerFreeListElem*)(void*)(&m_triggerAr[ idx-1]))->next = m_freelistidx;
	m_freelistidx = idx;
}

void EventTriggerTable::getTriggers( PodStructArrayBase<Trigger*,std::size_t>& triggers, uint32_t event) const
{
	if (!event) return;
	uint32_t wi=0;
	for (; wi<m_allocsize; ++wi)
	{
		if (m_eventAr[ wi] == event)
		{
			triggers.add( m_triggerAr + wi);
		}
	}
}

uint32_t StateMachine::createRule( uint32_t positionRange)
{
	uint32_t rt = m_ruleTable.add( Rule(0, 0, m_curpos + positionRange));
	m_ruleDisposeQueue.push_back( DisposeEvent( m_curpos + positionRange, rt));
	std::push_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	return rt;
}

uint32_t StateMachine::createGroup( uint32_t rule, uint32_t positionRange)
{
	Rule& rulerec = m_ruleTable[ rule];
	uint32_t lastpos = m_curpos + positionRange;
	if (rulerec.lastpos < lastpos)
	{
		lastpos = rulerec.lastpos;
	}
	uint32_t rt = m_groupTable.add( Group());
	m_groupList.push( rulerec.groupListIdx, rt);
	m_groupDisposeQueue.push_back( DisposeEvent( m_curpos + positionRange, rt));
	std::push_heap( m_groupDisposeQueue.begin(), m_groupDisposeQueue.end());
	return rt;
}

uint32_t StateMachine::createSlot( uint32_t rule, uint32_t group, uint32_t initsigval, uint32_t event, uint32_t program, float initweight, bool isComplete)
{
	Group& grouprec = m_groupTable[ group];
	uint32_t aidx = m_actionSlotTable.add( ActionSlot( initsigval, event, program, rule, initweight, isComplete));
	m_actionSlotList.push( grouprec.actionSlotListIdx, aidx);
	return aidx;
}

void StateMachine::createTrigger( uint32_t /*rule*/, uint32_t group, uint32_t event, uint32_t slot,
					Trigger::SigType sigtype, uint32_t sigval, float weight)
{
	Group& grouprec = m_groupTable[ group];
	uint32_t ridx = m_eventTriggerTable.add( EventTrigger( event, Trigger( slot, sigtype, sigval, weight)));
	m_eventTriggerList.push( grouprec.eventTriggerListIdx, ridx);
}

void StateMachine::disposeRule( uint32_t rule)
{
	Rule& rulerec = m_ruleTable[ rule];
	uint32_t grouplistitr = rulerec.groupListIdx;
	uint32_t groupidx;
	while (m_groupList.next( grouplistitr, groupidx))
	{
		deactivateGroup( groupidx);
		m_groupTable.remove( groupidx);
	}
	m_groupList.remove( rulerec.groupListIdx);
	rulerec.groupListIdx = 0;

	m_eventItemList.remove( rulerec.eventItemListIdx);
	m_ruleTable.remove( rule);
}

void StateMachine::deactivateGroup( uint32_t groupidx)
{
	Group& grouprec = m_groupTable[ groupidx];
	{
		uint32_t triggerlistitr = grouprec.eventTriggerListIdx;
		uint32_t trigger;
		while (m_eventTriggerList.next( triggerlistitr, trigger))
		{
			if (trigger) m_eventTriggerTable.remove( trigger);
		}
		m_eventTriggerList.remove( grouprec.eventTriggerListIdx);
		grouprec.eventTriggerListIdx = 0;
	}
	{
		uint32_t slotlistitr = grouprec.actionSlotListIdx;
		uint32_t slot;
		while (m_actionSlotList.next( slotlistitr, slot))
		{
			if (slot) m_actionSlotTable.remove( slot);
		}
		m_actionSlotList.remove( grouprec.actionSlotListIdx);
		grouprec.actionSlotListIdx = 0;
	}
}

struct WeightedEvent
{
	EventData eventData;
	float weight;

	WeightedEvent( const EventData& eventData_, float weight_)
		:eventData(eventData_),weight(weight_){}
	WeightedEvent( const WeightedEvent& o)
		:eventData(o.eventData),weight(o.weight){}
};
typedef PodStructArrayBase<WeightedEvent,std::size_t> WeightedEventList;

uint32_t ProgramTable::createProgram( uint32_t positionRange_)
{
	return 1+m_programTable.add( Program( positionRange_));
}

void ProgramTable::createSlot( uint32_t programidx, uint32_t initsigval, uint32_t event, uint32_t follow_program, float initweight, bool isComplete)
{
	Program& program = m_programTable[ programidx-1];
	m_actionSlotList.push( program.actionSlotListIdx, ActionSlotDef( initsigval, event, follow_program, initweight, isComplete));
}

void ProgramTable::createTrigger( uint32_t programidx, uint32_t event, uint32_t slot, Trigger::SigType sigtype, uint32_t sigval, float weight)
{
	Program& program = m_programTable[ programidx-1];
	m_triggerList.push( program.triggerListIdx, TriggerDef( event, slot, sigtype, sigval, weight));
}

StateMachine::StateMachine( const ProgramTable* programTable_)
	:m_programTable(programTable_)
	,m_curpos(0)
{
	std::make_heap( m_groupDisposeQueue.begin(), m_groupDisposeQueue.end());
	std::make_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
}

StateMachine::StateMachine( const StateMachine& o)
	:m_programTable(o.m_programTable)
	,m_eventTriggerTable(o.m_eventTriggerTable)
	,m_actionSlotTable(o.m_actionSlotTable)
	,m_groupTable(o.m_groupTable)
	,m_actionSlotList(o.m_actionSlotList)
	,m_eventTriggerList(o.m_eventTriggerList)
	,m_groupList(o.m_groupList)
	,m_eventItemList(o.m_eventItemList)
	,m_ruleTable(o.m_ruleTable)
	,m_results(o.m_results)
	,m_curpos(o.m_curpos){}

void StateMachine::doTransition( uint32_t event, float weight, const EventData& data)
{
	WeightedEventList followList;
	followList.add( WeightedEvent( data, weight));
	std::size_t ei = 0;
	enum {NofTriggers=256};
	for (; ei < followList.size(); ++ei)
	{
		Trigger* trigger_alloca[ NofTriggers];
		PodStructArrayBase<Trigger*,std::size_t> triggers( trigger_alloca, NofTriggers);
		const WeightedEvent& follow = followList[ ei];
		m_eventTriggerTable.getTriggers( triggers, follow.eventData.eventid);
		std::size_t ti = 0, te = triggers.size();
		for (; ti != te; ++ti)
		{
			Trigger& trigger = *triggers[ ti];
			ActionSlot& slot = m_actionSlotTable[ trigger.slot()];
			Rule& rule = m_ruleTable[ slot.rule];
			bool match = false;
			switch (trigger.sigtype())
			{
				case Trigger::SigAll:
					slot.weight *= trigger.weight();
					match = true;
					if (trigger.variable())
					{
						EventItem item( trigger.variable(), trigger.weight(), data);
						m_eventItemList.push( rule.eventItemListIdx, item);
					}
					if (slot.start_ordpos == 0 || slot.start_ordpos > data.ordpos)
					{
						slot.start_ordpos = data.ordpos;
						slot.start_bytepos = data.bytepos;
					}
					break;
				case Trigger::SigSeq:
					if (trigger.sigval() == slot.value)
					{
						slot.weight *= trigger.weight();
						--slot.value;
						match = (slot.value == 0);
						if (trigger.variable())
						{
							EventItem item( trigger.variable(), trigger.weight(), data);
							m_eventItemList.push( rule.eventItemListIdx, item);
						}
						if (slot.start_ordpos == 0 || slot.start_ordpos > data.ordpos)
						{
							slot.start_ordpos = data.ordpos;
							slot.start_bytepos = data.bytepos;
						}
					}
					break;
				case Trigger::SigAny:
				{
					if ((trigger.sigval() & slot.value) != 0)
					{
						slot.weight *= trigger.weight();
						slot.value &= ~trigger.sigval();
						match = (slot.value == 0);
						if (trigger.variable())
						{
							EventItem item( trigger.variable(), trigger.weight(), data);
							m_eventItemList.push( rule.eventItemListIdx, item);
						}
						if (slot.start_ordpos == 0 || slot.start_ordpos > data.ordpos)
						{
							slot.start_ordpos = data.ordpos;
							slot.start_bytepos = data.bytepos;
						}
					}
					break;
				}
				case Trigger::SigDel:
				{
					if (rule.isComplete)
					{
						m_results.add( Result( slot.rule, rule.eventItemListIdx));
						rule.eventItemListIdx = 0;
					}
					disposeRule( slot.rule);
					continue;
				}
			}
			if (match)
			{
				rule.isComplete |= slot.isComplete;
				if (slot.program)
				{
					installProgram( slot.rule, slot.program);
				}
				if (slot.event)
				{
					std::size_t eventByteSize = data.bytepos + data.bytesize - slot.start_bytepos;
					EventData followEventData( slot.start_bytepos, eventByteSize, slot.start_ordpos, slot.event);
					followList.add( WeightedEvent( followEventData, slot.weight));
				}
				else
				{
					m_results.add( Result( slot.rule, rule.eventItemListIdx));
					rule.eventItemListIdx = 0;
					disposeRule( slot.rule);
				}
			}
		}
	}
}

void StateMachine::setCurrentPos( uint32_t pos)
{
	if (pos < m_curpos) throw strus::runtime_error(_TXT("illegal set of current pos (positions not ascending)"));
	m_curpos = pos;
	while (!m_groupDisposeQueue.empty() && m_groupDisposeQueue.front().pos < pos)
	{
		deactivateGroup( m_groupDisposeQueue.front().idx);
		std::pop_heap( m_groupDisposeQueue.begin(), m_groupDisposeQueue.end());
		m_groupDisposeQueue.pop_back();
	}
	while (!m_ruleDisposeQueue.empty() && m_ruleDisposeQueue.front().pos < pos)
	{
		disposeRule( m_ruleDisposeQueue.front().idx);
		std::pop_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
		m_ruleDisposeQueue.pop_back();
	}
}

void StateMachine::installProgram( uint32_t ruleidx, uint32_t programidx)
{
	enum {NofSlots=32};
	uint32_t slotar_alloca[ NofSlots];
	const Program& program = (*m_programTable)[ programidx];
	uint32_t groupidx = createGroup( ruleidx, program.positionRange);
	PodStructArrayBase<uint32_t,uint32_t> slotar( slotar_alloca, NofSlots);
	uint32_t actionSlotListIdx = program.actionSlotListIdx;
	const ActionSlotDef* actionSlotDef;
	while (0!=(actionSlotDef=m_programTable->actionSlotList().nextptr( actionSlotListIdx)))
	{
		slotar.add(
			createSlot( ruleidx, groupidx, actionSlotDef->initsigval,
				actionSlotDef->event, actionSlotDef->program,
				actionSlotDef->initweight, actionSlotDef->isComplete));
	}
	uint32_t triggerListIdx = program.triggerListIdx;
	const TriggerDef* triggerDef;
	while (0!=(triggerDef=m_programTable->triggerList().nextptr( triggerListIdx)))
	{
		uint32_t slotidx = slotar[ triggerDef->slot];
		createTrigger( ruleidx, groupidx, triggerDef->event, slotidx,
				triggerDef->sigtype, triggerDef->sigval, triggerDef->weight);
	}
	
}



