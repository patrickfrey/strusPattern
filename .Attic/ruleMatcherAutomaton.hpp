class IndexList
{
public:
	IndexList();
	IndexList( const IndexList& o);

private:
	void expand( std::size_t newallocsize);

	struct Element
	{
		uint32_t value;
		uint32_t next;

		Element( uint32_t value_, uint32_t next_)
			:value(value_),next(next_){}
		Element( const Element& o)
			:value(o.value),next(o.next){}
	};

	void addElement( uint32_t& list, uint32_t element);
	uint32_t nextElement( uint32_t& list) const;
	void disposeList( const uint32_t& list);

private:
	enum {BlockSize=128};
	Element* m_ar;
	std::size_t m_allocsize;
	std::size_t m_size;
	uint32_t m_freelistidx;
};

