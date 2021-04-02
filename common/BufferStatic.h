//
// Created by imelker on 30.03.2021.
//

#ifndef CHATSNIFFER_COMMON_BUFFERSTATIC_H_
#define CHATSNIFFER_COMMON_BUFFERSTATIC_H_

#include <cstring>

template <int CAPACITY>
class BufferStatic
{
  public:
    enum class Result{ Ok, NoData, BufferFull, InvalidLen, InvalidUsage };

  public:
    explicit BufferStatic() : capacity(CAPACITY), size(0), head(0), tail(0) {
        static_assert(CAPACITY > 0 && CAPACITY < 1024 * 100);
    }
    virtual ~BufferStatic() = default;

    BufferStatic(const BufferStatic &) = delete;
    BufferStatic &operator=(const BufferStatic &) = delete;

    /// Appends data to the internal buffer
    Result append(char *data, unsigned int len) {
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

    /// Get data from internal buffer
    Result getData(char *out, unsigned int len) {
        return getData(out, len, false, false);
    }

    /// Get data from internal buffer without moving head
    Result peekData(char *out, unsigned int len) {
        return getData(out, len, true, false);
    }

    /// Skip data by moving internal buffer head
    Result seekData(unsigned int len) {
        return getData(nullptr, len, false, true);
    }

    /// Return pointer to the beginning of data(on head)
    [[nodiscard]] const char *getDataPtr() const {
        return buffer + head;
    }
    /// Return held data size
    [[nodiscard]] unsigned int getSize() const {
        return size;
    }
    /// Return the maximum size that can be helds
    [[nodiscard]] unsigned int getCapacity() const {
        return capacity;
    }
    /// Return the maximum sum free buffer(not linear) space
    [[nodiscard]] unsigned int getTotalFreeSpace() const {
        return capacity - size;
    }

    [[nodiscard]] unsigned int getHeadPos() const {
        return head;
    }
    [[nodiscard]] unsigned int getTailPos() const {
        return tail;
    }

    /// Return the free linear buffer size after tail to the buffer end
    [[nodiscard]] unsigned int getLinearFreeSpace() const {
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
    /// Return pointer to free linear space, move cursor to the beginning if buffer is full
    [[nodiscard]] char *getLinearAppendPtr() {
        if (tail == capacity) {  // tail at last position
            if (capacity != size) {
                // buffer has free space at the beginning
                tail = 0;
            }
        }
        return (buffer + tail);
    }
    /// Moves current tail by len in linear space
    Result expandSize(unsigned int len) {
        if (size + len > capacity)
            return Result::InvalidLen;

        tail += len;
        size += len;

        return Result::Ok;
    }
    /// Move data to the buffer start(head == 0, tail == size)
    void normilize() {
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

    void clear() {
        size = 0;
        head = 0;
        tail = 0;
    }
  private:
    BufferStatic::Result getData(char *out, unsigned int len, bool peek, bool moveHeadOnly) {
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

    char buffer[CAPACITY] = {};
    const unsigned int capacity;

    unsigned int size;
    unsigned int head;
    unsigned int tail;
};

#endif //CHATSNIFFER_COMMON_BUFFERSTATIC_H_
