//
// Created by l2pic on 30.03.2021.
//

#include <cstring>
#include <cstdio>

#include "BufferStatic.h"

BufferStatic::BufferStatic(unsigned int capacity)
    : buffer(new char[capacity]), capacity(capacity), size(0), head(0), tail(0) {
}

BufferStatic::~BufferStatic() {
    delete[] buffer;
}

BufferStatic::Result BufferStatic::append(char *data, unsigned int len) {
    if (capacity < len)
        return Result::InvalidLen;
    else if (capacity == size)
        return Result::BufferFull;

    if (tail >= head) {
        // normal data emplacement
        if (tail + len <= capacity) {
            // most general case, append data to buffer tail
            memcpy(buffer + tail, data, len);
            tail += len;
            size += len;
        } else /*if (tail + len > capacity)*/ {
            // need to split data and copy to the end and to the beginning of the buffer
            if (tail > 0 && len - (capacity - tail) <= head) {
                unsigned int firstBlockLen = capacity - tail;
                if (firstBlockLen > 0)
                    memcpy(buffer + tail, data, firstBlockLen);

                unsigned int secondBlockLen = len - firstBlockLen;
                memcpy(buffer, data + firstBlockLen, secondBlockLen);

                tail = secondBlockLen;
                size += len;
            } else {
                return Result::BufferFull;
            }
        }
    } else /*if (tail < head)*/ {
        // data emplacement separated on two blocks: at the end and at the beginning of buffer
        if (head - tail >= len) {
            memcpy(buffer + tail, data, len);
            tail += len;
            size += len;
        } else {
            return Result::BufferFull;
        }
    }

    return Result::Ok;
}

BufferStatic::Result BufferStatic::getData(char *out, unsigned int len) {
    return getData(out, len, false, false);
}

BufferStatic::Result BufferStatic::peekData(char *out, unsigned int len) {
    return getData(out, len, true, false);
}

BufferStatic::Result BufferStatic::seekData(unsigned int len) {
    printf("%s %u\n", __PRETTY_FUNCTION__, len);
    return getData(nullptr, len, false, true);
}

BufferStatic::Result BufferStatic::getData(char *out, unsigned int len, bool peek, bool moveHeadOnly) {
    if ((peek && moveHeadOnly))
        return Result::InvalidUsage;

    auto validateBuffer = [size = this->size] (unsigned int len) {
        if (size == 0)
            return Result::NoData;
        else if (size < len)
            return Result::InvalidLen;
        return Result::Ok;
    };
    if (auto valid = validateBuffer(len); valid != Result::Ok)
        return valid;

    if (tail > head) {
        // normal data emplacement
        if (head + len <= tail) {
            // most general case, data in one block
            if (!moveHeadOnly && out)
                memcpy(out, buffer + head, len);

            if (!peek)
                head += len;
        } else {
            return Result::InvalidLen;
        }
    } else /*if (tail <= head)*/ {  // if (tail == head) then buffer is fulfilled
        // data emplacement separated on two blocks: at the end and at the beginning of buffer
        if (head + len <= capacity) {
            // data in one block, from head to buffer end
            if (!moveHeadOnly)
                memcpy(out, buffer + head, len);

            if (!peek)
                head += len;
        } else /*if (head + len > capacity)*/ {
            // data emplacement separated on two blocks: at the end and at the beginning of buffer
            unsigned int firstBlockLen = capacity - head;
            unsigned int secondBlockLen = len - firstBlockLen;
            if (tail > 0 && tail >= secondBlockLen) {
                if (!moveHeadOnly) {
                    memcpy(out, buffer + head, firstBlockLen);
                    memcpy(out + firstBlockLen, buffer, secondBlockLen);
                }

                if (!peek)
                    head = secondBlockLen;
            }
            else {
                return Result::InvalidLen;
            }
        }
    }

    if (!peek)
        size -= len;

    return Result::Ok;
}

char *BufferStatic::getDataPtr() const {
    return buffer + head;
}

unsigned int BufferStatic::getSize() const {
    return size;
}

unsigned int BufferStatic::getCapacity() const {
    return capacity;
}

unsigned int BufferStatic::getTotalFreeSpace() const {
    return capacity - size;
}

unsigned int BufferStatic::getHeadPos() const {
    return head;
}

unsigned int BufferStatic::getTailPos() const {
    return tail;
}

unsigned int BufferStatic::getLinearFreeSpace() const {
    // current maximum linear buffer size
    if (tail == capacity) // tail at last position
        return getTotalFreeSpace();
    else if (head < tail) // normal linear data emplacement
        return capacity - tail;
    else if (head > tail)  // data separated in two blocks
        return head - tail;
    else
        return capacity - tail;
}

char *BufferStatic::getLinearAppendPtr() {
    if (tail == capacity) {  // tail at last position
        if (capacity != size) {
            // buffer has free space at the beginning
            tail = 0;
        }
    }
    return (buffer + tail);
}

BufferStatic::Result BufferStatic::expandSize(unsigned int len) {
    if (size + len > capacity)
        return Result::InvalidLen;

    tail += len;
    size += len;

    return Result::Ok;
}

void BufferStatic::normilize() {
    if (head == 0)
        return;

    if (size != 0) {
        if (tail == 0 || tail > head) { // normal data emplacement
            memcpy(buffer, buffer + head, size);
        } else /*if (tail <= head)*/ {
            // data emplacement separated on two blocks: at the end and at the beginning of buffer
            unsigned int firstBlockLen = capacity - head;
            unsigned int secondBlockLen = size - firstBlockLen;

            // move and reorder blocks
            char temp[secondBlockLen];
            memcpy(temp, buffer, secondBlockLen);
            memcpy(buffer, buffer + head, firstBlockLen);
            memcpy(buffer + firstBlockLen, temp, secondBlockLen);
        }
    }

    head = 0;
    tail = size;
}

void BufferStatic::clear() {
    size = 0;
    head = 0;
    tail = 0;
}
