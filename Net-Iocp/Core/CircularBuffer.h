#pragma once

#include <vector>
#include <cstring>
#include <cassert>
#include <mutex>

class CircularBuffer
{
private:
    std::vector<char> m_buffer;
    size_t m_capacity;
    size_t m_readPos;
    size_t m_writePos;
    mutable std::mutex m_bufferLock;
    
public:
    explicit CircularBuffer(size_t capacity)
    : m_buffer(capacity), m_capacity(capacity), m_readPos(0), m_writePos(0) {}
    
    size_t GetUseSize() const
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        if (m_writePos >= m_readPos)
            return m_writePos - m_readPos;
        return m_capacity - m_readPos + m_writePos;
    }
    
    size_t GetFreeSize() const
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        if (m_writePos >= m_readPos)
            return m_capacity - (m_writePos - m_readPos) - 1;
        return m_readPos - m_writePos - 1;
    }
    
    bool Write(const char* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        
        size_t useSize = (m_writePos >= m_readPos) ? (m_writePos - m_readPos) : (m_capacity - m_readPos + m_writePos);
        size_t freeSize = m_capacity - useSize - 1;
        
        if (freeSize < size)
            return false;
            
        size_t rightSpace = m_capacity - m_writePos;
        if (size <= rightSpace)
            std::memcpy(&m_buffer[m_writePos], data, size);
        else
        {
            std::memcpy(&m_buffer[m_writePos], data, rightSpace);
            std::memcpy(&m_buffer[0], data + rightSpace, size - rightSpace);
        }
        
        m_writePos = (m_writePos + size) % m_capacity;
        return true;
    }
    
    bool Peek(char* dest, size_t size) const
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        
        size_t useSize = (m_writePos >= m_readPos) ? (m_writePos - m_readPos) : (m_capacity - m_readPos + m_writePos);
        if (useSize < size)
            return false;
        
        size_t rightSpace = m_capacity - m_readPos;
        if (size <= rightSpace)
            std::memcpy(dest, &m_buffer[m_readPos], size);
        else
        {
            std::memcpy(dest, &m_buffer[m_readPos], rightSpace);
            std::memcpy(dest + rightSpace, &m_buffer[0], size - rightSpace);
        }
        return true;   
    }
    
    bool Consume(size_t size)
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        
        size_t useSize = (m_writePos >= m_readPos) ? (m_writePos - m_readPos) : (m_capacity - m_readPos + m_writePos);
        if (useSize < size)
            return false;
            
        m_readPos = (m_readPos + size) % m_capacity;
        return true;
    }
        
    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_bufferLock);
        m_readPos = 0;
        m_writePos = 0;
    }
};