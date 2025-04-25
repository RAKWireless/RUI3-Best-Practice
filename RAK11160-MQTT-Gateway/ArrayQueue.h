/**
 * @file ArrayQueue.h
 * @author Sphinkie, changed by Bernd Giesecke (bernd@giesecke.tk)
 * @brief FiFo for bool values, based on https://github.com/Sphinkie/ArrayQueue
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef ARRAYQUEUECLASS_H_INCLUDED
#define ARRAYQUEUECLASS_H_INCLUDED

#include <Arduino.h>

#define MAX_QUEUE_SIZE 20

class ArrayQueue
{
public:
	ArrayQueue();
	bool enQueue(uint8_t *payload, uint16_t payload_size);
	void deQueue();
	int getSize();
	bool isEmpty();
	uint8_t *getPayload(void);
	uint16_t getPayloadSize();

private:
	uint8_t Queue_Payload[MAX_QUEUE_SIZE][128];
	uint16_t Queue_Payload_Size[MAX_QUEUE_SIZE];
	int Front;
	int Rear;
};

#endif // ARRAYQUEUECLASS_H_INCLUDED
