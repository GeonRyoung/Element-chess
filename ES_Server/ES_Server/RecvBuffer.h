#pragma once
class RecvBuffer
{
public:
	RecvBuffer(int32_t bufferSize);
	~RecvBuffer();

	void Clean();
	bool OnRead(int32_t numOfBytes);
	bool OnWrite(int32_t numOfBytes);

	char* ReadPos() { return &_buffer[_readPos]; }
	char* WritePos() { return &_buffer[_writePos]; }
	int32_t DataSize() { return _writePos - _readPos; }
	int32_t FreeSize() { return _capacity - _writePos; }

private:
	int32_t _capacity = 0;
	int32_t _bufferSize = 0;
	int32_t _readPos = 0;
	int32_t _writePos = 0;
	vector<char> _buffer;
};

