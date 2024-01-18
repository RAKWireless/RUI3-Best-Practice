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

#define MAX_QUEUE_SIZE 50

class ArrayQueue
{
public:
	ArrayQueue();
	bool enQueue(bool element);
	bool deQueue();
	int getSize();
	bool getFirst();
	bool isEmpty();

private:
	bool Queue[MAX_QUEUE_SIZE];
	int Front;
	int Rear;
};

#endif // ARRAYQUEUECLASS_H_INCLUDED
