#ifndef __HEAP_H__
#define __HEAP_H__
#include <vector>
#include <stdexcept>

namespace ds {
// 用数组实现，不要用链表了
template <class T>
class MinHeap {
public:
    MinHeap(void);
    ~MinHeap(void);

    int push(const T &data);
    int pop(T &data);
    int size(void) const {return size_;}
    bool empty(void) const {return this->size() == 0 ? true : false;}

    T& operator[](int pos);
private:
    void swap(T &p, T &q) {
        auto tmp = p;
        p = q;
        q = tmp;
    }
private:
    std::vector<T> heap_;
    int size_;
};

template <class T>
MinHeap<T>::MinHeap(void)
: heap_(1),
size_(0)
{
}

template <class T>
MinHeap<T>::~MinHeap(void)
{

}

template <class T>
int MinHeap<T>::push(const T &data)
{
    if (size_ == heap_.size() - 1) {
        heap_.push_back(data);
    } else {
        heap_[size_ + 1] = data;
    }

    std::size_t min_pos = ++size_;
    while (true) {
        std::size_t parent_pos = min_pos / 2;
        if (parent_pos == 0) {
            break;
        }
        if (heap_[min_pos] < heap_[parent_pos]) {
            swap(heap_[min_pos], heap_[parent_pos]);
            min_pos = parent_pos;
        } else {
            break;
        }
    }
    return 1;
}

template <class T>
int MinHeap<T>::pop(T &data)
{
    if (this->empty()) {
        return 0;
    }

    data = heap_[1];
    heap_[1] = heap_[size_];
    --size_;

    int pos = 1;
    while (true)
    {
        int min_pos = pos;
        int left = min_pos * 2;
        int right = min_pos *2 + 1;
        if (left <= size_ && heap_[min_pos] > heap_[left]) {
            min_pos = left;
        }

        if (right <= size_ && heap_[min_pos] > heap_[right]) {
            min_pos = right;
        }

        if (min_pos != pos) {
            swap(heap_[pos], heap_[min_pos]);
            pos = min_pos;
        } else {
            break;
        }
    }
    return 1;
}

template <class T>
T& MinHeap<T>::operator[](int pos)
{
    if (pos < 0 || pos >= size_) {
        throw std::runtime_error("out of range");
    }

    return heap_[pos + 1];
}

}
#endif