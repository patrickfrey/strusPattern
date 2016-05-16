
IndexList::IndexList()
	:m_ar(0),m_allocsize(0),m_size(0),m_freelistidx(0){}

IndexList::IndexList( const IndexList& o)
	:m_ar(0),m_allocsize(0),m_size(o.m_size),m_freelistidx(o.m_freelistidx)
{
	if (o.m_allocsize)
	{
		expand( o.m_allocsize);
		std::memcpy( m_ar, o.m_ar, m_size * sizeof(*m_ar));
	}
}

void IndexList::expand( std::size_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::logic_error( "illegal call of IndexList::expand");
	}
	Element* ar_ = (Element*)std::realloc( m_ar, newallocsize * sizeof(Element));
	if (!ar_) throw std::bad_alloc();

	m_ar = ar_;
	m_allocsize = newallocsize;
}

void IndexList::addElement( uint32_t& list, uint32_t element)
{
	uint32_t newidx;
	if (m_freelistidx)
	{
		newidx = m_freelistidx-1;
		m_freelistidx = m_ar[ m_freelistidx-1].next;
	}
	else
	{
		if (m_size == m_allocsize)
		{
			if (m_allocsize >= (1<<31))
			{
				throw std::bad_alloc();
			}
			expand( m_allocsize?(m_allocsize*2):BlockSize);
		}
		newidx = m_size++;
	}
	m_ar[ newidx].value = element;
	m_ar[ newidx].next = list;
	list = newidx+1;
}

void IndexList::disposeList( const uint32_t& list)
{
	if (list == 0) return;
	if (list > m_size)
	{
		throw std::runtime_error("bad list index (disposeList)");
	}
	uint32_t idx = list;
	while (idx)
	{
		uint32_t nextidx = m_ar[ idx-1].next;
		m_ar[ idx-1].next = m_freelistidx;
		m_freelistidx = idx;
		idx = nextidx;
	}
}

uint32_t IndexList::nextElement( uint32_t& list) const
{
	if (list == 0) return 0;
	if (list > m_size)
	{
		throw std::runtime_error("bad list index (nextElement)");
	}
	if (list == 0) return 0;
	uint32_t rt = m_ar[ list-1].value;
	list = m_ar[ list-1].next;
}




uint32_t ActionSlotTable::addActionSlot( const ActionSlot& slot)
{
	uint32_t newidx;
	if (m_freelistidx)
	{
		newidx = m_freelistidx-1;
		m_freelistidx = m_ar[ m_freelistidx-1].event;
	}
	else
	{
		if (m_size == m_allocsize)
		{
			if (m_allocsize >= (1<<31))
			{
				throw std::bad_alloc();
			}
			expand( m_allocsize?(m_allocsize*2):BlockSize);
		}
		newidx = m_size++;
	}
	m_ar[ newidx] = slot;
	return newidx;
}

void ActionSlotTable::removeActionSlot( uint32_t idx)
{
	if (idx >= m_size)
	{
		throw std::runtime_error("bad rule index (remove action slot)");
	}
	m_ar[ idx].event = m_freelistidx;
	m_freelistidx = idx+1;
}

void ActionSlotTable::expand( uint32_t newallocsize)
{
	if (m_size > newallocsize)
	{
		throw std::logic_error( "illegal call of ActionSlotTable::expand");
	}
	ActionSlot* ar_ = (ActionSlot*)std::realloc( m_ar, newallocsize * sizeof(ActionSlot));
	if (!ar_) throw std::bad_alloc();

	m_ar = ar_;
	m_allocsize = newallocsize;
}

