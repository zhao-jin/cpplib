// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Simon Wang <simonwang@tencent.com>
// Created: 12/27/11
// Description: poppy test agent command definition

#ifndef POPPY_TEST_POPPY_TEST_COMMAND_H
#define POPPY_TEST_POPPY_TEST_COMMAND_H
#pragma once

#include <string>

namespace poppy {
namespace test  {

extern const std::string kStartServerCommand;
extern const std::string kStartCredentialCommand;
extern const std::string kStopServerCommand;
extern const std::string kEnableServerCommand;
extern const std::string kDisableServerCommand;

} // namespace test
} // namespace poppy

#endif // POPPY_TEST_POPPY_TEST_COMMAND_H
