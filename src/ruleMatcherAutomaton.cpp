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
#include <iostream>
#ifdef __SSE__
#include <emmintrin.h>
#undef STRUS_USE_SSE_SCAN_TRIGGERS
#endif
#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

EventTriggerTable::EventTriggerTable()
	:m_eventAr(0),m_triggerIndAr(0),m_triggerTab(),m_allocsize(0),m_size(0){}
EventTriggerTable::EventTriggerTable( const EventTriggerTable& o)
	:m_eventAr(0),m_triggerIndAr(0),m_triggerTab(o.m_triggerTab),m_allocsize(0),m_size(o.m_size)
{
	if (o.m_allocsize)
	{
		expandEventAr( o.m_allocsize);
		std::memcpy( m_eventAr, o.m_eventAr, m_size * sizeof(*m_eventAr));
		std::memcpy( m_triggerIndAr, o.m_triggerIndAr, m_size * sizeof(*m_triggerIndAr));
	}
}

enum {EventArrayMemoryAlignment=64};
void EventTriggerTable::expandEventAr( uint32_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::logic_error( "illegal call of EventTriggerTable::expandEventAr");
	}
	uint32_t* war = (uint32_t*)utils::aligned_malloc( newallocsize * sizeof(uint32_t), EventArrayMemoryAlignment);
	if (!war) throw std::bad_alloc();
	std::memcpy( war, m_eventAr, m_size * sizeof(uint32_t));
	m_eventAr = war;
	uint32_t* far = (uint32_t*)std::realloc( m_triggerIndAr, newallocsize * sizeof(uint32_t));
	if (!far) throw std::bad_alloc();
	m_triggerIndAr = far;
	m_allocsize = newallocsize;
}

uint32_t EventTriggerTable::add( const EventTrigger& et)
{
	if (m_size == m_allocsize)
	{
		if (m_allocsize >= (1U<<31))
		{
			throw std::bad_alloc();
		}
		expandEventAr( m_allocsize?(m_allocsize*2):BlockSize);
	}
	m_eventAr[ m_size] = et.event;
	uint32_t rt = m_triggerIndAr[ m_size] = m_triggerTab.add( LinkedTrigger( m_size, et.trigger));
	++m_size;
	return rt;
}

void EventTriggerTable::remove( uint32_t idx)
{
	uint32_t link = m_triggerTab[ idx].link;
	if (link >= m_size || m_triggerIndAr[ link] != idx)
	{
		throw strus::runtime_error( _TXT("bad trigger index (remove trigger)"));
	}
	m_triggerTab.remove( idx);
	if (link != m_size - 1)
	{
		m_eventAr[ link] = m_eventAr[ m_size-1];
		m_triggerIndAr[ link] = m_triggerIndAr[ m_size-1];
		m_triggerTab[ m_triggerIndAr[ link]].link = link;
	}
	--m_size;
}

Trigger const* EventTriggerTable::getTriggerPtr( uint32_t idx, uint32_t event) const
{
	const LinkedTrigger& trg = m_triggerTab[ idx];
	if (m_eventAr[ trg.link] == event)
	{
		return &m_triggerTab[ idx].trigger;
	}
	else
	{
		return 0;
	}
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
static inline void getTriggers_SSE4( Trigger const** results, std::size_t& nofresults, uint32_t event, const uint32_t* eventar, const uint32_t* triggerindar, const LinkedTriggerTable& triggertab, std::size_t arsize)
{
	__v4si *eventblkar = (__v4si*)eventar;		//... the SIMD search block (aligned to EventArrayMemoryAlignment byte blocks)
	uint32_t ii = 0;				//... index on 16 word blocks we handle with SSE
	uint32_t nn = ((uint32_t)arsize >> 4) << 2;	//... number of 16 word blocks we handle with SSE
	// We prepare the SIMD search key vector as 4 times the value of the needle
	__v4si event4 = (__v4si)_mm_set_epi32( event, event, event, event);

	for (;ii<nn; ii+=4)
	{
		// Compare 4 times 4 words = 16 32 bit uints and store the results in cmp[i].
		// cmp[i]: each [i] are 4 32 bit words, 0xFFffFFff true, 0x00000000 false:
		__v4si cmp0 = __builtin_ia32_pcmpeqd128( event4, eventblkar[ ii + 0]);
		__v4si cmp1 = __builtin_ia32_pcmpeqd128( event4, eventblkar[ ii + 1]);
		__v4si cmp2 = __builtin_ia32_pcmpeqd128( event4, eventblkar[ ii + 2]);
		__v4si cmp3 = __builtin_ia32_pcmpeqd128( event4, eventblkar[ ii + 3]);
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
			results[ nofresults++] = &triggertab[ triggerindar[ idx]].trigger;
			// and mask out the visited match from the mask with the matches:
			res ^= (1 << tz);
		}
	}
	// The rest of the events modulo 16, we handle in the standard way:
	ii <<= 2;
	for (; ii < arsize; ++ii)
	{
		if (eventar[ii] == event)
		{
			results[ nofresults++] = &triggertab[ triggerindar[ ii]].trigger;
		}
	}
}
#endif

void EventTriggerTable::getTriggers( TriggerRefList& triggers, uint32_t event) const
{
	// The following implementation looks a little bit funny, but it is 
	// crucial for the overall performance that this method is vectorizable.
	if (!event) return;
	Trigger const** tar = triggers.reserve( m_size);
	std::size_t nofresults = 0;

#if __GNUC__ >= 4
	const uint32_t* eventAr = (const uint32_t*)__builtin_assume_aligned( m_eventAr, EventArrayMemoryAlignment);
#else
	const uint32_t* eventAr = m_eventAr;
#endif
#ifdef STRUS_USE_SSE_SCAN_TRIGGERS
	getTriggers_SSE4( tar, nofresults, event, eventAr, m_triggerIndAr, m_triggerTab, m_size);
#else
	uint32_t wi=0;
	for (; wi<m_size; ++wi)
	{
		if (eventAr[ wi] == event)
		{
			tar[ nofresults++] = &m_triggerTab[ m_triggerIndAr[ wi]].trigger;
		}
	}
#endif
	triggers.commit_reserved( nofresults);
}

void ProgramTable::defineEventFrequency( uint32_t eventid, double df)
{
	if (df <= std::numeric_limits<double>::epsilon())
	{
		throw strus::runtime_error(_TXT("illegal value for df (must be positive)"));
	}
	m_frequencyMap[ eventid] = df;
}

uint32_t ProgramTable::createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_)
{
	return 1+m_programTable.add( Program( positionRange_, actionSlotDef_));
}

void ProgramTable::createTrigger( uint32_t programidx, uint32_t event, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable, float weight)
{
	Program& program = m_programTable[ programidx-1];
	m_triggerList.push( program.triggerListIdx, TriggerDef( event, sigtype, sigval, variable, weight));
	m_eventOccurrenceMap[ event] += 1;
}

void ProgramTable::defineProgramResult( uint32_t programidx, uint32_t eventid, uint32_t resultHandle)
{
	Program& program = m_programTable[ programidx-1];
	program.slotDef.event = eventid;
	program.slotDef.resultHandle = resultHandle;
}

void ProgramTable::defineEventProgramAlt( uint32_t eventid, uint32_t programidx, uint32_t past_eventid)
{
	EventProgamTriggerMap::iterator ei = m_eventProgamTriggerMap.find( eventid);
	if (ei == m_eventProgamTriggerMap.end())
	{
		uint32_t prglist = 0;
		m_programTriggerList.push( prglist, ProgramTrigger( programidx, past_eventid));
		m_eventProgamTriggerMap[ eventid] = prglist;
	}
	else
	{
		m_programTriggerList.push( ei->second, ProgramTrigger( programidx, past_eventid));
	}
	EventOccurrenceMap::iterator ci = m_keyOccurrenceMap.find( eventid);
	m_keyOccurrenceMap[ eventid] += 1;
	if (past_eventid)
	{
		m_keyOccurrenceMap[ past_eventid] -= 1;
		m_stopWordSet.insert( past_eventid);
	}
}

void ProgramTable::defineEventProgram( uint32_t eventid, uint32_t programidx)
{
	defineEventProgramAlt( eventid, programidx, 0);
	++m_totalNofPrograms;
}

uint32_t ProgramTable::getEventProgramList( uint32_t eventid) const
{
	EventProgamTriggerMap::const_iterator ei = m_eventProgamTriggerMap.find( eventid);
	return ei == m_eventProgamTriggerMap.end() ? 0:ei->second;
}

const ProgramTrigger* ProgramTable::nextProgramPtr( uint32_t& programlist) const
{
	return m_programTriggerList.nextptr( programlist);
}

double ProgramTable::calcEventWeight( uint32_t eventid) const
{
	double kf = 1.0;
	FrequencyMap::const_iterator fi = m_frequencyMap.find( eventid);
	if (fi != m_frequencyMap.end() && fi->second > 0.0)
	{
		kf = fi->second;
	}
	EventOccurrenceMap::const_iterator ki = m_keyOccurrenceMap.find( eventid);
	if (ki != m_keyOccurrenceMap.end() && ki->second > 0.0)
	{
		kf *= ki->second;
	}
	return kf;
}

uint32_t ProgramTable::getAltEventId( uint32_t eventid, uint32_t triggerListIdx) const
{
	const TriggerDef* trigger;
	uint32_t eventid_selected = 0;
	uint32_t sigval_selected = 0;
	Trigger::SigType sigtype = Trigger::SigAny;

	while (0!=(trigger=m_triggerList.nextptr( triggerListIdx)))
	{
		switch (trigger->sigtype)
		{
			case Trigger::SigAny:
				// ... For SigAny all events are key events, so no alternative can be chosen
				return 0;
			case Trigger::SigSequence:
			case Trigger::SigWithin:
			{
				if (sigtype == trigger->sigtype)
				{
					if (sigval_selected < trigger->sigval && trigger->event != eventid)
					{
						eventid_selected = trigger->event;
						sigval_selected = trigger->sigval;
						sigtype = trigger->sigtype;
					}
				}
				else if (sigtype == Trigger::SigAny && trigger->event != eventid)
				{
					eventid_selected = trigger->event;
					sigval_selected = trigger->sigval;
					sigtype = trigger->sigtype;
				}
				else
				{
					return 0;
				}
			}
			case Trigger::SigDel:
				continue;
		}
	}
	return eventid_selected;
}

void ProgramTable::getDelimTokenStopWordSet( uint32_t triggerListIdx)
{
	const TriggerDef* trigger;
	while (0!=(trigger=m_triggerList.nextptr( triggerListIdx)))
	{
		if (trigger->sigtype == Trigger::SigDel)
		{
			m_stopWordSet.insert( trigger->event);
		}
	}
}

ProgramTable::Statistics ProgramTable::getProgramStatistics() const
{
	ProgramTable::Statistics rt;
	std::vector<unsigned int> koheap;

	// Get distribution of key events:
	EventProgamTriggerMap::const_iterator
		ei = m_eventProgamTriggerMap.begin(),
		ee = m_eventProgamTriggerMap.end();
	for (; ei != ee; ++ei)
	{
		uint32_t eventid = ei->first;
		EventOccurrenceMap::const_iterator ki = m_keyOccurrenceMap.find( eventid);
		if (ki != m_keyOccurrenceMap.end())
		{
			koheap.push_back( ki->second);
		}
	}
	std::make_heap( koheap.begin(), koheap.end());
	while (koheap.size())
	{
		rt.keyEventDist.push_back( koheap.front());
		std::pop_heap( koheap.begin(), koheap.end());
		koheap.pop_back();
	}
	// Get list of stop events:
	std::set<uint32_t>::const_iterator si = m_stopWordSet.begin(), se = m_stopWordSet.end();
	for (; si != se; ++si)
	{
		rt.stopWordSet.push_back( *si);
	}
	return rt;
}

void ProgramTable::optimize( OptimizeOptions& opt)
{
	// Evaluate the key event identifiers to replace:
	std::vector<uint32_t> eventsToMove;
	{
		EventProgamTriggerMap::const_iterator
			ei = m_eventProgamTriggerMap.begin(),
			ee = m_eventProgamTriggerMap.end();
		for (; ei != ee; ++ei)
		{
			uint32_t eventid = ei->first;
	
			EventOccurrenceMap::const_iterator ki = m_keyOccurrenceMap.find( eventid);
			if (ki != m_keyOccurrenceMap.end()
			&&  ki->second >= (float)m_totalNofPrograms * opt.stopwordOccurrenceFactor)
			{
				eventsToMove.push_back( eventid);
			}
		}
	}
	// Get the event list of the event candidate, check for an alternative key events,
	// replace the program list of the event:
	std::vector<uint32_t>::const_iterator mi = eventsToMove.begin(), me = eventsToMove.end();
	for (; mi != me; ++mi)
	{
		// Build a new program list for each visited event. Because alternativeEventId does not
		// return the identity, we can just add the new relations and replace the old
		// program list with the new one:

		EventProgamTriggerMap::iterator ei = m_eventProgamTriggerMap.find( *mi);
		if (ei == m_eventProgamTriggerMap.end()) continue;
		uint32_t eventid = ei->first;
		uint32_t prglist = ei->second;
		uint32_t new_prglist = 0;
		double weight = calcEventWeight( eventid);

		const ProgramTrigger* programTrigger;
		while (0!=(programTrigger=m_programTriggerList.nextptr( prglist)))
		{
			Program& program = m_programTable[ programTrigger->programidx-1];
			uint32_t alt_eventid;
			if (!programTrigger->past_eventid
			&&  opt.maxRange >= program.positionRange
			&&  0!=(alt_eventid = getAltEventId( eventid, program.triggerListIdx))
			&&  weight > (calcEventWeight( alt_eventid) * opt.weightFactor))
			{
				defineEventProgramAlt( alt_eventid, programTrigger->programidx, eventid);
				getDelimTokenStopWordSet( program.triggerListIdx);
			}
			else
			{
				m_programTriggerList.push( new_prglist, *programTrigger);
			}
		}
		// Set new program list
		m_programTriggerList.remove( prglist);
		if (new_prglist != 0)
		{
			ei->second = new_prglist;
		}
		else
		{
			m_eventProgamTriggerMap.erase( ei);
		}
	}
}


StateMachine::StateMachine( const ProgramTable* programTable_)
	:m_programTable(programTable_)
	,m_curpos(0)
	,m_nofProgramsInstalled(0)
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
	,m_stopWordsEventList(o.m_stopWordsEventList)
	,m_nofProgramsInstalled(o.m_nofProgramsInstalled)
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

void StateMachine::fireTrigger(
	ActionSlot& slot, const Trigger& trigger, const EventData& data,
	DisposeRuleList& disposeRuleList, EventStructList& followList)
{
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
			if (trigger.sigval() == slot.value)
			{
				slot.value = trigger.sigval();
				if (slot.count > 0)
				{
					--slot.count;
					match = (slot.count == 0);
				}
				else
				{
					match = true;
				}
				finished = (slot.value == 0);
				takeEventData = true;
			}
			break;
		case Trigger::SigWithin:
		{
			if ((trigger.sigval() & slot.value) != 0)
			{
				slot.value &= ~trigger.sigval();
				if (slot.count > 0)
				{
					--slot.count;
					match = (slot.count == 0);
				}
				else
				{
					match = true;
				}
				finished = (slot.value == 0);
				takeEventData = true;
			}
			break;
		}
		case Trigger::SigDel:
		{
			slot.count = 0;
			slot.value = 0;
			disposeRuleList.add( slot.rule);
			return;
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
				EventStruct followEventData( EventData( slot.start_origpos, eventOrigSize, slot.start_ordpos, rule.eventDataReferenceIdx), slot.event);
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

void StateMachine::doTransition( uint32_t event, const EventData& data)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "call doTransition( event=" << event << ", ordpos=" << data.ordpos << ")" << std::endl;
#endif
	// Some logging:
	m_nofOpenPatterns += m_eventTriggerTable.size();

	enum {NofTriggers=1024,NofEventStruct=1024,NofDisposeRules=1024};
	Trigger const* trigger_alloca[ NofTriggers];
	EventStruct followList_alloca[ NofEventStruct];
	uint32_t disposeRuleList_alloca[ NofDisposeRules];

	// Install triggered programs:
	uint32_t programlist = m_programTable->getEventProgramList( event);
	const ProgramTrigger* programTrigger;
#ifdef STRUS_LOWLEVEL_DEBUG
	uint32_t icnt = 0;
#endif
	while (0!=(programTrigger=m_programTable->nextProgramPtr( programlist)))
	{
		installProgram( *programTrigger);
#ifdef STRUS_LOWLEVEL_DEBUG
		++icnt;
#endif
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "programs installed " << icnt << ", nof rules used " << m_ruleTable.used_size() << ", triggers active " << m_eventTriggerTable.size() << std::endl;
#endif

	// Process the event and all follow events triggered:
	EventStructList followList( followList_alloca, NofEventStruct);
	followList.add( EventStruct( data, event));
	std::size_t ei = followList.first();
	if (followList[ei].data.subdataref)
	{
		referenceEventData( followList[ei].data.subdataref);
	}
	for (; ei < followList.first() + followList.size(); ++ei)
	{
		EventTriggerTable::TriggerRefList triggers( trigger_alloca, NofTriggers);
		DisposeRuleList disposeRuleList( disposeRuleList_alloca, NofDisposeRules);

		EventStruct follow = followList[ ei];
		m_eventTriggerTable.getTriggers( triggers, follow.eventid);
		m_nofPatternsTriggered += triggers.size();
		EventTriggerTable::TriggerRefList::const_iterator
			ti = triggers.begin(), te = triggers.end();
		for (; ti != te; ++ti)
		{
			const Trigger& trigger = **ti;
			ActionSlot& slot = m_actionSlotTable[ trigger.slot()];

			fireTrigger( slot, trigger, follow.data, disposeRuleList, followList);
		}
		DisposeRuleList::const_iterator
			di = disposeRuleList.begin(), de = disposeRuleList.end();
		for (; di != de; ++di)
		{
			deactivateRule( *di);
		}

		// Keep all stopword events to feed slots of programs triggered by a key event 
		// that is not the first appearing:
		if (m_programTable->isStopWord( follow.eventid))
		{
			m_stopWordsEventList.push_back( EventStruct( follow.data, event));
		}
		else if (follow.data.subdataref)
		{
			disposeEventDataReference( follow.data.subdataref);
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "current state after transition [" << followList.size() << "]: nof rules used " << m_ruleTable.used_size() << ", nof active triggers " << m_eventTriggerTable.size() << std::endl;
#endif
}

void StateMachine::setCurrentPos( uint32_t pos)
{
	if (pos < m_curpos) throw strus::runtime_error(_TXT("illegal set of current pos (positions not ascending)"));
	m_curpos = pos;
#ifdef STRUS_LOWLEVEL_DEBUG
	int disposeCount = 0;
#endif
	while (!m_ruleDisposeQueue.empty() && m_ruleDisposeQueue.front().pos < pos)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		disposeCount += 1;
#endif
		disposeRule( m_ruleDisposeQueue.front().idx);
		std::pop_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
		m_ruleDisposeQueue.pop_back();
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "set current position " << pos << ", rules deleted " << disposeCount << ", nof rules used " << m_ruleTable.used_size() << ", nof active triggers " << m_eventTriggerTable.size() << std::endl;
#endif
}

void StateMachine::installProgram( const ProgramTrigger& programTrigger)
{
	const Program& program = (*m_programTable)[ programTrigger.programidx];
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
	m_nofProgramsInstalled += 1;

	// Trigger past stopwords events matching if the key event is not first event expected:
	if (programTrigger.past_eventid)
	{
		std::vector<EventStruct>::const_reverse_iterator
			ei = m_stopWordsEventList.rbegin(), ee = m_stopWordsEventList.rend();
		if (ei != ee)
		{
			uint32_t start_position = ei->data.origpos > program.positionRange ? (ei->data.origpos - program.positionRange):1;
			for (;ei != ee && ei->data.origpos > start_position; ++ei)
			{
				if (programTrigger.past_eventid == ei->eventid)
				{
					break;
				}
			}
			if (ei != ee && ei->data.origpos > start_position)
			{
				ActionSlot& slot = m_actionSlotTable[ rule.actionSlotIdx-1];

				enum {NofDisposeRules=128};
				EventStructList followList;
				uint32_t disposeRuleList_alloca[ NofDisposeRules];
				DisposeRuleList disposeRuleList( disposeRuleList_alloca, NofDisposeRules);

				// Replay all stopword events starting with the key event found
				// to get the slot of the triggered rule into its state expected:
				std::vector<EventStruct>::const_iterator
					ai = ei.base(), ae = m_stopWordsEventList.end();
				for (--ai; ai != ae; ++ai)
				{
					uint32_t triggerlist = rule.eventTriggerListIdx;
					uint32_t trigger;
					while (m_eventTriggerList.next( triggerlist, trigger))
					{
						Trigger const* tp = m_eventTriggerTable.getTriggerPtr( trigger, ai->eventid);
						if (tp)
						{
							fireTrigger( slot, *tp, ai->data, disposeRuleList, followList);
						}
					}
					DisposeRuleList::const_iterator
						di = disposeRuleList.begin(), de = disposeRuleList.end();
					for (; di != de; ++di)
					{
						deactivateRule( *di);
					}
					if (!rule.isActive()) break;
				}
				if (followList.size())
				{
					throw strus::runtime_error(_TXT("internal: encountered past trigger with follow"));
					// ... only rules that really need the alternative event triggered should have an alternative key event
				}
			}
		}
	}
}



