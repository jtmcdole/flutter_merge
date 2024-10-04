// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_TESTING_MOCK_TEXT_INPUT_HANDLER_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_TESTING_MOCK_TEXT_INPUT_HANDLER_H_

#include <gdk/gdk.h>

#include "flutter/shell/platform/linux/fl_text_input_handler.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(FlMockTextInputHandler,
                     fl_mock_text_input_handler,
                     FL,
                     MOCK_TEXT_INPUT_HANDLER,
                     FlTextInputHandler)

FlMockTextInputHandler* fl_mock_text_input_handler_new(
    gboolean (*filter_keypress)(FlTextInputHandler* self, FlKeyEvent* event));

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_TESTING_MOCK_TEXT_INPUT_HANDLER_H_
