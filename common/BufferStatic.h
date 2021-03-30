//
// Created by l2pic on 30.03.2021.
//

#ifndef CHATSNIFFER_COMMON_BUFFERSTATIC_H_
#define CHATSNIFFER_COMMON_BUFFERSTATIC_H_

#define DEFAULT_CAPACITY (1024 * 4)

class BufferStatic
{
  public:
    enum class Result{ Ok, NoData, BufferFull, InvalidLen, InvalidUsage };

  public:
    explicit BufferStatic(unsigned int capacity = DEFAULT_CAPACITY);
    virtual ~BufferStatic();

    BufferStatic(const BufferStatic &) = delete;
    BufferStatic &operator=(const BufferStatic &) = delete;

    /// Appends data to the internal buffer
    Result append(char *data, unsigned int len);

    /// Get data from internal buffer
    Result getData(char *out, unsigned int len);

    /// Get data from internal buffer without moving head
    Result peekData(char *out, unsigned int len);

    /// Skip data by moving internal buffer head
    Result seekData(unsigned int len);

    /// Return pointer to the beginning of data(on head)
    [[nodiscard]] char *getDataPtr() const;
    /// Return held data size
    [[nodiscard]] unsigned int getSize() const;
    /// Return the maximum size that can be helds
    [[nodiscard]] unsigned int getCapacity() const;
    /// Return the maximum sum free buffer(not linear) space
    [[nodiscard]] unsigned int getTotalFreeSpace() const;

    [[nodiscard]] unsigned int getHeadPos() const;
    [[nodiscard]] unsigned int getTailPos() const;

    /// Return the free linear buffer size after tail to the buffer end
    [[nodiscard]] unsigned int getLinearFreeSpace() const;
    /// Return pointer to free linear space, move cursor to the beginning if buffer is full
    [[nodiscard]] char *getLinearAppendPtr();
    /// Moves current tail by len in linear space
    Result expandSize(unsigned int len);
    /// Move data to the buffer start(head == 0, tail == size)
    void normilize();

    void clear();
  private:
    BufferStatic::Result getData(char *out, unsigned int len, bool peek, bool moveHeadOnly);

    char *buffer;
    const unsigned int capacity;

    unsigned int size;
    unsigned int head;
    unsigned int tail;
};

#endif //CHATSNIFFER_COMMON_BUFFERSTATIC_H_
