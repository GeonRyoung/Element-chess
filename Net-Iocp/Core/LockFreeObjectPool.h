#pragma once

#include <atomic>
#include <memory>
#include <cassert>

// ABA 문제 완화를 위한 태그 포인터 구조체
template <typename T>
struct alignas(16) TaggedPointer
{
    T* ptr;
    uint64_t tag;
};

template <typename T>
class LockFreeObjectPool
{
private:
    struct Node
    {
        T data;
        TaggedPointer<Node> next;
    };
    
    std::atomic<TaggedPointer<Node>> m_pFreeList;
    std::atomic<size_t> m_availableCount;
    
public:
    LockFreeObjectPool() : m_availableCount(0)
    {
        m_pFreeList.store(TaggedPointer<Node>{nullptr, 0});
    }
    
    ~LockFreeObjectPool()
    {
        TaggedPointer<Node> current = m_pFreeList.load(std::memory_order_relaxed);
        while (current.ptr != nullptr)
        {
            Node* next = current.ptr->next.ptr;
            delete current.ptr;
            current.ptr = next;
        }
    }
    
    // Lock 없이 Pool에서 객체를 가져오거나 새로 생성
    std::shared_ptr<T> Alloc()
    {
        TaggedPointer<Node> head = m_pFreeList.load(std::memory_order_acquire);
        while (head.ptr != nullptr)
        {
            TaggedPointer<Node> nextHead = head.ptr->next;
            nextHead.tag = head.tag + 1;
            
            if (m_pFreeList.compare_exchange_weak(head, nextHead,
                std::memory_order_release,
                std::memory_order_relaxed))
            {
                m_availableCount.fetch_sub(1, std::memory_order_relaxed);
                
                // std::enable_shared_from_this의 weak_ptr 상태를 리셋하기 위해 Placement New 호출
                T* ptr = &head.ptr->data;
                ptr->~T();
                new (ptr) T();
                
                // shared_ptr의 커스텀 삭제자를 지정하여, delete 대신 Free로 반환되게 함
                return std::shared_ptr<T>(ptr, [this, node = head.ptr](T* p)
                {
                    this->Free(node);
                });
            }
        }
        
        // Pool이 비어 있다면 바로 생성
        Node* newNode = new Node();
        return std::shared_ptr<T>(&newNode->data, [this, node = newNode](T* ptr) {
            this->Free(node);
        });
    }
    
    // 사용이 끝난 객체를 락 없이 풀에 반환
    void Free(Node* node)
    {
        assert(node != nullptr);
        
        TaggedPointer<Node> head = m_pFreeList.load(std::memory_order_relaxed);
        TaggedPointer<Node> newHead;
        newHead.ptr = node;

        do
        {
            newHead.tag = head.tag + 1;
            node->next = head;
        }
        while (!m_pFreeList.compare_exchange_weak(head, newHead,
            std::memory_order_release,
            std::memory_order_relaxed));
        
        m_availableCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    size_t GetAvailableCount() const
    {
        return m_availableCount.load(std::memory_order_relaxed);
    }
};