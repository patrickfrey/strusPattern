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

using namespace strus;

WeightedEvent ActionSlot::fire( Trigger::SigType sigtype, uint32_t sigval, float sigweight)
{
	switch (sigtype)
	{
		case Trigger::SigAll:
			weight *= sigweight;
			return WeightedEvent( event, weight);
		case Trigger::SigSeq:
			if (sigval == value++)
			{
				weight *= sigweight;
				return WeightedEvent( event, weight);
			}
			return WeightedEvent( 0, 0.0);
		case Trigger::SigAny:
		{
			if ((sigval & value) == 0)
			{
				weight *= sigweight;
				value |= sigval;
				return WeightedEvent( event, weight);
			}
			return WeightedEvent( 0, 0.0);
		}
		case Trigger::SigDel:
		{
			if (0 != (value & ~sigval))
			{
				value &= sigval;
				return WeightedEvent( event, weight);
			}
			return WeightedEvent( 0, 0.0);
		}
	}
	return WeightedEvent( 0, 0.0);
}

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
		newidx = m_freelistidx-1;
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
		newidx = m_size++;
	}
	m_eventAr[ newidx] = et.event;
	m_triggerAr[ newidx] = et.trigger;
	return newidx;
}

void EventTriggerTable::remove( uint32_t idx)
{
	if (idx >= m_size)
	{
		throw std::runtime_error("bad rule index (remove rule)");
	}
	m_eventAr[ idx] = 0;
	((EventTriggerFreeListElem*)(void*)(&m_triggerAr[ idx]))->next = m_freelistidx;
	m_freelistidx = idx+1;
}

void EventTriggerTable::doTransition( WeightedEventList& followevents, ActionSlotTable& slottab, const WeightedEvent& event) const
{
	if (!event.event) return;
	uint32_t wi=0;
	for (; wi<m_allocsize; ++wi)
	{
		if (m_eventAr[ wi] == event.event)
		{
			WeightedEvent follow = slottab.fire( m_triggerAr[ wi]);
			if (follow.event) followevents.add( follow);
		}
	}
}

void EventTriggerTable::doTransition( ActionSlotTable& slottab, const WeightedEvent& event) const
{
	WeightedEventList followevents;
	doTransition( followevents, slottab, event);
	std::size_t fi = 0;
	for (; fi < followevents.size(); ++fi)
	{
		doTransition( followevents, slottab, followevents[fi]);
	}
}

uint32_t StateMachine::createRule()
{
	return m_ruleTable.add( Rule());
}

uint32_t StateMachine::createSlot( uint32_t rule, uint32_t initvalue, uint32_t event, float initweight)
{
	uint32_t aidx = m_actionSlotTable.add( ActionSlot( initvalue, event, initweight));
	m_actionSlotList.push( m_ruleTable[ rule].actionSlotListIdx, aidx);
	return aidx;
}

void StateMachine::createTrigger( uint32_t rule, uint32_t event, uint32_t slot,
					Trigger::SigType sigtype, uint32_t sigval, float weight)
{
	uint32_t ridx = m_eventTriggerTable.add( EventTrigger( event, Trigger( slot, sigtype, sigval, weight)));
	m_eventTriggerList.push( m_ruleTable[ rule].eventTriggerListIdx, ridx);
}

void StateMachine::disposeRule( uint32_t rule)
{
	uint32_t ridx = m_ruleTable[ rule].eventTriggerListIdx;
	uint32_t eidx;
	while (m_eventTriggerList.next( ridx, eidx))
	{
		m_eventTriggerTable.remove( eidx);
	}
	ridx = m_ruleTable[ rule].actionSlotListIdx;
	while (m_eventTriggerList.next( ridx, eidx))
	{
		m_eventTriggerTable.remove( eidx);
	}
}


