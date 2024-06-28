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
#ifndef PIPELINE_ID_PUB_H
#define PIPELINE_ID_PUB_H

/*
 * '->' means 'otf' connected sub IPs
 * '=>' means 'dma buffer' chained sub IPs
 */

/*
 * Pipeline: Mipi0=>Isp
 * sub IPs : Mipi0=>Byrp->Rgbp->Mcfp->Yuvp->Mcsc
 */
#define MIPI0_ISP_PIPELINE_ID 0x5f00

/* Pipeline: LME
 * sub IPs: LME
 */
#define LME_PIPELINE_ID 0x80

/*
 * Pipeline: Mipi0CsisCsta0=>LME=>Isp
 * sub IPs : Mipi0->Csis->Cstat0=>LME=>Byrp->Rgbp->Mcfp->Yuvp->Mcsc
 */
#define MIPI0CSISCSTAT0_LME_ISP_PIPELINE_ID 0x5f91

/*
 * Pipeline: Mipi0CsisCstat0=>Isp
 * sub IPs: MIPI0->CSIS->CSTAT0=>Isp
 */
#define MIPI0CSISCSTAT0_ISP_PIPELINE_ID 0x5f11

#endif
