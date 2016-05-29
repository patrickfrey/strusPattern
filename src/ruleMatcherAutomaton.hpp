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
#include <map>
#include <string>
#include <stdexcept>
#include <new>

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
	BaseAddrDisposeRuleList =	(10000000 * 13)
};

class Trigger
{
public:
	enum SigType {SigAny=0x0,SigSequence=0x1,SigWithin=0x2,SigDel=0x3};

	Trigger( uint32_t slot_, SigType sigtype_, uint32_t sigval_, uint32_t variable_, float weight_)
		:m_slot(slot_),m_sigtype(sigtype_),m_variable(variable_),m_sigval(sigval_),m_weight(weight_)
	{
		if (variable_ > MaxVariableId) throw std::bad_alloc();
	}
	Trigger( const Trigger& o)
		:m_slot(o.m_slot),m_sigtype(o.m_sigtype),m_variable(o.m_variable),m_sigval(o.m_sigval),m_weight(o.m_weight){}

	uint32_t slot() const		{return m_slot;}
	SigType sigtype() const		{return (SigType)m_sigtype;}
	uint32_t sigval() const		{return (uint32_t)m_sigval;}
	uint32_t variable() const	{return (uint32_t)m_variable;}
	float weight() const		{return m_weight;}

private:
	enum {MaxVariableId=(1<<28)-1};
	uint32_t m_slot;
	unsigned int m_sigtype:4;
	unsigned int m_variable:28;
	uint32_t m_sigval;
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
	EventTriggerTable();
	EventTriggerTable( const EventTriggerTable& o);

	uint32_t add( const EventTrigger& et);
	void remove( uint32_t idx);
	Trigger const* getTriggerPtr( uint32_t idx, uint32_t event) const;

	typedef PodStructArrayBase<Trigger const*,std::size_t,0> TriggerRefList;
	void getTriggers( TriggerRefList& triggers, uint32_t event) const;
	uint32_t size() const	{return m_size;}

private:
	void expandEventAr( uint32_t newallocsize);

private:
	enum {BlockSize=256};
	uint32_t* m_eventAr;
	uint32_t* m_triggerIndAr;
	LinkedTriggerTable m_triggerTab;
	uint32_t m_allocsize;
	uint32_t m_size;
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
	uint32_t subdataref;

	EventData()
		:origpos(0),origsize(0),ordpos(0),subdataref(0){}
	EventData( std::size_t origpos_, std::size_t origsize_, uint32_t ordpos_, uint32_t subdataref_)
		:origpos(origpos_),origsize(origsize_),ordpos(ordpos_),subdataref(subdataref_){}
	EventData( const EventData& o)
		:origpos(o.origpos),origsize(o.origsize),ordpos(o.ordpos),subdataref(o.subdataref){}
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
	float weight;
	EventData data;

	EventItem( uint32_t variable_, float weight_, const EventData& data_)
		:variable(variable_),weight(weight_),data(data_){}
	EventItem( const EventItem& o)
		:variable(o.variable),weight(o.weight),data(o.data){}
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
	float weight;

	TriggerDef( uint32_t event_, Trigger::SigType sigtype_, uint32_t sigval_, uint32_t variable_, float weight_)
		:event(event_),sigtype(sigtype_),sigval(sigval_),variable(variable_),weight(weight_){}
	TriggerDef( const TriggerDef& o)
		:event(o.event),sigtype(o.sigtype),sigval(o.sigval),variable(o.variable),weight(o.weight){}
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
	void createTrigger( uint32_t program, uint32_t event, Trigger::SigType sigtype, uint32_t sigval, uint32_t variable, float weight);
	void defineEventProgram( uint32_t eventid, uint32_t programidx);

	const Program& operator[]( uint32_t programidx) const	{return m_programTable[ programidx-1];}
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

private:
	ActionSlotDefList m_actionSlotArray;
	TriggerDefList m_triggerList;
	PodStructTableBase<Program,uint32_t,ProgramTableFreeListElem,BaseAddrProgramTable> m_programTable;
	PodStackPoolBase<ProgramTrigger,uint32_t,BaseAddrProgramList> m_programTriggerList;
	typedef utils::UnorderedMap<uint32_t,uint32_t> EventProgamTriggerMap;
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
	explicit StateMachine( const ProgramTable* programTable_);
	StateMachine( const StateMachine& o);

	void doTransition( uint32_t event, const EventData& data);
	void setCurrentPos( uint32_t pos);
	void installProgram( const ProgramTrigger& programtrigger);

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
	unsigned int nofProgramsInstalled() const	{return m_nofProgramsInstalled;}
	unsigned int nofPatternsTriggered() const	{return m_nofPatternsTriggered;}
	double nofOpenPatterns() const			{return m_nofOpenPatterns;}

private:
	typedef PodStructArrayBase<uint32_t,std::size_t,BaseAddrDisposeRuleList> DisposeRuleList;
	void fireTrigger( ActionSlot& slot, const Trigger& trigger, const EventData& data,
				DisposeRuleList& disposeRuleList, EventStructList& followList);
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
	std::vector<EventStruct> m_stopWordsEventList;
	unsigned int m_nofProgramsInstalled;
	unsigned int m_nofPatternsTriggered;
	double m_nofOpenPatterns;
};

} //namespace
#endif

