/**
 * @file ArrayQueue.cpp
 * @author Sphinkie, changed by Bernd Giesecke (bernd@giesecke.tk)
 * @brief FiFo for bool values, based on https://github.com/Sphinkie/ArrayQueue
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "ArrayQueue.h"

// *******************************************************************
// Management of a FIFO of the type :
//  ---FxxxxR----
// *******************************************************************
ArrayQueue::ArrayQueue()
{
	Front = -1;
	Rear = -1;
}

// *******************************************************************
// We put an element in the FIFO (by the Rear). Examples:
// xR---------
// --FxxxxR---
// xR----Fxxxx
// When the queue is full, new items are ignored!
//  xxFwwwRxxxxx
// *******************************************************************
bool ArrayQueue::enQueue(bool element)
{
	if (this->getSize() == MAX_QUEUE_SIZE - 1)
	{
		Serial.println(F("Warning: overwriting ArrayQueue"));
		Serial.println("Rear=" + String(Rear));
		Serial.println("Front=" + String(Front));
		return false;
	}
	// We store the data provided
	Queue[Rear] = element;

	// Modulo is used so that rear indicator can wrap around
	Rear = ++Rear % MAX_QUEUE_SIZE;

	return true;
}

// *******************************************************************
// Remove an item from the list. Returns -1 if the list is empty.
// *******************************************************************
bool ArrayQueue::deQueue()
{
	if (this->isEmpty())
		return -1;

	bool Value = Queue[Front];

	// Modulo is used so that front indicator can wrap around
	Front = ++Front % MAX_QUEUE_SIZE;

	return Value;
}

// *******************************************************************
// Returns the last element of the FIFO, but without popping it.
// Returns -1 if the list is empty.
// *******************************************************************
bool ArrayQueue::getFirst()
{
	if (this->isEmpty())
		return -1;
	return Queue[Front];
}

// *******************************************************************
// Returns the number in the list. Examples:
// xR---------  : Size = 0--1 = 1
// --FxxxxR---  : Size = 5 -1 = 4
// xR----Fxxxx  : Size = 0 -5 = 5
// *******************************************************************
int ArrayQueue::getSize()
{
	return abs(Rear - Front);
}

// *******************************************************************
// Returns TRUE if the FIFO is empty. (i.e. if R=F).
// *******************************************************************
bool ArrayQueue::isEmpty()
{
	return (Front == Rear) ? true : false;
}
