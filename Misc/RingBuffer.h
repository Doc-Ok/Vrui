/***********************************************************************
RingBuffer - Class for ring buffers that increase size dynamically when
full.
Copyright (c) 2019 Oliver Kreylos
***********************************************************************/

#ifndef MISC_RINGBUFFER_INCLUDED
#define MISC_RINGBUFFER_INCLUDED

#include <stddef.h>
#include <stdlib.h>

namespace Misc {

template <class EntryParam>
class RingBuffer
	{
	/* Embedded classes: */
	public:
	typedef EntryParam Entry; // Type of entry stored in the ring buffer
	
	class iterator // Class for mutable iterators into a ring buffer
		{
		friend class RingBuffer;
		
		/* Elements: */
		private:
		RingBuffer& buffer; // Buffer in which the iterator iterates
		Entry* entry; // Pointer to current entry
		
		/* Constructors and destructors: */
		private:
		iterator(RingBuffer& sBuffer,Entry* sEntry)
			:buffer(sBuffer),entry(sEntry)
			{
			}
		
		/* Methods: */
		public:
		bool operator==(const iterator& other) const // Equality operator; assumes other iterator is into the same buffer
			{
			return entry==other.entry;
			}
		bool operator!=(const iterator& other) const // Inequality operator; assumes other iterator is into the same buffer
			{
			return entry!=other.entry;
			}
		Entry& operator*(void) const // Indirection operator
			{
			return *entry;
			}
		Entry* operator->(void) const // Arrow operator
			{
			return entry;
			}
		iterator& operator++(void) // Pre-increment
			{
			if(++entry==buffer.bufferEnd)
				entry=buffer.buffer;
			return *this;
			}
		iterator operator++(int) // Post-increment
			{
			iterator result=*this;
			if(++entry==buffer.bufferEnd)
				entry=buffer.buffer;
			return result;
			}
		iterator& operator--(void) // Pre-decrement
			{
			if(entry==buffer.buffer)
				entry=buffer.bufferEnd;
			--entry;
			return *this;
			}
		iterator operator--(int) // Post-decrement
			{
			iterator result=*this;
			if(entry==buffer.buffer)
				entry=buffer.bufferEnd;
			--entry;
			return result;
			}
		};
	
	friend class iterator;
	
	/* Elements: */
	private:
	Entry* buffer; // Allocated buffer
	Entry* bufferEnd; // Pointer to end of buffer
	Entry* head; // Pointer to entry at the front of the buffer
	Entry* tail; // Pointer to entry behind the end of the buffer; only equal to head if buffer is empty
	
	/* Constructors and destructors: */
	public:
	RingBuffer(size_t bufferSize) // Creates an empty buffer of the given number of slots
		:buffer(static_cast<Entry*>(malloc((bufferSize+1)*sizeof(Entry)))), // Allocate one extra entry
		 bufferEnd(buffer+(bufferSize+1)), // Add one extra slot as a sentinel
		 head(buffer),tail(buffer)
		{
		}
	~RingBuffer(void) // Destroys the ring buffer
		{
		/* Destroy all entries still in the buffer: */
		if(tail>=head)
			{
			/* Destroy from head to tail: */
			for(;head!=tail;++head)
				head->~Entry();
			}
		else
			{
			/* Destroy from head to buffer end, then from buffer start to tail: */
			for(;head!=bufferEnd;++head)
				head->~Entry();
			for(head=buffer;head!=tail;++head)
				head->~Entry();
			}
		
		/* Release the buffer: */
		free(buffer);
		}
	
	/* Methods: */
	bool empty(void) const // Returns true if the buffer is empty
		{
		return tail==head;
		}
	bool full(void) const // Returns true if adding another entry would cause a buffer expansion
		{
		/* Tentatively advance the tail pointer: */
		Entry* nextTail=tail;
		if(++nextTail==bufferEnd)
			nextTail=buffer;
		
		/* If the advanced tail is equal to the head, the buffer is full: */
		return nextTail==head;
		}
	size_t size(void) const // Returns number of entries in the buffer
		{
		if(tail>=head)
			return size_t(tail-head);
		else
			return size_t(tail-buffer)+size_t(bufferEnd-head);
		}
	const Entry& front(void) const // Returns the first entry; assumes the buffer is not empty
		{
		return *head;
		}
	Entry& front(void) // Ditto
		{
		return *head;
		}
	RingBuffer& pop_front(void) // Removes the first entry from the buffer; assumes the buffer is not empty
		{
		/* Destroy the head entry in place: */
		head->~Entry();
		
		/* Move the head pointer to the next entry: */
		if(++head==bufferEnd)
			head=buffer;
		
		return *this;
		}
	RingBuffer& push_back(const Entry& newEntry) // Places the given entry at the end of the buffer
		{
		/* Just put it in using in-place copy construction; there is a sentinel slot at the end: */
		new(tail) Entry(newEntry);
		
		/* Move the tail pointer to the next entry: */
		if(++tail==bufferEnd)
			tail=buffer;
		
		/* Check if the buffer became full: */
		if(tail==head)
			{
			/* Increase the size of the buffer: */
			size_t bufferSize=bufferEnd-buffer;
			size_t newBufferSize=(bufferSize*3+2)/2;
			
			/* Move the current buffer contents to a new larger buffer using in-place copy construction and destruction: */
			Entry* newBuffer=static_cast<Entry*>(malloc(newBufferSize*sizeof(Entry)));
			tail=newBuffer;
			for(Entry* ePtr=head;ePtr!=bufferEnd;++ePtr,++tail)
				{
				new(tail) Entry(*ePtr);
				ePtr->~Entry();
				}
			for(Entry* ePtr=buffer;ePtr!=head;++ePtr,++tail)
				{
				new(tail) Entry(*ePtr);
				ePtr->~Entry();
				}
			
			/* Replace the old buffer: */
			free(buffer);
			buffer=newBuffer;
			bufferEnd=newBuffer+newBufferSize;
			head=buffer;
			}
		
		return *this;
		}
	RingBuffer& pop_back(void) // Removes the last entry from the buffer; assumes the buffer is not empty
		{
		/* Move the tail pointer to the previous entry: */
		if(tail==buffer)
			tail=bufferEnd;
		--tail;
		
		/* Destroy the tail entry in place: */
		tail->~Entry();
		
		return *this;
		}
	iterator begin(void) // Returns an iterator to the beginning of the buffer
		{
		return iterator(*this,head);
		}
	iterator end(void) // Returns an iterator to the end of the buffer
		{
		return iterator(*this,tail);
		}
	};

}

#endif
