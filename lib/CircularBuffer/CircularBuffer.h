/*
 * CircularBuffer.h
 *
 *  Created on: 4 Oct 2015
 *      Author: eng-nbb
 */

#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "Stream.h"

class CircularBuffer
{
private:
	uint8_t *buffer;
	size_t size;
	size_t pos;
	Stream* debug;

public:
	CircularBuffer(size_t size, Stream* debug);
	~CircularBuffer();
	void insert(uint8_t value);
	uint8_t operator[](int pos);
	bool endsWith(const char *buf, size_t buf_size);
	bool endsWith(uint8_t *buf, size_t buf_size);

};


#endif /* CIRCULARBUFFER_H_ */
