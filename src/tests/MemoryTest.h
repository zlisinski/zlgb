#pragma once

#include <gtest/gtest.h>
#include "../Memory.h"

class MemoryTest : public ::testing::Test
{
protected:
    MemoryTest();
    ~MemoryTest() override;

    void SetUp() override;
    void TearDown() override;

    //Memory *memory;
};