#include "CircularBuffer.h"


CircularBuffer::CircularBuffer(size_t size, Stream* debug)
{
	this->buffer = (uint8_t*)calloc(size, sizeof(uint8_t));

	this->size = size;
	this->pos = 0;
	this->debug = debug;

}

CircularBuffer::~CircularBuffer()
{
	free(this->buffer);
}

void CircularBuffer::insert(uint8_t value)
{
	this->buffer[this->pos++] = value;
}

uint8_t CircularBuffer::operator[](int pos)
{
	int val = (this->size + pos + this->pos) % this->size;
	return this->buffer[val];
}

bool CircularBuffer::endsWith(const char *buf, size_t buf_size)
{
	return this->endsWith((uint8_t*) buf, buf_size);
}

bool CircularBuffer::endsWith(uint8_t *buf, size_t buf_size)
{
	if(buf_size > this->size)
	{
		return false;
	}
	bool result = true;
	for(int i = buf_size * -1; i < 0; i++)
	{
		result = (*this)[i] == buf[i + buf_size];

		if(!result)
		{
			break;
		}
	}
	return result;
}
