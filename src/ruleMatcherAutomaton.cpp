/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ruleMatcherAutomaton.hpp"
#include "strus/base/malloc.hpp"
#include <limits>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <new>
#include <stdint.h>

#if !defined (__APPLE__) && !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(_WIN32) && UINTPTR_MAX != 0xffffffff && !defined(__clang__) 
#ifdef __SSE__
#include <emmintrin.h>
#define STRUS_USE_SSE_SCAN_TRIGGERS
#endif
#define HAVE_BUILTIN_ASSUME_ALIGNED
#endif

#if defined(__clang__) || defined(__GNUC__) 
#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)
#endif

using namespace strus;

static inline uint32_t evhash( uint32_t a)
{
	a += ~(a>>5);
	a +=  (a<<3);
	a ^=  (a>>4);
	return a;
}

EventTriggerTable::EventTriggerTable()
	:m_nofTriggers(0){}
EventTriggerTable::EventTriggerTable( const EventTriggerTable& o)
	:m_triggerTab(o.m_triggerTab),m_nofTriggers(o.m_nofTriggers)
{
	std::size_t htidx=0;
	for (; htidx != EventHashTabSize; ++htidx)
	{
		TriggerInd& rec = m_triggerIndAr[ htidx];
		const TriggerInd& rec_o = o.m_triggerIndAr[ htidx];

		if (rec_o.m_allocsize)
		{
			rec.expand( rec_o.m_allocsize);
		}
		std::memcpy( rec.m_eventAr, rec_o.m_eventAr, rec_o.m_size * sizeof(*rec.m_eventAr));
		std::memcpy( rec.m_ar, rec_o.m_ar, rec_o.m_size * sizeof(*rec.m_ar));
		rec.m_size = rec_o.m_size;
	}
}

EventTriggerTable::TriggerInd::~TriggerInd()
{
	if (m_eventAr)
	{
		std::free( m_eventAr);
	}
	if (m_ar)
	{
		std::free( m_ar);
	}
}

void EventTriggerTable::TriggerInd::clear()
{
	if (m_eventAr) strus::aligned_free( m_eventAr);
	if (m_ar) std::free( m_ar);
	std::memset( this, 0, sizeof(*this));
}

enum {EventArrayMemoryAlignment=64};
void EventTriggerTable::TriggerInd::expand( uint32_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::runtime_error( "illegal call of EventTriggerTable::TriggerInd::expand");
	}
	uint32_t* war = (uint32_t*)strus::aligned_malloc( newallocsize * sizeof(uint32_t), EventArrayMemoryAlignment);
	if (!war) throw std::bad_alloc();
	std::memcpy( war, m_eventAr, m_size * sizeof(uint32_t));
	if (m_eventAr) strus::aligned_free( m_eventAr);
	m_eventAr = war;
	uint32_t* far = (uint32_t*)std::realloc( m_ar, newallocsize * sizeof(uint32_t));
	if (!far) throw std::bad_alloc();
	m_ar = far;
	m_allocsize = newallocsize;
}

void EventTriggerTable::clear()
{
	std::size_t hi = 0, he = EventHashTabSize;
	for (;  hi != he; ++hi) m_triggerIndAr[hi].clear();
	m_triggerTab.clear();
	m_nofTriggers = 0;
}

static inline uint32_t linkid( uint32_t htidx, uint32_t elidx)
{
	if (elidx >= (1U<<EventTriggerTable::EventHashTabIdxShift)) throw strus::runtime_error(_TXT("too many event triggers defined, maximum is %u"), (1U<<EventTriggerTable::EventHashTabIdxShift));
	return elidx + (htidx << EventTriggerTable::EventHashTabIdxShift);
}

uint32_t EventTriggerTable::add( const EventTrigger& et)
{
	uint32_t htidx = evhash( et.event) & EventHashTabIdxMask;
	TriggerInd& rec = m_triggerIndAr[ htidx];
	if (rec.m_size == rec.m_allocsize)
	{
		if (rec.m_allocsize >= (1U<<31))
		{
			throw std::runtime_error(_TXT("too many elements in event trigger table"));
		}
		rec.expand( rec.m_allocsize?(rec.m_allocsize*2):BlockSize);
	}
	rec.m_eventAr[ rec.m_size] = et.event;
	uint32_t rt = rec.m_ar[ rec.m_size] = m_triggerTab.add( LinkedTrigger( linkid( htidx,rec.m_size), et.trigger));
	++rec.m_size;
	++m_nofTriggers;
	return rt;
}

void EventTriggerTable::remove( uint32_t idx)
{
	uint32_t link = m_triggerTab[ idx].link;
	uint32_t htidx = (link >> EventHashTabIdxShift) & EventHashTabIdxMask;
	uint32_t aridx = link & ((1 << EventHashTabIdxShift) -1);
	TriggerInd& rec = m_triggerIndAr[ htidx];
	if (aridx >= rec.m_size || rec.m_ar[ aridx] != idx)
	{
		throw std::runtime_error( _TXT("bad trigger index (remove trigger)"));
	}
	m_triggerTab.remove( idx);
	if (aridx != rec.m_size - 1)
	{
		rec.m_eventAr[ aridx] = rec.m_eventAr[ rec.m_size-1];
		rec.m_ar[ aridx] = rec.m_ar[ rec.m_size-1];
		m_triggerTab[ rec.m_ar[ aridx]].link = link;
	}
	--rec.m_size;
	--m_nofTriggers;
}

uint32_t EventTriggerTable::getTriggerEventId( uint32_t triggeridx) const
{
	uint32_t link = m_triggerTab[ triggeridx].link;
	uint32_t htidx = (link >> EventHashTabIdxShift) & EventHashTabIdxMask;
	uint32_t aridx = link & ((1 << EventHashTabIdxShift) -1);
	return m_triggerIndAr[ htidx].m_eventAr[ aridx];
	
}

Trigger const* EventTriggerTable::getTriggerPtr( uint32_t idx) const
{
	return &m_triggerTab[ idx].trigger;
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
	uint32_t htidx = evhash( event) & EventHashTabIdxMask;
	const TriggerInd& rec = m_triggerIndAr[ htidx];
	Trigger const** tar = triggers.reserve( rec.m_size);
	std::size_t nofresults = 0;

#if __GNUC__ >= 4 && defined(HAVE_BUILTIN_ASSUME_ALIGNED)
	const uint32_t* eventAr = (const uint32_t*)__builtin_assume_aligned( rec.m_eventAr, EventArrayMemoryAlignment);
#else
	const uint32_t* eventAr = rec.m_eventAr;
#endif
#ifdef STRUS_USE_SSE_SCAN_TRIGGERS
	getTriggers_SSE4( tar, nofresults, event, eventAr, rec.m_ar, m_triggerTab, rec.m_size);
#else
	uint32_t wi=0;
	for (; wi<rec.m_size; ++wi)
	{
		if (eventAr[ wi] == event)
		{
			tar[ nofresults++] = &m_triggerTab[ rec.m_ar[ wi]].trigger;
		}
	}
#endif
	triggers.commit_reserved( nofresults);
}

void ProgramTable::defineEventFrequency( uint32_t eventid, double df)
{
	if (df <= std::numeric_limits<double>::epsilon())
	{
		throw std::runtime_error( _TXT("illegal value for df (must be positive)"));
	}
	m_frequencyMap[ eventid] = df;
}

uint32_t ProgramTable::createProgram( uint32_t positionRange_, const ActionSlotDef& actionSlotDef_)
{
	return 1+m_programMap.add( Program( positionRange_, actionSlotDef_));
}

void ProgramTable::createTrigger( uint32_t programidx, uint32_t event, bool isKeyEvent, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable)
{
	Program& program = m_programMap[ programidx-1];
	m_triggerList.push( program.triggerListIdx, TriggerDef( event, isKeyEvent, sigtype, sigval, variable));
	m_eventOccurrenceMap[ event] += 1;
}

void ProgramTable::doneProgram( uint32_t programidx)
{
	Program& program = m_programMap[ programidx-1];
	uint32_t triggerListIdx = program.triggerListIdx;
	const TriggerDef* trigger;

	while (0!=(trigger=m_triggerList.nextptr( triggerListIdx)))
	{
		if (trigger->isKeyEvent)
		{
			defineEventProgram( trigger->event, programidx);
		}
	}
}

void ProgramTable::defineProgramResult( uint32_t programidx, uint32_t eventid, uint32_t resultHandle, uint32_t formatHandle)
{
	Program& program = m_programMap[ programidx-1];
	program.slotDef.event = eventid;
	program.slotDef.resultHandle = resultHandle;
	program.slotDef.formatHandle = formatHandle;
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
			case Trigger::SigAnd:
			{
				if (sigtype == (Trigger::SigType)trigger->sigtype)
				{
					if (trigger->event != eventid)
					{
						eventid_selected = trigger->event;
						sigtype = (Trigger::SigType)trigger->sigtype;
					}
				}
				else if (sigtype == Trigger::SigAny && trigger->event != eventid)
				{
					eventid_selected = trigger->event;
					sigtype = (Trigger::SigType)trigger->sigtype;
				}
				else
				{
					return 0;
				}
			}
			case Trigger::SigSequence:
			case Trigger::SigSequenceImm:
			case Trigger::SigWithin:
			{
				if (sigtype == (Trigger::SigType)trigger->sigtype)
				{
					if (sigval_selected < trigger->sigval && trigger->event != eventid)
					{
						eventid_selected = trigger->event;
						sigval_selected = trigger->sigval;
						sigtype = (Trigger::SigType)trigger->sigtype;
					}
				}
				else if (sigtype == Trigger::SigAny && trigger->event != eventid)
				{
					eventid_selected = trigger->event;
					sigval_selected = trigger->sigval;
					sigtype = (Trigger::SigType)trigger->sigtype;
				}
				else if (sigtype == Trigger::SigSequenceImm && trigger->sigtype == Trigger::SigSequence)
				{
					if (sigval_selected < trigger->sigval && trigger->event != eventid)
					{
						eventid_selected = trigger->event;
						sigval_selected = trigger->sigval;
						sigtype = (Trigger::SigType)trigger->sigtype;
					}
				}
				else
				{
					return 0;
				}
				break;
			}
			case Trigger::SigDel:
				break;
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

void ProgramTable::eliminateUnusedEvents()
{
	std::set<uint32_t> usedEvents;
	std::set<uint32_t> programs;
	EventProgamTriggerMap::const_iterator
		ei = m_eventProgamTriggerMap.begin(),
		ee = m_eventProgamTriggerMap.end();
	for (; ei != ee; ++ei)
	{
		uint32_t prglist = ei->second;
		const ProgramTrigger* programTrigger;
		while (0!=(programTrigger=m_programTriggerList.nextptr( prglist)))
		{
			programs.insert( programTrigger->programidx);
			const Program& program = m_programMap[ programTrigger->programidx-1];
			uint32_t triggerlistitr = program.triggerListIdx;
			const TriggerDef* trigger;
			while (0!=(trigger = m_triggerList.nextptr( triggerlistitr)))
			{
				usedEvents.insert( trigger->event);
			}
		}
	}
	std::set<uint32_t>::const_iterator gi = programs.begin(), ge = programs.end();
	for (; gi != ge; ++gi)
	{
		Program& program = m_programMap[ *gi-1];
		if (usedEvents.find( program.slotDef.event) == usedEvents.end())
		{
			program.slotDef.event = 0;
		}
	}
}

void ProgramTable::optimize( OptimizeOptions& opt)
{
	eliminateUnusedEvents();

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
			Program& program = m_programMap[ programTrigger->programidx-1];
			uint32_t alt_eventid = getAltEventId( eventid, program.triggerListIdx);
			if (!alt_eventid)
			{
				m_programTriggerList.push( new_prglist, *programTrigger);
			}
			else
			{
				double alt_weight = calcEventWeight( alt_eventid) * opt.weightFactor;
				if (!programTrigger->past_eventid
				&&  opt.maxRange >= program.positionRange
				&&  weight > alt_weight)
				{
					defineEventProgramAlt( alt_eventid, programTrigger->programidx, eventid);
					getDelimTokenStopWordSet( program.triggerListIdx);
				}
				else
				{
					m_programTriggerList.push( new_prglist, *programTrigger);
				}
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


StateMachine::StateMachine( const ProgramTable* programTable_, DebugTraceContextInterface* debugtrace_)
	:m_debugtrace(debugtrace_)
	,m_programTable(programTable_)
	,m_curpos(0)
	,m_nofProgramsInstalled(0)
	,m_nofAltKeyProgramsInstalled(0)
	,m_nofSignalsFired(0)
	,m_nofOpenPatterns(0.0)
	,m_timestmp(0)
{
	std::memset( m_disposeWindow, 0, sizeof(m_disposeWindow));
	std::make_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	std::memset( m_observeEvents, 0, sizeof(m_observeEvents));
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
	,m_disposeRuleList(o.m_disposeRuleList)
	,m_ruleDisposeQueue(o.m_ruleDisposeQueue)
	,m_stopWordsEventLogMap(o.m_stopWordsEventLogMap)
	,m_nofProgramsInstalled(o.m_nofProgramsInstalled)
	,m_nofAltKeyProgramsInstalled(o.m_nofAltKeyProgramsInstalled)
	,m_nofSignalsFired(o.m_nofSignalsFired)
	,m_nofOpenPatterns(o.m_nofOpenPatterns)
	,m_timestmp(o.m_timestmp)
{
	std::memcpy( m_disposeWindow, o.m_disposeWindow, sizeof(m_disposeWindow));
	std::make_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	std::memcpy( m_observeEvents, o.m_observeEvents, sizeof(m_observeEvents));
}

void StateMachine::addObserveEvent( uint32_t event)
{
	std::size_t ei = 0, ee = MaxNofObserveEvents;
	for (; ei < ee && m_observeEvents[ei] != 0; ++ei){}
	if (ei == ee) throw std::runtime_error( _TXT("too many observe events defined"));
}

bool StateMachine::isObservedEvent( uint32_t event) const
{
	uint32_t const* ev = m_observeEvents;
	if (*ev)
	{
		for (; *ev && *ev != event; ++ev){}
		if (*ev) return true;
	}
	else
	{
		return true;
	}
	return false;
}

void StateMachine::clear()
{
	m_eventTriggerTable.clear();
	m_actionSlotTable.clear();
	m_eventTriggerList.clear();
	m_eventItemList.clear();
	m_eventDataReferenceTable.clear();
	m_ruleTable.clear();
	m_results.clear();
	m_curpos = 0;
	std::memset( m_disposeWindow, 0, sizeof(m_disposeWindow));
	m_disposeRuleList.clear();
	m_ruleDisposeQueue.clear();
	std::make_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	m_stopWordsEventLogMap.clear();
	m_nofProgramsInstalled = 0;
	m_nofAltKeyProgramsInstalled = 0;
	m_nofSignalsFired = 0;
	m_nofOpenPatterns = 0;
	m_timestmp = 0;
}

uint32_t StateMachine::createRule( uint32_t expiryOrdpos)
{
	uint32_t rt = m_ruleTable.add( Rule( expiryOrdpos));
	defineDisposeRule( expiryOrdpos, rt);
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
		throw std::runtime_error( _TXT("illegal free of event data reference"));
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

void StateMachine::fireSignal(
	ActionSlot& slot, const Trigger& trigger, const EventData& data,
	DisposeRuleList& disposeRuleList, EventStructList& followList)
{
	Rule& rule = m_ruleTable[ slot.rule];
	bool match = false;
	bool takeEventData = false;
	bool finished = false;
	++m_nofSignalsFired;

	if (UNLIKELY(!!m_debugtrace))
	{
		bool observed = isObservedEvent( slot.event);
		if (observed)
		{
			m_debugtrace->event( "firesignal", "rule %d sig %s val %x slot %d #%d",
						(int)slot.rule,Trigger::sigTypeName( trigger.sigtype()),
						trigger.sigval(),(int)slot.value,(int)slot.count);
		}
	}
	switch (trigger.sigtype())
	{
		case Trigger::SigAny:
			takeEventData = true;
			if (slot.count > 0)
			{
				match = true;
				--slot.count;
				finished = (slot.count == 0);
				if (slot.end_ordpos < data.end_ordpos)
				{
					slot.end_ordpos = data.end_ordpos;
				}
			}
			break;
		case Trigger::SigAnd:
			if (slot.count > 0)
			{
				if (!slot.value)
				{
					slot.value = data.start_ordpos;
					if (slot.end_ordpos > data.end_ordpos)
					{
						slot.end_ordpos = data.end_ordpos;
					}
				}
				if (slot.value == data.start_ordpos)
				{
					match = true;
					--slot.count;
					finished = (slot.count == 0);
					takeEventData = true;
				}
			}
			break;
		case Trigger::SigSequence:
			if (trigger.sigval() == slot.value && slot.end_ordpos <= data.start_ordpos)
			{
				slot.end_ordpos = data.end_ordpos;
				slot.value = trigger.sigval()-1;
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
		case Trigger::SigSequenceImm:
			if (trigger.sigval() == slot.value && slot.end_ordpos == data.start_ordpos)
			{
				slot.end_ordpos = data.end_ordpos;
				slot.value = trigger.sigval()-1;
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
			if ((trigger.sigval() & slot.value) != 0 && slot.end_ordpos <= data.start_ordpos)
			{
				slot.end_ordpos = data.end_ordpos;
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
	if (UNLIKELY(!!m_debugtrace))
	{
		bool observed = isObservedEvent( slot.event);
		if (observed)
		{
			m_debugtrace->event( "action", "match %s finish %s data %s",
						match?"yes":"no", finished?"yes":"no", takeEventData?"yes":"no");
		}
	}
	if (takeEventData)
	{
		if (trigger.variable())
		{
			EventItem item( trigger.variable(), data);
			if (!rule.eventDataReferenceIdx)
			{
				rule.eventDataReferenceIdx = createEventData();
			}
			appendEventData( rule.eventDataReferenceIdx, item);
		}
		else if (data.subdataref)
		{
			if (!rule.eventDataReferenceIdx)
			{
				rule.eventDataReferenceIdx = createEventData();
			}
			joinEventData( rule.eventDataReferenceIdx, data.subdataref);
		}
		if (slot.start_ordpos == 0)
		{
			slot.start_ordpos = data.start_ordpos;
			slot.start_origseg = data.start_origseg;
			slot.start_origpos = data.start_origpos;
		}
		else if (slot.start_ordpos > data.start_ordpos)
		{
			slot.start_ordpos = data.start_ordpos;
			if (slot.start_origseg > data.start_origseg || (slot.start_origseg == data.start_origseg && slot.start_origpos > data.start_origpos))
			{
				slot.start_origseg = data.start_origseg;
				slot.start_origpos = data.start_origpos;
			}
		}
	}
	if (match)
	{
		if (!rule.done)
		{
			if (slot.event)
			{
				EventStruct followEventData( EventData( slot.start_origseg, slot.start_origpos, data.end_origseg, data.end_origpos, slot.start_ordpos, slot.end_ordpos, rule.eventDataReferenceIdx, slot.formatHandle), slot.event);
				if (rule.eventDataReferenceIdx)
				{
					referenceEventData( rule.eventDataReferenceIdx);
				}
				followList.add( followEventData);
			}
			if (slot.resultHandle)
			{
				m_results.add( Result( slot.resultHandle, slot.formatHandle, rule.eventDataReferenceIdx, slot.start_ordpos, slot.end_ordpos, slot.start_origseg, slot.start_origpos, data.end_origseg, data.end_origpos));
				if (rule.eventDataReferenceIdx)
				{
					referenceEventData( rule.eventDataReferenceIdx);
				}
				if (UNLIKELY(!!m_debugtrace))
				{
					bool observed = isObservedEvent( slot.event);
					if (observed)
					{
						m_debugtrace->event( "action", "result %d", (int)slot.resultHandle);
					}
				}
			}
			rule.done = true;
			if (UNLIKELY(!!m_debugtrace))
			{
				bool observed = isObservedEvent( slot.event);
				if (observed)
				{
					m_debugtrace->event( "action", "done");
				}
			}
		}
		if (finished)
		{
			disposeRuleList.add( slot.rule);
		}
	}
}

void StateMachine::doTransition( uint32_t event, const EventData& data)
{
	if (UNLIKELY(!!m_debugtrace))
	{
		bool observed = isObservedEvent( event);
		if (observed)
		{
			m_debugtrace->event( "transition", "event %d pos %d end %d", (int)event,(int)data.start_ordpos, (int)data.end_ordpos);
		}
	}
	// Some logging:
	m_nofOpenPatterns += m_eventTriggerTable.nofTriggers();

	enum {NofTriggers=1024,NofEventStruct=1024,NofDisposeRules=1024};
	Trigger const* trigger_alloca[ NofTriggers];
	EventStruct followList_alloca[ NofEventStruct];
	uint32_t disposeRuleList_alloca[ NofDisposeRules];

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

		// Fire triggers waiting for this event:
		m_eventTriggerTable.getTriggers( triggers, follow.eventid);
		EventTriggerTable::TriggerRefList::const_iterator
			ti = triggers.begin(), te = triggers.end();
		for (; ti != te; ++ti)
		{
			const Trigger& trigger = **ti;
			ActionSlot& slot = m_actionSlotTable[ trigger.slot()];

			fireSignal( slot, trigger, follow.data, disposeRuleList, followList);
		}
		// Install triggered programs:
		installEventPrograms( follow.eventid, follow.data, followList, disposeRuleList);

		// Deactivate rules that got a signal to be deleted:
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
			m_stopWordsEventLogMap[ follow.eventid] = EventLog( follow.data, ++m_timestmp);
		}
		// Release event data not referenced by any active rule:
		else if (follow.data.subdataref)
		{
			disposeEventDataReference( follow.data.subdataref);
		}
	}
	if (UNLIKELY(!!m_debugtrace))
	{
		bool observed = isObservedEvent( event);
		if (observed)
		{
			m_debugtrace->event( "transition", "event %d pos %d end %d", (int)event,(int)data.start_ordpos, (int)data.end_ordpos);
		}
	}
	if (UNLIKELY(!!m_debugtrace))
	{
		bool observed = isObservedEvent( event);
		if (observed)
		{
			m_debugtrace->event( "state", "size %d used %d triggers %d", (int)followList.size(),(int)m_ruleTable.used_size(), (int)m_eventTriggerTable.nofTriggers());
		}
	}
}

void StateMachine::defineDisposeRule( uint32_t pos, uint32_t ruleidx)
{
	if (pos < m_curpos)
	{
		throw strus::runtime_error(_TXT("illegal definition of dispose rule at position %u (smaller than current %u)"), pos, m_curpos);
	}
	if (pos < m_curpos + DisposeWindowSize)
	{
		uint32_t widx = pos % DisposeWindowSize;
		m_disposeRuleList.push( m_disposeWindow[ widx], ruleidx);
	}
	else
	{
		m_ruleDisposeQueue.push_back( DisposeEvent( pos, ruleidx));
		std::push_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
	}
}

void StateMachine::setCurrentPos( uint32_t pos)
{
	if (pos < m_curpos)
	{
		throw strus::runtime_error(_TXT("illegal definition of current pos (positions not ascending: %u ... %u)"), m_curpos, pos);
	}
	if (m_curpos == pos) return;
	int disposeCount = 0;

	std::size_t wcnt=0;
	for (; wcnt < DisposeWindowSize && m_curpos < pos; ++wcnt,++m_curpos)
	{
		uint32_t rulelist;
		uint32_t widx = m_curpos % DisposeWindowSize;
		if (widx == 0)
		{
			while (!m_ruleDisposeQueue.empty() && m_ruleDisposeQueue.front().pos < m_curpos + DisposeWindowSize)
			{
				wcnt = 0;
				m_disposeRuleList.push( m_disposeWindow[ m_ruleDisposeQueue.front().pos % DisposeWindowSize], m_ruleDisposeQueue.front().idx);
				std::pop_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
				m_ruleDisposeQueue.pop_back();
			}
		}
		if (0!=(rulelist = m_disposeWindow[ widx]))
		{
			uint32_t ruleidx;
			while (m_disposeRuleList.next( rulelist, ruleidx))
			{
				disposeRule( ruleidx);
				disposeCount += 1;
			}
			m_disposeWindow[ widx] = 0;
		}
	}
	if (m_curpos < pos)
	{
		m_curpos = pos;
		while (!m_ruleDisposeQueue.empty() && m_ruleDisposeQueue.front().pos < m_curpos)
		{
			disposeRule( m_ruleDisposeQueue.front().idx);
			disposeCount += 1;
			std::pop_heap( m_ruleDisposeQueue.begin(), m_ruleDisposeQueue.end());
			m_ruleDisposeQueue.pop_back();
		}
	}
	if (UNLIKELY(!!m_debugtrace))
	{
		m_debugtrace->event( "current", "pos %d deleted %d used %d active %d",
					(int)pos, (int)disposeCount, (int)m_ruleTable.used_size(), (int)m_eventTriggerTable.nofTriggers());
	}
}

void StateMachine::installEventPrograms( uint32_t event, const EventData& data, EventStructList& followList, DisposeRuleList& disposeRuleList)
{
	uint32_t programlist = m_programTable->getEventProgramList( event);
	const ProgramTrigger* programTrigger;
	uint32_t icnt = 0;

	while (0!=(programTrigger=m_programTable->nextProgramPtr( programlist)))
	{
		installProgram( event, *programTrigger, data, followList, disposeRuleList);
		++icnt;
	}
	if (UNLIKELY(!!m_debugtrace))
	{
		if (isObservedEvent( event))
		{
			m_debugtrace->event( "install", "programs %d used %d active %d",
					(int)icnt, (int)m_ruleTable.used_size(),
					(int)m_eventTriggerTable.nofTriggers());
		}
	}
}

static bool triggerDefNeedsInstall( const TriggerDef& triggerDef, const ActionSlot& slot)
{
	if ((Trigger::SigType)triggerDef.sigtype == Trigger::SigAny && slot.count > 1)
	{
		return true;
	}
	return false;
}

void StateMachine::installProgram( uint32_t keyevent, const ProgramTrigger& programTrigger, const EventData& data, EventStructList& followList, DisposeRuleList& disposeRuleList)
{
	const Program& program = (*m_programTable)[ programTrigger.programidx];
	if (data.start_ordpos + program.positionRange < m_curpos)
	{
		if (UNLIKELY(!!m_debugtrace))
		{
			if (isObservedEvent( keyevent))
			{
				m_debugtrace->event( "expired", "pos %d", (int)(data.start_ordpos + program.positionRange));
			}
		}
		return; /*rule cannot match anymore because of expired maximum position*/
	}
	uint32_t ruleidx = createRule( data.start_ordpos + program.positionRange);
	Rule& rule = m_ruleTable[ ruleidx];
	if (UNLIKELY(!!m_debugtrace))
	{
		if (isObservedEvent( keyevent))
		{
			m_debugtrace->event( "install", "event %d program %d rule %d pos %d", (int)keyevent, (int)programTrigger.programidx, (int)ruleidx, (int)data.start_ordpos);
		}
	}
	rule.actionSlotIdx =
		1+m_actionSlotTable.add(
			ActionSlot( program.slotDef.initsigval, program.slotDef.initcount,
					program.slotDef.event, ruleidx,
					program.slotDef.resultHandle, program.slotDef.formatHandle));

	ActionSlot& slot = m_actionSlotTable[ rule.actionSlotIdx-1];
	uint32_t program_triggerListItr = program.triggerListIdx;
	const TriggerDef* triggerDef;
	enum {MaxNofKeyTriggerDefs=32};
	const TriggerDef* keyTriggerDef[ MaxNofKeyTriggerDefs];
	std::size_t nofKeyTriggerDef = 0;
	bool hasKeyEvent = false;
	while (0!=(triggerDef=m_programTable->triggerList().nextptr( program_triggerListItr)))
	{
		bool doInstall = false;
		if (keyevent == triggerDef->event)
		{
			if (nofKeyTriggerDef < MaxNofKeyTriggerDefs)
			{
				keyTriggerDef[ nofKeyTriggerDef++] = triggerDef;
			}
			else
			{
				throw strus::runtime_error(_TXT("pattern with too many (%u) identical key events defined"), (unsigned int)nofKeyTriggerDef);
			}
			if (triggerDef->isKeyEvent && !hasKeyEvent)
			{
				hasKeyEvent = true;
				if (triggerDefNeedsInstall( *triggerDef, slot))
				{
					doInstall = true;
				}
			}
			else if ((Trigger::SigType)triggerDef->sigtype == Trigger::SigDel)
			{
				if (triggerDefNeedsInstall( *triggerDef, slot))
				{
					doInstall = true;
				}
			}
			else
			{
				doInstall = true;
			}
		}
		else
		{
			doInstall = true;
		}
		if (doInstall)
		{
			uint32_t eventTrigger =
				m_eventTriggerTable.add(
					EventTrigger( triggerDef->event, 
					Trigger( rule.actionSlotIdx-1, 
						 (Trigger::SigType)triggerDef->sigtype, triggerDef->sigval, triggerDef->variable)));
			m_eventTriggerList.push( rule.eventTriggerListIdx, eventTrigger);
		}
	}
	m_nofProgramsInstalled += 1;

	// Trigger the past stopword event, that is the real key event:
	if (programTrigger.past_eventid)
	{
		m_nofAltKeyProgramsInstalled += 1;
		replayPastEvent( programTrigger.past_eventid, rule, program.positionRange);
	}
	if (nofKeyTriggerDef && rule.actionSlotIdx)
	{
		std::size_t ki = 0;
		for (; ki < nofKeyTriggerDef; ++ki)
		{
			Trigger keyTrigger( rule.actionSlotIdx-1, 
					(Trigger::SigType)keyTriggerDef[ki]->sigtype, keyTriggerDef[ki]->sigval,
					keyTriggerDef[ki]->variable);
			fireSignal( slot, keyTrigger, data, disposeRuleList, followList);
		}
	}
}

void StateMachine::replayPastEvent( uint32_t eventid, const Rule& rule, uint32_t positionRange)
{
	// Search for the event 'eventid' in the latest visited stopwords and trigger them
	// to fire on the slot of the installed rule
	std::map<uint32_t,EventLog>::const_iterator
		ei = m_stopWordsEventLogMap.find( eventid);
	if (ei != m_stopWordsEventLogMap.end() && ei->second.data.start_ordpos + positionRange >= m_curpos)
	{
		ActionSlot& slot = m_actionSlotTable[ rule.actionSlotIdx-1];

		enum {NofDisposeRules=128,NofDelEvents=32};
		EventStructList followList;
		uint32_t disposeRuleList_alloca[ NofDisposeRules];
		DisposeRuleList disposeRuleList( disposeRuleList_alloca, NofDisposeRules);

		uint32_t delEventList_alloca[ NofDelEvents];
		typedef PodStructArrayBase<uint32_t,std::size_t,0> DelEventList;
		DelEventList delEventList( delEventList_alloca, NofDelEvents);

		uint32_t triggerlist = rule.eventTriggerListIdx;
		uint32_t trigger;
		while (m_eventTriggerList.next( triggerlist, trigger))
		{
			Trigger const* tp = m_eventTriggerTable.getTriggerPtr( trigger);
			uint32_t trigger_eventid = m_eventTriggerTable.getTriggerEventId( trigger);
			if (tp->sigtype() == Trigger::SigDel)
			{
				delEventList.add( trigger_eventid);
			}
			if (eventid == trigger_eventid)
			{
				fireSignal( slot, *tp, ei->second.data, disposeRuleList, followList);
			}
		}
		if (delEventList.size())
		{
			DelEventList::const_iterator
				li = delEventList.begin(),
				le = delEventList.end();
			for (; li != le; ++li)
			{
				std::map<uint32_t,EventLog>::const_iterator
					stopwi = m_stopWordsEventLogMap.find( *li);
				if (stopwi != m_stopWordsEventLogMap.end() && stopwi->second.timestmp > ei->second.timestmp)
				{
					deactivateRule( slot.rule);
					break;
				}
			}
		}
		DisposeRuleList::const_iterator
			di = disposeRuleList.begin(), de = disposeRuleList.end();
		for (; di != de; ++di)
		{
			deactivateRule( *di);
		}
		if (followList.size())
		{
			throw std::runtime_error( _TXT("internal: encountered past trigger with follow"));
			// ... only rules that really need the alternative event triggered should have an alternative key event
		}
	}
}



