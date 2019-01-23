//------------------------------------------------------------------------------
// This class implements a circular doubly-linked list.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxListImpl_H__
#define FxListImpl_H__

namespace OC3Ent
{

namespace Face
{

//------------------------------------------------------------------------------
// FxList.
//------------------------------------------------------------------------------
// Constructor.
template<typename FxListElem>
FxList<FxListElem>::FxList()
	: _head(NULL)
	, _size(0)
{
}

template<typename FxListElem>
FxList<FxListElem>::FxList( const FxList<FxListElem>& other )
	: _head(0)
	, _size(0)
{
	_head = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Head"));
	FxDefaultConstruct(_head);
	_head->next = _head;
	_head->prev = _head;

	Iterator curr = other.Begin();
	Iterator end  = other.End();
	for( ; curr != end; ++curr )
	{
		PushBack(*curr);
	}
}

template<typename FxListElem>
FxList<FxListElem>& 
FxList<FxListElem>::operator=( const FxList<FxListElem>& other )
{
	if( _size )
	{
		Clear();
	}

	Iterator curr = other.Begin();
	Iterator end  = other.End();
	for( ; curr != end; ++curr )
	{
		PushBack(*curr);
	}

	return *this;
}

// Destructor.
template<typename FxListElem>
FxList<FxListElem>::~FxList()
{
	Clear();
	if( _head )
	{
		FxDelete(_head);
	}
}

template<typename FxListElem>
typename FxList<FxListElem>::Iterator 
FxList<FxListElem>::Begin( void ) const
{
	if( !_head )
	{
		_head = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Head"));
		FxDefaultConstruct(_head);
		_head->next = _head;
		_head->prev = _head;
	}
	return ++Iterator(_head);
}

template<typename FxListElem>
typename FxList<FxListElem>::Iterator 
FxList<FxListElem>::End( void ) const
{
	if( !_head )
	{
		_head = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Head"));
		FxDefaultConstruct(_head);
		_head->next = _head;
		_head->prev = _head;
	}
	return Iterator(_head);
}

template<typename FxListElem>
typename FxList<FxListElem>::Iterator 
FxList<FxListElem>::Find( const FxListElem& toFind ) const
{
	Iterator finder = Begin();
	Iterator end    = End();
	for( ; finder != end; ++finder )
	{
		if( *finder == toFind ) break;
	}
	return finder;
}

template<typename FxListElem>
typename FxList<FxListElem>::ReverseIterator
FxList<FxListElem>::ReverseBegin( void ) const
{
	if( !_head )
	{
		_head = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Head"));
		FxDefaultConstruct(_head);
		_head->next = _head;
		_head->prev = _head;
	}
	return ++ReverseIterator(_head);
}

template<typename FxListElem>
typename FxList<FxListElem>::ReverseIterator
FxList<FxListElem>::ReverseEnd( void ) const
{
	if( !_head )
	{
		_head = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Head"));
		FxDefaultConstruct(_head);
		_head->next = _head;
		_head->prev = _head;
	}
	return ReverseIterator(_head);
}

template<typename FxListElem>
void
FxList<FxListElem>::Clear( void )
{
	if( _head )
	{
		FxListNode* pNode = _head->next;
		while( pNode != _head )
		{
			FxListNode* temp = pNode;
			pNode = pNode->next;
			FxDelete(temp);
		}
		FxDelete(_head);
		_size = 0;
	}
}

template<typename FxListElem>
FxBool
FxList<FxListElem>::IsEmpty( void ) const
{
	return _size == 0;
}

template<typename FxListElem>
FxSize
FxList<FxListElem>::Length( void ) const
{
	return _size;
}

template<typename FxListElem>
FxSize
FxList<FxListElem>::Allocated( void ) const
{
	return (_size + 1) * (sizeof(FxListElem) + 2 * sizeof(FxListNode*)) 
		   + sizeof(_size);
}

template<typename FxListElem>
FxListElem& 
FxList<FxListElem>::Back( void ) const
{
	return *--End();
}

template<typename FxListElem>
void 
FxList<FxListElem>::PushBack( const FxListElem& element )
{
	Insert(element, End());
}

template<typename FxListElem>
void 
FxList<FxListElem>::PopBack( void )
{
	Remove(--End());
}

template<typename FxListElem>
FxListElem& 
FxList<FxListElem>::Front( void ) const
{
	return *Begin();
}

template<typename FxListElem>
void 
FxList<FxListElem>::PushFront( const FxListElem& element )
{
	Insert(element, Begin());
}

template<typename FxListElem>
void 
FxList<FxListElem>::PopFront( void )
{
	Remove(Begin());
}

template<typename FxListElem>
typename FxList<FxListElem>::Iterator
FxList<FxListElem>::Insert( const FxListElem& element, Iterator iter )
{
	FxListNode* insertNode = static_cast<FxListNode*>(FxAlloc(sizeof(FxListNode), "List Node"));
	FxConstruct(insertNode, FxListNode(element, iter.GetNode(), iter.GetNode()->prev));
	insertNode->prev->next = insertNode;
	insertNode->next->prev = insertNode;
	++_size;
	return Iterator(insertNode);
}

template<typename FxListElem>
void
FxList<FxListElem>::Remove( Iterator iter )
{
	FxListNode* toRemove = iter.GetNode();
	FxAssert( toRemove != _head );
	if( toRemove != _head )
	{
		toRemove->prev->next = toRemove->next;
		toRemove->next->prev = toRemove->prev;
		FxDelete(toRemove);
		--_size;
	}
}

template<typename FxListElem>
void FxList<FxListElem>::RemoveIfEqual( const FxListElem& element )
{
	if( _head )
	{
		FxListNode* curr = _head->next;
		while( curr != _head )
		{
			if( curr->element == element )
			{
				FxListNode* temp = curr;
				curr = curr->next;

				temp->prev->next = temp->next;
				temp->next->prev = temp->prev;
				FxDelete(temp);
				--_size;
			}
			else
			{
				curr = curr->next;
			}
		}
	}
}

template<typename FxListElem>
void
FxList<FxListElem>::Sort( void )
{
	if( _head )
	{
		// List merge sort algorithm from:
		// http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
		FxListNode* list = _head;
		FxListNode *p, *q, *e, *tail, *oldhead;
		FxInt32 insize, nmerges, psize, qsize, i;

		insize = 1;

		while( 1 )
		{
			p = list;
			// Only used for circular linkage.
			oldhead = list;
			list = NULL;
			tail = NULL;

			// Count number of merges we do in this pass.
			nmerges = 0; 

			while( p ) 
			{
				// There exists a merge to be done.
				nmerges++; 
				// Step "insize" places along from p.
				q = p;
				psize = 0;
				for( i = 0; i < insize; ++i ) 
				{
					psize++;
					q = ( q->next == oldhead ) ? NULL : q->next;
					if( !q )
					{
						break;
					}
				}

				// If q hasn't fallen off end, we have two lists to merge.
				qsize = insize;

				// Now we have two lists; merge them.
				while( psize > 0 || ( qsize > 0 && q ) ) 
				{

					// Decide whether next element of merge comes from p or q.
					if( psize == 0 ) 
					{
						// p is empty; e must come from q.
						e = q; q = q->next; qsize--;
						if( q == oldhead ) q = NULL;
					} 
					else if( qsize == 0 || !q ) 
					{
						// q is empty; e must come from p.
						e = p; p = p->next; psize--;
						if( p == oldhead ) p = NULL;
					} 
					else if( p->element < q->element ) 
					{
						// First element of p is lower (or same); e must come from p.
						e = p; p = p->next; psize--;
						if( p == oldhead ) p = NULL;
					} 
					else 
					{
						// First element of q is lower; e must come from q.
						e = q; q = q->next; qsize--;
						if (q == oldhead) q = NULL;
					}

					// Add the next element to the merged list.
					if( tail ) 
					{
						tail->next = e;
					} 
					else 
					{
						list = e;
					}
					// Maintain reverse pointers in a doubly linked list.
					e->prev = tail;
					tail = e;
				}

				// Now p has stepped "insize" places along, and q has too.
				p = q;
			}
			tail->next = list;
			list->prev = tail;

			// If we have done only one merge, we're finished.
			// Allow for nmerges == 0, the empty list case.
			if( nmerges <= 1 )
			{
				return;
			}

			// Otherwise repeat, merging lists twice the size.
			insize *= 2;
		}
	}
}

} // namespace Face

} // namespace OC3Ent

#endif
