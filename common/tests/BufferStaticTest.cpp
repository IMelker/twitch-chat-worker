//
// Created by l2pic on 30.03.2021.
//
#include "../BufferStatic.h"
#include <gtest/gtest.h>

//-----------------------------------------------------------------------------
TEST(Basic, AppendAndGet) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering;
    ASSERT_TRUE(buffering.getCapacity() == DEFAULT_CAPACITY); //default buffer length
    ASSERT_TRUE(BufferStatic::Result::NoData == buffering.getData(dataOut, 3));

    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "aaa", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ(buffering.getSize(), 3);

    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "bbb", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ(buffering.getSize(), 6);

    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 7));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 6));
    EXPECT_STREQ("aaabbb", dataOut);
    EXPECT_EQ   (buffering.getSize(), 0);
}

//-----------------------------------------------------------------------------
TEST(Basic, AppendAndGetLoop) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(9);
    ASSERT_TRUE(buffering.getCapacity() == 9); //default buffer length
    EXPECT_EQ(buffering.getSize(), 0);

    for (int i = 0; i < 30; i++) {
        memset(data, 0x00, sizeof(data));
        memcpy(data, (void *) "aaa", 3);
        ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));

        memset(data, 0x00, sizeof(data));
        memcpy(data, (void *) "abbb", 4);
        ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 4));
        EXPECT_EQ  (buffering.getSize(), 7);

        memset(dataOut, 0x00, sizeof(dataOut));
        ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 8));
        ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 4));
        EXPECT_STREQ("aaaa", dataOut);

        memset(dataOut, 0x00, sizeof(dataOut));
        ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 3));
        EXPECT_STREQ("bbb", dataOut);
    }
}

//-----------------------------------------------------------------------------
TEST(Basic, LinearTest1) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    EXPECT_EQ  (buffering.getCapacity(), 10);
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 0);
    EXPECT_EQ  (buffering.getSize(), 0);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 10);

// append 7
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "a.....b", 7);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 7));
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 7);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3);

// get 4
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 4));
    EXPECT_STREQ("a...", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 4);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getTotalFreeSpace(), 7);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3);

// append 5
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "c***d", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));
    EXPECT_EQ  (buffering.getHeadPos(), 4);
    EXPECT_EQ  (buffering.getTailPos(), 2);
    EXPECT_EQ  (buffering.getSize(), 8);
    EXPECT_EQ  (buffering.getTotalFreeSpace(), 2);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 2);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 8));
    EXPECT_EQ(strncmp("..bc***d", dataOut, 8), 0);
}

//-----------------------------------------------------------------------------
TEST(Basic, LinearTest2) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 10);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 10);

    memcpy(data, (void *) "abcde", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 5);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 5);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 5));
    EXPECT_EQ(strncmp("abcde", dataOut, 5), 0);
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 5);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 5);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.seekData(2));
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 3));
    EXPECT_EQ(strncmp("cde", dataOut, 3), 0);
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 7);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 5);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.seekData(2));
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 9);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 5);
}

//-----------------------------------------------------------------------------
TEST(Basic, LinearTest3) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    EXPECT_EQ  (buffering.getCapacity(), 10);
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 0);
    EXPECT_EQ  (buffering.getSize(), 0);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 10);

// append 3
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "aaa", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 3);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 7);

// append 4
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "bbbb", 4);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 4));
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 7);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3);

// append 3
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ccc", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 10);
    EXPECT_EQ  (buffering.getSize(), 10);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 0);

// get 5
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 5));
    EXPECT_STREQ("aaabb", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 5);
    EXPECT_EQ  (buffering.getTailPos(), 10);
    EXPECT_EQ  (buffering.getSize(), 5);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 5);

// get 2
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 2));
    EXPECT_STREQ("bb", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 7);
    EXPECT_EQ  (buffering.getTailPos(), 10);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 7);

// get 3
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 3));
    EXPECT_STREQ("ccc", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 10);
    EXPECT_EQ  (buffering.getTailPos(), 10);
    EXPECT_EQ  (buffering.getSize(), 0);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 10);

// append 3
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ddd", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ  (buffering.getHeadPos(), 10);
    EXPECT_EQ  (buffering.getTailPos(), 3);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 7);

// append 4
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "eeee", 4);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 4));
    EXPECT_EQ  (buffering.getHeadPos(), 10);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 7);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3);

// get 4
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 4));
    EXPECT_STREQ("ddde", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 4);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3); //!!!

// get 3
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 3));
    EXPECT_STREQ("eee", dataOut);
    EXPECT_EQ  (buffering.getHeadPos(), 7);
    EXPECT_EQ  (buffering.getTailPos(), 7);
    EXPECT_EQ  (buffering.getSize(), 0);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 3); //!!!

// append 3
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "fff", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ  (buffering.getHeadPos(), 7);
    EXPECT_EQ  (buffering.getTailPos(), 10);
    EXPECT_EQ  (buffering.getSize(), 3);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 7);
}

//-----------------------------------------------------------------------------
TEST(Basic, LinearTest4) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    memcpy(data, (void *) "abcde", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));
    memcpy(data, (void *) "ghijk", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));

//buffer full
    EXPECT_EQ   (buffering.getHeadPos(), 0);
    EXPECT_EQ   (buffering.getTailPos(), 10);

    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 10));
    EXPECT_STREQ("abcdeghijk", dataOut);

    EXPECT_EQ   (buffering.getHeadPos(), 10);
    EXPECT_EQ   (buffering.getTailPos(), 10);

    ASSERT_TRUE(BufferStatic::Result::NoData == buffering.getData(dataOut, 1));

    EXPECT_EQ   (buffering.getTotalFreeSpace(), 10);
    EXPECT_EQ   (buffering.getLinearFreeSpace(), 10);
}

//-----------------------------------------------------------------------------
TEST(Basic, increaseLinearData) {
    char dataOut[100];

    BufferStatic buffering(10);
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 10);

    memcpy(buffering.getLinearAppendPtr(), (void *) "abcde", 5);

    buffering.expandSize(5);
    EXPECT_EQ   (buffering.getTotalFreeSpace(), 5);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 5));
    EXPECT_EQ(strncmp("abcde", dataOut, 5), 0);
}

//-----------------------------------------------------------------------------
TEST(Basic, seekData) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    memcpy(data, (void *) "abcde", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 5));
    EXPECT_EQ(strncmp("abcde", dataOut, 5), 0);

    ASSERT_TRUE(BufferStatic::Result::InvalidLen == buffering.seekData(11));
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.seekData(2));
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 3));
    EXPECT_EQ(strncmp("cde", dataOut, 3), 0);
    EXPECT_EQ   (buffering.getHeadPos(), 2);
    EXPECT_EQ   (buffering.getSize(), 3);
}


//-----------------------------------------------------------------------------
TEST(Exceptional, BufferFull) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(10);
    ASSERT_TRUE(BufferStatic::Result::InvalidLen == buffering.append(data, 11));
    ASSERT_TRUE(BufferStatic::Result::InvalidLen == buffering.append(data, 20));

    memcpy(data, (void *) "aaaaa", 5);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 5));

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 5));
    EXPECT_EQ(strncmp("aaaaa", dataOut, 5), 0);

    ASSERT_TRUE(BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 10));

//--------
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "bbbbb", 5);
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.append(data, 5));

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 10));
    EXPECT_EQ(strncmp("aaaaabbbbb", dataOut, 10), 0);

    EXPECT_EQ   (buffering.getSize(), 10);
    EXPECT_EQ   (buffering.getTailPos(), 10);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 11));
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 20));

//-------- append when full
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ccc", 3);
    ASSERT_TRUE(BufferStatic::Result::BufferFull == buffering.append(data, 3));
    EXPECT_EQ   (buffering.getSize(), 10);

//--------
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 3));
    EXPECT_STREQ("aaa", dataOut);
    EXPECT_EQ   (buffering.getHeadPos(), 3);
    EXPECT_EQ   (buffering.getSize(), 7);
    ASSERT_TRUE (BufferStatic::Result::BufferFull == buffering.append(data, 4));
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 8));

//--------
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ccc", 3);
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.append(data, 3));

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 10));
    EXPECT_EQ(strncmp("aabbbbbccc", dataOut, 10), 0);

    EXPECT_EQ   (buffering.getTailPos(), 3);
    EXPECT_EQ   (buffering.getSize(), 10);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 11));

//-------- append when full
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ddd", 3);
    ASSERT_TRUE(BufferStatic::Result::BufferFull == buffering.append(data, 3));
    EXPECT_EQ   (buffering.getSize(), 10);

//--------
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 3));
    EXPECT_STREQ("aab", dataOut);
    EXPECT_EQ   (buffering.getHeadPos(), 6);
    EXPECT_EQ   (buffering.getSize(), 7);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 8));

//--------
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "ddd", 3);
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ   (buffering.getTailPos(), 6);
    EXPECT_EQ   (buffering.getHeadPos(), 6);

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 10));
    EXPECT_EQ(strncmp("bbbbcccddd", dataOut, 10), 0);

    EXPECT_EQ   (buffering.getSize(), 10);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 11));

//-------- append when full
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "eeeeee", 6);
    ASSERT_TRUE(BufferStatic::Result::BufferFull == buffering.append(data, 6));
    EXPECT_EQ   (buffering.getSize(), 10);

//--------
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 6));
    EXPECT_STREQ("bbbbcc", dataOut);
    EXPECT_EQ   (buffering.getHeadPos(), 2);
    EXPECT_EQ   (buffering.getTailPos(), 6);
    EXPECT_EQ   (buffering.getSize(), 4);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 5));

//--------
    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "eeeeee", 6);
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.append(data, 6));

    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.peekData(dataOut, 10));
    EXPECT_EQ(strncmp("cdddeeeeee", dataOut, 10), 0);

    EXPECT_EQ   (buffering.getHeadPos(), 2);
    EXPECT_EQ   (buffering.getTailPos(), 2);
    EXPECT_EQ   (buffering.getSize(), 10);
    ASSERT_TRUE (BufferStatic::Result::InvalidLen == buffering.getData(dataOut, 11));

//--------
    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 10));
    EXPECT_STREQ("cdddeeeeee", dataOut);
    EXPECT_EQ   (buffering.getHeadPos(), 2);
    EXPECT_EQ   (buffering.getTailPos(), 2);
    EXPECT_EQ   (buffering.getSize(), 0);
}

//-----------------------------------------------------------------------------
TEST(Basic, DirectWriteGet) {
    BufferStatic buffering(10);
    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 0);
    EXPECT_EQ  (buffering.getSize(), 0);
    EXPECT_EQ  (buffering.getTotalFreeSpace(), 10);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 10);

//direct write to buffer
    memcpy(buffering.getLinearAppendPtr(), (void *) "abcde", 5);
    buffering.expandSize(5); //after write, increaseLinearData should be called

    EXPECT_EQ  (buffering.getHeadPos(), 0);
    EXPECT_EQ  (buffering.getTailPos(), 5);
    EXPECT_EQ  (buffering.getSize(), 5);
    EXPECT_EQ  (buffering.getTotalFreeSpace(), 5);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 5);

//direct access to read buffer (for no memcpy)
    EXPECT_EQ(strncmp("abcde", buffering.getDataPtr(), 5), 0);
    buffering.seekData(5); //when buffer usage is finished, seekData should be called

    EXPECT_EQ  (buffering.getHeadPos(), 5);
    EXPECT_EQ  (buffering.getTailPos(), 5);
    EXPECT_EQ  (buffering.getTotalFreeSpace(), 10);
    EXPECT_EQ  (buffering.getLinearFreeSpace(), 5); //XXX linear available space !
    EXPECT_EQ  (buffering.getSize(), 0);
}

//-----------------------------------------------------------------------------
TEST(Basic, NormilizeClear) {
    char data[100];
    char dataOut[100];

    BufferStatic buffering(16);
    ASSERT_TRUE(buffering.getCapacity() == 16); //default buffer length
    EXPECT_EQ(buffering.getSize(), 0);

    // move head
    {
        ASSERT_TRUE(BufferStatic::Result::Ok == buffering.expandSize(10));
        EXPECT_EQ(buffering.getSize(), 10);
        EXPECT_EQ(buffering.getTailPos(), 10);

        ASSERT_TRUE(BufferStatic::Result::Ok == buffering.seekData(10));
        EXPECT_EQ(buffering.getHeadPos(), 10);
        EXPECT_EQ(buffering.getTailPos(), 10);
        EXPECT_EQ(buffering.getSize(), 0);
    }

    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "aaa", 3);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 3));
    EXPECT_EQ(buffering.getTailPos(), 13);
    EXPECT_EQ(buffering.getSize(), 3);

    buffering.normilize();
    EXPECT_EQ(buffering.getHeadPos(), 0);
    EXPECT_EQ(buffering.getTailPos(), 3);
    EXPECT_EQ(buffering.getSize(), 3);

    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "abbb", 4);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 4));
    EXPECT_EQ(buffering.getHeadPos(), 0);
    EXPECT_EQ(buffering.getTailPos(), 7);
    EXPECT_EQ(buffering.getSize(), 7);

    buffering.clear();
    EXPECT_EQ(buffering.getHeadPos(), 0);
    EXPECT_EQ(buffering.getTailPos(), 0);
    EXPECT_EQ(buffering.getSize(), 0);

    buffering.expandSize(10);
    buffering.seekData(10);

    memset(data, 0x00, sizeof(data));
    memcpy(data, (void *) "abbcccdddd", 10);
    ASSERT_TRUE(BufferStatic::Result::Ok == buffering.append(data, 10));
    EXPECT_EQ(buffering.getHeadPos(), 10);
    EXPECT_EQ(buffering.getTailPos(), 4);
    EXPECT_EQ(buffering.getSize(), 10);

    buffering.normilize();
    EXPECT_EQ(buffering.getHeadPos(), 0);
    EXPECT_EQ(buffering.getTailPos(), 10);
    EXPECT_EQ(buffering.getSize(), 10);

    memset(dataOut, 0x00, sizeof(dataOut));
    ASSERT_TRUE (BufferStatic::Result::Ok == buffering.getData(dataOut, 10));
    EXPECT_STREQ("abbcccdddd", dataOut);

    // TODO check border values
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    //::testing::GTEST_FLAG(filter) = "Basic.increaseLinearData";
    //::testing::GTEST_FLAG(filter) = "Basic.LinearFreeSpaceBoundaryCase";
    //::testing::GTEST_FLAG(filter) = "Basic.Calculations";
    //::testing::GTEST_FLAG(filter) = "Basic.DirectWriteGet";
    return RUN_ALL_TESTS();
}
