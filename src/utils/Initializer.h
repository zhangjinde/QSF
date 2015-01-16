// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

// Initialize per-thread environment
class Initializer
{
public:
    Initializer();
    ~Initializer();

    Initializer(const Initializer&) = delete;
    Initializer& operator = (const Initializer&) = delete;

private:
    void init();
    void deinit();
};