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
#ifdef __SSE__
#include <emmintrin.h>
#define STRUS_USE_SSE_SCAN_TRIGGERS
#endif

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

enum {EventArrayMemoryAlignment=64};
void EventTriggerTable::expand( uint32_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::logic_error( "illegal call of EventTriggerTable::expand");
	}
	uint32_t* war = (uint32_t*)utils::aligned_malloc( newallocsize * sizeof(uint32_t), EventArrayMemoryAlignment);
	if (!war) throw std::bad_alloc();
	std::memcpy( war, m_eventAr, m_size * sizeof(uint32_t));
	m_eventAr = war;
	Trigger* far = (Trigger*)std::realloc( m_triggerAr, newallocsize * sizeof(Trigger));
	if (!far) throw std::bad_alloc();
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
		newidx = ++m_size;
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

#ifdef STRUS_USE_SSE_SCAN_TRIGGERS
inline std::ostream & operator << (std::ostream& out, const __v4si & val)
{
	const uint32_t* vals;
	vals = (const uint32_t*)&val;
	return out << "[" << vals[0] << ", " << vals[1] << ", " << vals[2]
				<< ", " << vals[3] << "]";
}

///\brief Linear search for triggers to fire on an event with help of SSE vectorization
///\note This SIMD vectorization implementation with SSE was inspired by https://schani.wordpress.com/tag/c-optimization-linear-binary-search-sse2-simd
static inline void getTriggers_SSE4( Trigger** results, std::size_t& nofresults, uint32_t event, const uint32_t* eventar_, Trigger* triggerar, std::size_t arsize)
{
	__v4si *eventar = (__v4si*)eventar_;		//... the SIMD search block (aligned to EventArrayMemoryAlignment byte blocks)
	uint32_t ii = 0;				//... index on 16 word blocks we handle with SSE
	uint32_t nn = ((uint32_t)arsize >> 4) << 2;	//... number of 16 word blocks we handle with SSE
	// We prepare the SIMD search key vector as 4 times the value of the needle
	__v4si event4 = (__v4si)_mm_set_epi32( event, event, event, event);

	for (;ii<nn; ii+=4)
	{
		// Compare 4 times 4 words = 16 32 bit uints and store the results in cmp[i].
		// cmp[i]: each [i] are 4 32 bit words, 0xFFffFFff true, 0x00000000 false:
		__v4si cmp0 = __builtin_ia32_pcmpeqd128( event4, eventar[ ii + 0]);
		__v4si cmp1 = __builtin_ia32_pcmpeqd128( event4, eventar[ ii + 1]);
		__v4si cmp2 = __builtin_ia32_pcmpeqd128( event4, eventar[ ii + 2]);
		__v4si cmp3 = __builtin_ia32_pcmpeqd128( event4, eventar[ ii + 3]);
		// Pack two times the lo word (with sign) of 4 32 bit words into 8 * 16 bit words:
		__v8hi pack01 = __builtin_ia32_packssdw128 (cmp0, cmp1);
		// Pack two times the lo word (with sign) of 4 32 bit words into 8 * 16 bit words:
		__v8hi pack23 = __builtin_ia32_packssdw128 (cmp2, cmp3);
		// Pack two times the lo byte (with sign) of 8 16 bit words into 16 bytes:
		__v16qi pack0123 = __builtin_ia32_packsswb128 (pack01, pack23);

		// Pack the most significant bit of 16 bytes into a 16 bit word:
		uint16_t res = __builtin_ia32_pmovmskb128( pack0123);
		while (res)
		{
			// ... while there is a bit, that points to a match,
			// get trailing bit index into 'tz':
			uint8_t tz = __builtin_ctz( res);
			// Evaluate the index of the corresponding match:
			uint32_t idx = (ii << 2) + tz;
			// and append it to the result (list of triggers fired by the event):
			results[ nofresults++] = triggerar + idx;
			// and mask out the visited match from the mask with the matches:
			res ^= (1 << tz);
		}
	}
	// The rest of the events modulo 16, we handle in the standard way:
	ii <<= 2;
	for (; ii < arsize; ++ii)
	{
		if (eventar_[ii] == event)
		{
			results[ nofresults++] = triggerar + ii;
		}
	}
}
#endif

void EventTriggerTable::getTriggers( TriggerRefList& triggers, uint32_t event) const
{
	// The following implementation looks a little bit funny, but it is 
	// crucial for the overall performance that this method is vectorizable.
	if (!event) return;
	Trigger** tar = triggers.reserve( m_size);
	std::size_t nofresults = 0;

#if __GNUC__ >= 4
	const uint32_t* ar = (const uint32_t*)__builtin_assume_aligned( m_eventAr, EventArrayMemoryAlignment);
#else
	const uint32_t* ar = m_eventAr;
#endif
#ifdef STRUS_USE_SSE_SCAN_TRIGGERS
	getTriggers_SSE4( tar, nofresults, event, ar, m_triggerAr, m_size);
#else
	uint32_t wi=0;
	for (; wi<m_size; ++wi)
	{
		if (ar[ wi] == event)
		{
			tar[ nofresults++] = m_triggerAr + wi;
		}
	}
#endif
	triggers.commit_reserved( nofresults);
}

uint32_t ProgramTable::createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_)
{
	return 1+m_programTable.add( Program( positionRange_, actionSlotDef_));
}

void ProgramTable::createTrigger( uint32_t programidx, uint32_t event, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable, float weight)
{
	Program& program = m_programTable[ programidx-1];
	m_triggerList.push( program.triggerListIdx, TriggerDef( event, sigtype, sigval, variable, weight));
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
	,m_nofPatternsTriggered(0)
	,m_nofOpenPatterns(0.0)
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
	,m_ruleDisposeQueue(o.m_ruleDisposeQueue)
	,m_nofPatternsTriggered(o.m_nofPatternsTriggered)
	,m_nofOpenPatterns(o.m_nofOpenPatterns)
{}


uint32_t StateMachine::createRule( uint32_t positionRange)
{
	uint32_t rt = m_ruleTable.add( Rule( m_curpos + positionRange));
	m_ruleDisposeQueue.push_back( DisposeEvent( m_curpos + positionRange, rt));
	std::push_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	return rt;
}

void StateMachine::deactivateRule( uint32_t rule)
{
	Rule& rulerec = m_ruleTable[ rule];
	if (rulerec.isActive())
	{
		m_actionSlotTable.remove( rulerec.actionSlotIdx-1);
		rulerec.actionSlotIdx = 0;
	
		uint32_t triggerlistitr = rulerec.eventTriggerListIdx;
		uint32_t trigger;
		while (m_eventTriggerList.next( triggerlistitr, trigger))
		{
			if (trigger) m_eventTriggerTable.remove( trigger);
		}
		m_eventTriggerList.remove( rulerec.eventTriggerListIdx);
		rulerec.eventTriggerListIdx = 0;
	
		if (rulerec.eventDataReferenceIdx)
		{
			disposeEventDataReference( rulerec.eventDataReferenceIdx);
			rulerec.eventDataReferenceIdx = 0;
		}
	}
}

void StateMachine::disposeRule( uint32_t rule)
{
	deactivateRule( rule);
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
		if (ref.eventItemListIdx)
		{
			m_eventItemList.remove( ref.eventItemListIdx);
		}
		m_eventDataReferenceTable.remove( eventdataref);
	}
	else
	{
		m_eventDataReferenceTable[ eventdataref];
		m_eventItemList.check( ref.eventItemListIdx);
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
	if (item.data.subdataref)
	{
		referenceEventData( item.data.subdataref);
	}
	m_eventItemList.push( ref.eventItemListIdx, item);
}

uint32_t StateMachine::createEventData()
{
	return m_eventDataReferenceTable.add( EventDataReference( 0, 1/*ref*/));
}

void StateMachine::joinEventData( uint32_t eventdataref_dest, uint32_t eventdataref_src)
{
	EventDataReference& ref_dest = m_eventDataReferenceTable[ eventdataref_dest];
	EventDataReference& ref_src = m_eventDataReferenceTable[ eventdataref_src];
	const EventItem* item;
	uint32_t itemlist = ref_src.eventItemListIdx;

	while (0!=(item=m_eventItemList.nextptr( itemlist)))
	{
		if (item->data.subdataref)
		{
			referenceEventData( item->data.subdataref);
		}
		m_eventItemList.push( ref_dest.eventItemListIdx, *item);
	}
}

void StateMachine::doTransition( uint32_t event, const EventData& data)
{
	m_nofOpenPatterns += m_eventTriggerTable.size();

	enum {NofTriggers=1024,NofEventData=1024,NofDisposeRules=1024};
	Trigger* trigger_alloca[ NofTriggers];
	EventData followList_alloca[ NofEventData];
	uint32_t disposeRuleList_alloca[ NofDisposeRules];

	typedef PodStructArrayBase<EventData,std::size_t,0> EventList;
	EventList followList( followList_alloca, NofEventData);
	followList.add( data);
	if (followList[0].subdataref)
	{
		referenceEventData( followList[0].subdataref);
	}
	uint32_t programlist = m_programTable->getEventProgramList( event);
	uint32_t program;
	while (m_programTable->nextProgram( programlist, program))
	{
		installProgram( program);
	}

	std::size_t ei = 0;
	for (; ei < followList.size(); ++ei)
	{
		EventTriggerTable::TriggerRefList triggers( trigger_alloca, NofTriggers);
		typedef PodStructArrayBase<uint32_t,std::size_t,0> DisposeRuleList;
		DisposeRuleList disposeRuleList( disposeRuleList_alloca, NofDisposeRules);

		EventData follow = followList[ ei];
		m_eventTriggerTable.getTriggers( triggers, follow.eventid);
		m_nofPatternsTriggered += triggers.size();
		EventTriggerTable::TriggerRefList::const_iterator
			ti = triggers.begin(), te = triggers.end();
		for (; ti != te; ++ti)
		{
			Trigger& trigger = **ti;
			ActionSlot& slot = m_actionSlotTable[ trigger.slot()];
			Rule& rule = m_ruleTable[ slot.rule];
			bool match = false;
			bool takeEventData = false;
			bool finished = true;
			switch (trigger.sigtype())
			{
				case Trigger::SigAny:
					takeEventData = true;
					if (slot.count > 0)
					{
						match = true;
						--slot.count;
						finished = (slot.count == 0);
					}
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
				case Trigger::SigWithin:
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
					disposeRuleList.add( slot.rule);
					continue;
				}
			}
			if (takeEventData)
			{
				if (trigger.variable())
				{
					EventItem item( trigger.variable(), trigger.weight(), data);
					if (!rule.eventDataReferenceIdx)
					{
						rule.eventDataReferenceIdx = createEventData();
					}
					appendEventData( rule.eventDataReferenceIdx, item);
				}
				else
				{
					if (data.subdataref)
					{
						if (!rule.eventDataReferenceIdx)
						{
							rule.eventDataReferenceIdx = createEventData();
						}
						joinEventData( rule.eventDataReferenceIdx, data.subdataref);
					}
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
						if (rule.eventDataReferenceIdx)
						{
							referenceEventData( rule.eventDataReferenceIdx);
						}
						followList.add( followEventData);
					}
					if (slot.resultHandle)
					{
						m_results.add( Result( slot.resultHandle, rule.eventDataReferenceIdx));
						if (rule.eventDataReferenceIdx)
						{
							referenceEventData( rule.eventDataReferenceIdx);
						}
						rule.done = true;
					}
					rule.done = true;
				}
				if (finished)
				{
					disposeRuleList.add( slot.rule);
				}
			}
		}
		DisposeRuleList::const_iterator
			di = disposeRuleList.begin(), de = disposeRuleList.end();
		for (; di != de; ++di)
		{
			deactivateRule( *di);
		}
		if (follow.subdataref)
		{
			disposeEventDataReference( follow.subdataref);
		}
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
	uint32_t ruleidx = createRule( program.positionRange);
	Rule& rule = m_ruleTable[ ruleidx];

	rule.actionSlotIdx =
		1+m_actionSlotTable.add(
			ActionSlot( program.slotDef.initsigval, program.slotDef.initcount,
					program.slotDef.event, ruleidx, program.slotDef.resultHandle));

	uint32_t program_triggerListItr = program.triggerListIdx;
	const TriggerDef* triggerDef;

	while (0!=(triggerDef=m_programTable->triggerList().nextptr( program_triggerListItr)))
	{
		uint32_t eventTrigger =
			m_eventTriggerTable.add(
				EventTrigger( triggerDef->event, 
				Trigger( rule.actionSlotIdx-1, 
					 triggerDef->sigtype, triggerDef->sigval, triggerDef->variable, triggerDef->weight)));
		m_eventTriggerList.push( rule.eventTriggerListIdx, eventTrigger);
	}
}



