/*
 * Copyright 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ISP4_OFFSET_BYTE_HEADER
#define ISP4_OFFSET_BYTE_HEADER

#define ISP_SOFT_RESET                                0x62000
#define ISP_SYS_INT0_EN                               0x62010
#define ISP_SYS_INT0_STATUS                           0x62014
#define ISP_SYS_INT0_ACK                              0x62018
#define ISP_SYS_INT1_EN                               0x6201C
#define ISP_SYS_INT1_STATUS                           0x62020
#define ISP_SYS_INT1_ACK                              0x62024
#define ISP_SYS_INT2_EN                               0x62028
#define ISP_SYS_INT2_STATUS                           0x6202C
#define ISP_SYS_INT2_ACK                              0x62030
#define ISP_SYS_INT3_EN                               0x62034
#define ISP_SYS_INT3_STATUS                           0x62038
#define ISP_SYS_INT3_ACK                              0x6203C
#define ISP_SYS_INT4_EN                               0x62040
#define ISP_SYS_INT4_STATUS                           0x62044
#define ISP_SYS_INT4_ACK                              0x62048
#define ISP_CCPU_CNTL                                 0x62054
#define ISP_STATUS                                    0x62058
#define ISP_LOG_RB_BASE_LO0                           0x62148
#define ISP_LOG_RB_BASE_HI0                           0x6214C
#define ISP_LOG_RB_SIZE0                              0x62150
#define ISP_LOG_RB_RPTR0                              0x62154
#define ISP_LOG_RB_WPTR0                              0x62158
#define ISP_RB_BASE_LO1                               0x62170
#define ISP_RB_BASE_HI1                               0x62174
#define ISP_RB_SIZE1                                  0x62178
#define ISP_RB_RPTR1                                  0x6217C
#define ISP_RB_WPTR1                                  0x62180
#define ISP_RB_BASE_LO2                               0x62184
#define ISP_RB_BASE_HI2                               0x62188
#define ISP_RB_SIZE2                                  0x6218C
#define ISP_RB_RPTR2                                  0x62190
#define ISP_RB_WPTR2                                  0x62194
#define ISP_RB_BASE_LO3                               0x62198
#define ISP_RB_BASE_HI3                               0x6219C
#define ISP_RB_SIZE3                                  0x621A0
#define ISP_RB_RPTR3                                  0x621A4
#define ISP_RB_WPTR3                                  0x621A8
#define ISP_RB_BASE_LO4                               0x621AC
#define ISP_RB_BASE_HI4                               0x621B0
#define ISP_RB_SIZE4                                  0x621B4
#define ISP_RB_RPTR4                                  0x621B8
#define ISP_RB_WPTR4                                  0x621BC
#define ISP_RB_BASE_LO5                               0x621C0
#define ISP_RB_BASE_HI5                               0x621C4
#define ISP_RB_SIZE5                                  0x621C8
#define ISP_RB_RPTR5                                  0x621CC
#define ISP_RB_WPTR5                                  0x621D0
#define ISP_RB_BASE_LO6                               0x621D4
#define ISP_RB_BASE_HI6                               0x621D8
#define ISP_RB_SIZE6                                  0x621DC
#define ISP_RB_RPTR6                                  0x621E0
#define ISP_RB_WPTR6                                  0x621E4
#define ISP_RB_BASE_LO7                               0x621E8
#define ISP_RB_BASE_HI7                               0x621EC
#define ISP_RB_SIZE7                                  0x621F0
#define ISP_RB_RPTR7                                  0x621F4
#define ISP_RB_WPTR7                                  0x621F8
#define ISP_RB_BASE_LO8                               0x621FC
#define ISP_RB_BASE_HI8                               0x62200
#define ISP_RB_SIZE8                                  0x62204
#define ISP_RB_RPTR8                                  0x62208
#define ISP_RB_WPTR8                                  0x6220C
#define ISP_RB_BASE_LO9                               0x62210
#define ISP_RB_BASE_HI9                               0x62214
#define ISP_RB_SIZE9                                  0x62218
#define ISP_RB_RPTR9                                  0x6221C
#define ISP_RB_WPTR9                                  0x62220
#define ISP_RB_BASE_LO10                              0x62224
#define ISP_RB_BASE_HI10                              0x62228
#define ISP_RB_SIZE10                                 0x6222C
#define ISP_RB_RPTR10                                 0x62230
#define ISP_RB_WPTR10                                 0x62234
#define ISP_RB_BASE_LO11                              0x62238
#define ISP_RB_BASE_HI11                              0x6223C
#define ISP_RB_SIZE11                                 0x62240
#define ISP_RB_RPTR11                                 0x62244
#define ISP_RB_WPTR11                                 0x62248
#define ISP_RB_BASE_LO12                              0x6224C
#define ISP_RB_BASE_HI12                              0x62250
#define ISP_RB_SIZE12                                 0x62254
#define ISP_RB_RPTR12                                 0x62258
#define ISP_RB_WPTR12                                 0x6225C
#define ISP_RB_BASE_LO13                              0x62260
#define ISP_RB_BASE_HI13                              0x62264
#define ISP_RB_SIZE13                                 0x62268
#define ISP_RB_RPTR13                                 0x6226C
#define ISP_RB_WPTR13                                 0x62270
#define ISP_RB_BASE_LO14                              0x62274
#define ISP_RB_BASE_HI14                              0x62278
#define ISP_RB_SIZE14                                 0x6227C
#define ISP_RB_RPTR14                                 0x62280
#define ISP_RB_WPTR14                                 0x62284
#define ISP_RB_BASE_LO15                              0x62288
#define ISP_RB_BASE_HI15                              0x6228C
#define ISP_RB_SIZE15                                 0x62290
#define ISP_RB_RPTR15                                 0x62294
#define ISP_RB_WPTR15                                 0x62298
#define ISP_RB_BASE_LO16                              0x6229C
#define ISP_RB_BASE_HI16                              0x622A0
#define ISP_RB_SIZE16                                 0x622A4
#define ISP_RB_RPTR16                                 0x622A8
#define ISP_RB_WPTR16                                 0x622AC
#define ISP_RB_BASE_LO17                              0x622B0
#define ISP_RB_BASE_HI17                              0x622B4
#define ISP_RB_SIZE17                                 0x622B8
#define ISP_RB_RPTR17                                 0x622BC
#define ISP_RB_WPTR17                                 0x622C0
#define ISP_RB_BASE_LO18                              0x622C4
#define ISP_RB_BASE_HI18                              0x622C8
#define ISP_RB_SIZE18                                 0x622CC
#define ISP_RB_RPTR18                                 0x622D0
#define ISP_RB_WPTR18                                 0x622D4
#define ISP_RB_BASE_LO19                              0x622D8
#define ISP_RB_BASE_HI19                              0x622DC
#define ISP_RB_SIZE19                                 0x622E0
#define ISP_RB_RPTR19                                 0x622E4
#define ISP_RB_WPTR19                                 0x622E8
#define ISP_RB_BASE_LO20                              0x622EC
#define ISP_RB_BASE_HI20                              0x622F0
#define ISP_RB_SIZE20                                 0x622F4
#define ISP_RB_RPTR20                                 0x622F8
#define ISP_RB_WPTR20                                 0x622FC
#define ISP_RB_BASE_LO21                              0x62300
#define ISP_RB_BASE_HI21                              0x62304
#define ISP_RB_SIZE21                                 0x62308
#define ISP_RB_RPTR21                                 0x6230C
#define ISP_RB_WPTR21                                 0x62310
#define ISP_RB_BASE_LO22                              0x62314
#define ISP_RB_BASE_HI22                              0x62318
#define ISP_RB_SIZE22                                 0x6231C
#define ISP_RB_RPTR22                                 0x62320
#define ISP_RB_WPTR22                                 0x62324
#define ISP_RB_BASE_LO23                              0x62328
#define ISP_RB_BASE_HI23                              0x6232C
#define ISP_RB_SIZE23                                 0x62330
#define ISP_RB_RPTR23                                 0x62334
#define ISP_RB_WPTR23                                 0x62338
#define ISP_RB_BASE_LO24                              0x6233C
#define ISP_RB_BASE_HI24                              0x62340
#define ISP_RB_SIZE24                                 0x62344
#define ISP_RB_RPTR24                                 0x62348
#define ISP_RB_WPTR24                                 0x6234C
#define ISP_VERSION                                   0x62350
#define ISP_CCPU_PDEBUG_STATUS                        0x623A4

#define ISP_POWER_STATUS                              0x60000
#define ISP_SEMAPHORE_0                               0x60004

#endif
