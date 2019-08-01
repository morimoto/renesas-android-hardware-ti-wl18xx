/*
 * Copyright (C) 2019 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UPIO_TI_TEST_H
#define UPIO_TI_TEST_H

#include "libbt_tests_common.h"

typedef struct {
    iv_stub is_rfkill_disabled_stub;
    iv_stub init_rfkill_stub;
} upio_ti_stubs;

void upio_set_stubs(iv_stub, iv_stub);
upio_ti_stubs* get_upio_stubs(void);
iv_func get_init_rfkill(void);
iv_func get_is_rfkill_disabled(void);

#endif // UPIO_TI_TEST_H
