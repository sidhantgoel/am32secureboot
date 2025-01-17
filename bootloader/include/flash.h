/*
 * Library: am32secureboot
 * File:    bootloader/include/flash.h
 * Author:  Sidhant Goel
 *
 * This file is licensed under the MIT License as stated below
 *
 * Copyright (c) 2024 Sidhant Goel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description
 * -----------
 * The headerfile include/flash.h contains the definitions and prototypes
 * for routines that can be used to interact with the flash memory.
 */

#ifndef DEF_FLASH_H
#define DEF_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void flErasePage(uint32_t add);
void flWrite(const uint8_t *data, int length, uint32_t add);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //DEF_FLASH_H