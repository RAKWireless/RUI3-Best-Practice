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

/**
 * @brief Construct a new Array Queue object
 * 		FiFo is of type ---FxxxxR----
 */
ArrayQueue::ArrayQueue()
{
	Front = -1;
	Rear = -1;
}

/**
 * @brief Add element to FiFo (at the rear)
 * 
 * @param payload pointer to uint8_t array to add to the FiFo
 * @param payload_size size of the uint8_t array
 * @return true Success
 * @return false Failed, FiFo is full
 */
bool ArrayQueue::enQueue(uint8_t *payload, uint16_t payload_size)
{
	noInterrupts();
	if (this->getSize() == MAX_QUEUE_SIZE - 1)
	{
		Serial.println(F("Warning: overwriting ArrayQueue"));
		Serial.println("Rear=" + String(Rear));
		Serial.println("Front=" + String(Front));
		interrupts();
		return false;
	}
	// We store the data provided
	memcpy((void *)&Queue_Payload[Rear], (void *)payload, payload_size);
	Queue_Payload_Size[Rear] = payload_size;

	// Modulo is used so that rear indicator can wrap around
	Rear = ++Rear % MAX_QUEUE_SIZE;

	interrupts();

	return true;
}

/**
 * @brief Remove item from the FiFo
 * 
 */
void ArrayQueue::deQueue()
{
	if (this->isEmpty())
		return;

	noInterrupts();

	// Modulo is used so that front indicator can wrap around
	Front = ++Front % MAX_QUEUE_SIZE;

	interrupts();
}

/**
 * @brief Get payload size from first entry in FiFO
 * 
 * @return uint16_t payload size
 */
uint16_t ArrayQueue::getPayloadSize(void)
{
	if (this->isEmpty())
		return 0;

	noInterrupts();
	uint16_t size = Queue_Payload_Size[Front];
	interrupts();
	return size;
}

/**
 * @brief Get pointer to first payload entry in FiFo 
 * 
 * @return uint8_t* pointer to uint8_t array
 */
uint8_t *ArrayQueue::getPayload(void)
{
	if (this->isEmpty())
		return NULL;
	return &Queue_Payload[Front][0];
}

/**
 * @brief Return number of entries in the queue
 * @return int number of entries
 */
int ArrayQueue::getSize()
{
	return abs(Rear - Front);
}

/**
 * @brief Check if FiFo is empty
 * 
 * @return true No entries in FiFo
 * @return false FiFo has entries
 */
bool ArrayQueue::isEmpty()
{
	return (Front == Rear) ? true : false;
}
