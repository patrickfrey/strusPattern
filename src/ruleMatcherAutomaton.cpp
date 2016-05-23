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

void EventTriggerTable::getTriggers( TriggerRefList& triggers, uint32_t event) const
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

uint32_t ProgramTable::createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_)
{
	return 1+m_programTable.add( Program( positionRange_, actionSlotDef_));
}

void ProgramTable::createTrigger( uint32_t programidx, uint32_t event, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable)
{
	Program& program = m_programTable[ programidx-1];
	m_triggerList.push( program.triggerListIdx, TriggerDef( event, sigtype, sigval, variable));
}

void ProgramTable::defineProgramResult( uint32_t programidx, uint32_t eventid, uint32_t resultHandle)
{
	Program& program = m_programTable[ programidx-1];
	program.slotDef.event = eventid;
	program.slotDef.resultHandle = resultHandle;
}

void ProgramTable::defineEventProgram( uint32_t eventid, uint32_t programidx)
{
	utils::UnorderedMap<uint32_t,uint32_t>::iterator ei = eventProgamMap.find( eventid);
	if (ei == eventProgamMap.end())
	{
		uint32_t list = 0;
		programList.push( list, programidx);
		eventProgamMap[ eventid] = list;
	}
	else
	{
		programList.push( ei->second, programidx);
	}
}

uint32_t ProgramTable::getEventProgramList( uint32_t eventid) const
{
	utils::UnorderedMap<uint32_t,uint32_t>::const_iterator ei = eventProgamMap.find( eventid);
	return ei == eventProgamMap.end() ? 0:ei->second;
}

bool ProgramTable::nextProgram( uint32_t& programlist, uint32_t& program) const
{
	return programList.next( programlist, program);
}


StateMachine::StateMachine( const ProgramTable* programTable_)
	:m_programTable(programTable_)
	,m_curpos(0)
{
	std::make_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
}

StateMachine::StateMachine( const StateMachine& o)
	:m_programTable(o.m_programTable)
	,m_eventTriggerTable(o.m_eventTriggerTable)
	,m_actionSlotTable(o.m_actionSlotTable)
	,m_eventTriggerList(o.m_eventTriggerList)
	,m_eventItemList(o.m_eventItemList)
	,m_eventDataReferenceTable(o.m_eventDataReferenceTable)
	,m_ruleTable(o.m_ruleTable)
	,m_results(o.m_results)
	,m_curpos(o.m_curpos)
	,m_ruleDisposeQueue(o.m_ruleDisposeQueue){}


uint32_t StateMachine::createRule( uint32_t positionRange, uint32_t actionSlotIdx, uint32_t eventTriggerListIdx, uint32_t eventDataReferenceIdx)
{
	uint32_t rt = m_ruleTable.add( Rule( actionSlotIdx, eventTriggerListIdx, eventDataReferenceIdx, m_curpos + positionRange));
	m_ruleDisposeQueue.push_back( DisposeEvent( m_curpos + positionRange, rt));
	std::push_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	return rt;
}

void StateMachine::disposeRule( uint32_t rule)
{
	Rule& rulerec = m_ruleTable[ rule];
	m_actionSlotTable.remove( rulerec.actionSlotIdx);
	rulerec.actionSlotIdx = 0;

	uint32_t triggerlistitr = rulerec.eventTriggerListIdx;
	uint32_t trigger;
	while (m_eventTriggerList.next( triggerlistitr, trigger))
	{
		if (trigger) m_eventTriggerTable.remove( trigger);
	}
	m_eventTriggerList.remove( rulerec.eventTriggerListIdx);
	rulerec.eventTriggerListIdx = 0;

	disposeEventDataReference( rulerec.eventDataReferenceIdx);
	m_ruleTable.remove( rule);
}

void StateMachine::disposeEventDataReference( uint32_t eventdataref)
{
	EventDataReference& ref = m_eventDataReferenceTable[ eventdataref];
	if (ref.referenceCount > 1)
	{
		--ref.referenceCount;
	}
	else if (ref.referenceCount == 1)
	{
		--ref.referenceCount;
		m_eventItemList.remove( ref.eventItemListIdx);
		m_eventDataReferenceTable.remove( eventdataref);
	}
	else
	{
		throw strus::runtime_error( _TXT("illegal free of event data reference"));
	}
}

void StateMachine::referenceEventData( uint32_t eventdataref)
{
	EventDataReference& ref = m_eventDataReferenceTable[ eventdataref];
	++ref.referenceCount;
}

void StateMachine::appendEventData( uint32_t eventdataref, const EventItem& item)
{
	EventDataReference& ref = m_eventDataReferenceTable[ eventdataref];
	referenceEventData( item.data.subdataref);
	m_eventItemList.push( ref.eventItemListIdx, item);
}

void StateMachine::doTransition( uint32_t event, const EventData& data)
{
	typedef PodStructArrayBase<EventData,std::size_t,0> EventList;
	EventList followList;
	followList.add( data);
	referenceEventData( followList[0].subdataref);

	uint32_t programlist = m_programTable->getEventProgramList( event);
	uint32_t program;
	while (m_programTable->nextProgram( programlist, program))
	{
		installProgram( program);
	}

	std::size_t ei = 0;
	for (; ei < followList.size(); ++ei)
	{
		enum {NofTriggers=256};
		Trigger* trigger_alloca[ NofTriggers];
		EventTriggerTable::TriggerRefList triggers( trigger_alloca, NofTriggers);

		const EventData& follow = followList[ ei];
		m_eventTriggerTable.getTriggers( triggers, follow.eventid);
		std::size_t ti = 0, te = triggers.size();
		for (; ti != te; ++ti)
		{
			Trigger& trigger = *triggers[ ti];
			ActionSlot& slot = m_actionSlotTable[ trigger.slot()];
			Rule& rule = m_ruleTable[ slot.rule];
			bool match = false;
			bool takeEventData = false;
			switch (trigger.sigtype())
			{
				case Trigger::SigAny:
					match = true;
					takeEventData = true;
					break;
				case Trigger::SigSequence:
					if (trigger.sigval() < slot.value)
					{
						slot.value = trigger.sigval();
						--slot.count;
						match = (slot.count == 0);
						takeEventData = true;
					}
					break;
				case Trigger::SigInRange:
				{
					if ((trigger.sigval() & slot.value) != 0)
					{
						slot.value &= ~trigger.sigval();
						--slot.count;
						match = (slot.count == 0);
						takeEventData = true;
					}
					break;
				}
				case Trigger::SigDel:
				{
					disposeRule( slot.rule);
					continue;
				}
			}
			if (takeEventData)
			{
				if (trigger.variable())
				{
					EventItem item( trigger.variable(), data);
					appendEventData( rule.eventDataReferenceIdx, item);
				}
				if (slot.start_ordpos == 0 || slot.start_ordpos > data.ordpos)
				{
					slot.start_ordpos = data.ordpos;
					slot.start_origpos = data.origpos;
				}
			}
			if (match)
			{
				if (!rule.done)
				{
					if (slot.event)
					{
						std::size_t eventOrigSize = data.origpos + data.origsize - slot.start_origpos;
						EventData followEventData( slot.start_origpos, eventOrigSize, slot.start_ordpos, slot.event, rule.eventDataReferenceIdx);
						referenceEventData( rule.eventDataReferenceIdx);
						followList.add( followEventData);
					}
					if (slot.resultHandle)
					{
						m_results.add( Result( slot.resultHandle, rule.eventDataReferenceIdx));
						referenceEventData( rule.eventDataReferenceIdx);
						rule.done = true;
					}
					rule.done = true;
				}
			}
		}
		disposeEventDataReference( follow.subdataref);
	}
}

void StateMachine::setCurrentPos( uint32_t pos)
{
	if (pos < m_curpos) throw strus::runtime_error(_TXT("illegal set of current pos (positions not ascending)"));
	m_curpos = pos;
	while (!m_ruleDisposeQueue.empty() && m_ruleDisposeQueue.front().pos < pos)
	{
		disposeRule( m_ruleDisposeQueue.front().idx);
		std::pop_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
		m_ruleDisposeQueue.pop_back();
	}
}

void StateMachine::installProgram( uint32_t programidx)
{
	const Program& program = (*m_programTable)[ programidx];
	uint32_t eventDataReferenceIdx = m_eventDataReferenceTable.add( EventDataReference());
	uint32_t ruleidx = createRule( program.positionRange, 0/*slotIdx*/, 0/*trigger list*/, eventDataReferenceIdx);
	Rule& rule = m_ruleTable[ ruleidx];

	rule.actionSlotIdx =
		m_actionSlotTable.add(
			ActionSlot( program.slotDef.initsigval, program.slotDef.initcount,
					program.slotDef.event, ruleidx, program.slotDef.resultHandle));

	uint32_t program_triggerListItr = program.triggerListIdx;
	const TriggerDef* triggerDef;
	while (0!=(triggerDef=m_programTable->triggerList().nextptr( program_triggerListItr)))
	{
		uint32_t eventTrigger =
			m_eventTriggerTable.add(
				EventTrigger( triggerDef->event, 
				Trigger( rule.actionSlotIdx, 
					 triggerDef->sigtype, triggerDef->sigval, triggerDef->variable)));
		m_eventTriggerList.push( rule.eventTriggerListIdx, eventTrigger);
	}
}



