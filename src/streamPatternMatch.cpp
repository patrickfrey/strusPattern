/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of an automaton for detecting patterns in a document stream
/// \file "streamPatternMatch.cpp"
#include "streamPatternMatch.hpp"
#include "symbolTable.hpp"
#include "utils.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include "strus/streamPatternMatchInstanceInterface.hpp"
#include "strus/streamPatternMatchContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "ruleMatcherAutomaton.hpp"
#include <map>
#include <limits>


using namespace strus;


struct StreamPatternMatchData
{
	StreamPatternMatchData(){}

	SymbolTable variableMap;
	SymbolTable patternMap;
	ProgramTable programTable;
};

class StreamPatternMatchContext
	:public StreamPatternMatchContextInterface
{
public:
	StreamPatternMatchContext( const StreamPatternMatchData* data_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_data(data_){}

	virtual ~StreamPatternMatchContext(){}

	virtual unsigned int termId( const std::string& name) const
	{
		try
		{
			return m_data->variableMap.get( name);
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier", *m_errorhnd, 0);
	}

	virtual void putInput( unsigned int termid, unsigned int ordpos, unsigned int bytepos, unsigned int bytesize)
	{
		try
		{
		}
		CATCH_ERROR_MAP( "failed to feed input to pattern patcher", *m_errorhnd);
	}

	virtual std::vector<stream::PatternMatchResult> fetchResult()
	{
		try
		{
		}
		CATCH_ERROR_MAP_RETURN( "failed to fetch pattern match result", *m_errorhnd, std::vector<stream::PatternMatchResult>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const StreamPatternMatchData* m_data;
};

enum JoinOperation {Sequence,SequenceStruct,Within,WithinStruct,Any,Intersect};

static JoinOperation joinOperationName( const char* joinopstr)
{
	static const char* ar[] = {"sequence","sequence_struct","within","within_struct","any","intersect",0};
	std::size_t ai = 0;
	for (; ar[ai]; ++ai)
	{
		if (strus::utils::caseInsensitiveEquals( joinopstr, ar[ai]))
		{
			return (JoinOperation)ai;
		}
	}
	throw strus::runtime_error(_TXT("unknown join operation of nodes '%s'"), joinopstr);
}


/// \brief Interface for building the automaton for detecting patterns in a document stream
class StreamPatternMatchInstance
	:public StreamPatternMatchInstanceInterface
{
public:
	StreamPatternMatchInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_expression_event_cnt(0){}

	virtual ~StreamPatternMatchInstance(){}

	virtual unsigned int getTermId( const std::string& name)
	{
		try
		{
			return m_data.variableMap.getOrCreate( name);
		}
		CATCH_ERROR_MAP_RETURN( "failed to get/create pattern match term identifier", *m_errorhnd, 0);
	}

	virtual void pushTerm( unsigned int termid)
	{
		try
		{
			uint32_t event = eventHandle( TermEvent, termid);
			m_stack.push_back( TreeNode( TreeNode::Term, event, 0/*expression*/, 0/*child*/));
		}
		CATCH_ERROR_MAP( "failed to push term on pattern match expression stack", *m_errorhnd);
	}

	virtual void pushExpression(
			const char* operationstr,
			std::size_t argc, unsigned int range_, unsigned int cardinality)
	{
		try
		{
			if (range_ > std::numeric_limits<uint32_t>::max())
			{
				throw strus::runtime_error(_TXT("proximity range value out of range or negative"));
			}
			if (argc > m_stack.size())
			{
				throw strus::runtime_error(_TXT("expression references more arguments than nodes on the stack"));
			}
			if (cardinality > (unsigned int)argc)
			{
				throw strus::runtime_error(_TXT("illegal value for cardinality"));
			}
			std::size_t ai = 0; ae = argc;
			for (; ai != ae; ++ai)
			{
				TreeNode& node = m_stack[ m_stack.size() - ai -1];
				node.next = ai?m_tree.size():0;
				m_tree.push_back( node);
			}
			m_stack.resize( m_stack.size() - argc);
			uint32_t event = eventHandle( ExpressionEvent, ++m_expression_event_cnt);
			m_stack.push_back( TreeNode( TreeNode::Expression, event, m_exprtab.size(), m_tree.size()/*child*/));
			m_exprtab.push_back( ExpressionDef( joinOperationName( operationstr), range_, cardinality));
		}
		CATCH_ERROR_MAP( "failed to push expression on pattern match expression stack", *m_errorhnd);
	}

	virtual void pushPattern( const std::string& name)
	{
		try
		{
			uint32_t event = eventHandle( TermEvent, m_data.patternMap.getValue( name));
			m_stack.push_back( TreeNode( TreeNode::PatternRef, event, 0/*child*/));
		}
		CATCH_ERROR_MAP( "failed to push pattern reference on pattern match expression stack", *m_errorhnd);
	}

	virtual void attachVariable( const std::string& name)
	{
		try
		{
			if (m_stack.empty())
			{
				throw strus::runtime_error(_TXT( "illegal operation attach variable when no node on the stack"));
			}
			if (m_stack.back().variable)
			{
				throw strus::runtime_error(_TXT( "more than one variable assignment to a node"));
			}
			m_stack.back().variable = m_data.variableMap.getOrCreate( name);
		}
		CATCH_ERROR_MAP( "failed to attach variable to top element of pattern match expression stack", *m_errorhnd);
	}

	uint32_t calculateRange( unsigned int nodeidx)
	{
		unsigned int rt = 0;
		
	}

	void createProgram( unsigned int nodeidx)
	{
		
	}

	virtual void closePattern( const std::string& name_)
	{
		try
		{
			if (m_stack.empty())
			{
				throw strus::runtime_error(_TXT("illegal operation close pattern when no node on the stack"));
			}
			TreeNode& node = m_stack.back();
			uint32_t positionRange = (node.type == TreeNode::Expression) ? m_exprtab[ node.expression].range;
			uint32_t program = m_data.programTable.createProgram( positionRange);
			
			uint32_t program;
			uint32_t initcount;
			uint32_t initval;
			switch (expressionDef.operation)
			{
				case Sequence:
					program = m_data.programTable.createProgram( range);
					initcount = cardinality?(uint32_t)cardinality:(uint32_t)argc;
					initval = (uint32_t)argc;
					m_data.programTable.createSlot( program, initval, initcount, uint32_t follow_event, uint32_t follow_program, bool isComplete);
					
				case SequenceStruct:
				case Within:
				case WithinStruct:
				case Any:
				case Intersect:
			}
		}
		CATCH_ERROR_MAP( "failed to close pattern definition on pattern match expression stack", *m_errorhnd);
	}

	virtual StreamPatternMatchContextInterface* createContext() const
	{
		try
		{
			return new StreamPatternMatchContext( &data, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( "failed to create pattern match context", *m_errorhnd, 0);
	}

private:
	struct ExpressionDef
	{
		JoinOperation operation;
		uint32_t program;
		uint32_t range;
		uint32_t cardinality;

		ExpressionDef( JoinOperation operation_, uint32_t range_, uint32_t cardinality_)
			:operation(operation_),program(0),range(range_),cardinality(cardinality_){}
		ExpressionDef( const ExpressionDef& o)
			:operation(o.operation),program(o.program),range(o.range),cardinality(o.cardinality){}
	};
	struct TreeNode
	{
		enum NodeType {Expression,Term,PatternRef};
		NodeType type;
		uint32_t event;
		uint32_t variable;
		uint32_t expression;
		std::size_t next;
		std::size_t child;

		TreeNode( NodeType type_, uint32_t event_, uint32_t expression_, std::size_t child_)
			:type(type_),event(event_),variable(0),expression(expression_),child(child_),next(0){}
		TreeNode( const TreeNode& o)
			:type(o.type),event(o.event),variable(o.variable),expression(o.expression),next(o.next),child(o.child){}
	};

private:
	enum PatternEventType {TermEvent=0, ExpressionEvent=1, ReferenceEvent=2};
	static uint32_t eventHandle( PatternEventType type_, uint32_t idx)
	{
		if (idx >= (1<<30)) throw std::bad_alloc();
		return idx | ((uint32_t)type_ << 30);
	}

private:
	ErrorBufferInterface* m_errorhnd;
	StreamPatternMatchData m_data;
	std::vector<ExpressionDef> m_exprtab;
	std::vector<TreeNode> m_stack;
	std::vector<TreeNode> m_tree;
	ExpressionDef expressionDef;
	uint32_t m_expression_event_cnt;
};

StreamPatternMatch::~StreamPatternMatch()
{}

StreamPatternMatchInstanceInterface* StreamPatternMatch::createInstance() const
{
	try
	{
		return new StreamPatternMatchInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( "failed to create pattern match instance", *m_errorhnd, 0);
}

