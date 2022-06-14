// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2022 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

static void convolution_winograd_dot_pack8_avx(Mat& bottom_blob_tm, int outch, const Mat& kernel_tm, Mat& top_blob_tm, const Option& opt)
{
    // Mat bottom_blob_tm(tiles, 16/36/64, inch, 32u, 4, opt.workspace_allocator);

    const int tiles = bottom_blob_tm.w;
    const int batch = bottom_blob_tm.h;
    const int inch = bottom_blob_tm.c;

    // permute
    Mat bottom_blob_tm2;
    if (tiles >= 12)
        bottom_blob_tm2.create(12 * inch, tiles / 12 + (tiles % 12) / 8 + (tiles % 12 % 8) / 4 + (tiles % 12 % 4) / 2 + tiles % 12 % 2, batch, 32u, 8, opt.workspace_allocator);
    else if (tiles >= 8)
        bottom_blob_tm2.create(8 * inch, tiles / 8 + (tiles % 8) / 4 + (tiles % 4) / 2 + tiles % 2, batch, 32u, 8, opt.workspace_allocator);
    else if (tiles >= 4)
        bottom_blob_tm2.create(4 * inch, tiles / 4 + (tiles % 4) / 2 + tiles % 2, batch, 32u, 8, opt.workspace_allocator);
    else if (tiles >= 2)
        bottom_blob_tm2.create(2 * inch, tiles / 2 + tiles % 2, batch, 32u, 8, opt.workspace_allocator);
    else // if (tiles >= 1)
        bottom_blob_tm2.create(1 * inch, tiles, batch, 32u, 8, opt.workspace_allocator);

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int r = 0; r < batch; r++)
    {
        Mat tm2 = bottom_blob_tm2.channel(r);

        // tile
        int i = 0;

        for (; i + 11 < tiles; i += 12)
        {
            float* tmpptr = tm2.row(i / 12);

            const float* r0 = bottom_blob_tm;

            r0 += (r * tiles + i) * 8;

            for (int q = 0; q < inch; q++)
            {
                // transpose 8x12
                __m256 _r0 = _mm256_load_ps(r0);
                __m256 _r1 = _mm256_load_ps(r0 + 8);
                __m256 _r2 = _mm256_load_ps(r0 + 8 * 2);
                __m256 _r3 = _mm256_load_ps(r0 + 8 * 3);
                __m256 _r4 = _mm256_load_ps(r0 + 8 * 4);
                __m256 _r5 = _mm256_load_ps(r0 + 8 * 5);
                __m256 _r6 = _mm256_load_ps(r0 + 8 * 6);
                __m256 _r7 = _mm256_load_ps(r0 + 8 * 7);
                __m256 _r8 = _mm256_load_ps(r0 + 8 * 8);
                __m256 _r9 = _mm256_load_ps(r0 + 8 * 9);
                __m256 _ra = _mm256_load_ps(r0 + 8 * 10);
                __m256 _rb = _mm256_load_ps(r0 + 8 * 11);

                __m256 _tmp0 = _mm256_unpacklo_ps(_r0, _r1);
                __m256 _tmp1 = _mm256_unpackhi_ps(_r0, _r1);
                __m256 _tmp2 = _mm256_unpacklo_ps(_r2, _r3);
                __m256 _tmp3 = _mm256_unpackhi_ps(_r2, _r3);
                __m256 _tmp4 = _mm256_unpacklo_ps(_r4, _r5);
                __m256 _tmp5 = _mm256_unpackhi_ps(_r4, _r5);
                __m256 _tmp6 = _mm256_unpacklo_ps(_r6, _r7);
                __m256 _tmp7 = _mm256_unpackhi_ps(_r6, _r7);
                __m256 _tmp8 = _mm256_unpacklo_ps(_r8, _r9);
                __m256 _tmp9 = _mm256_unpackhi_ps(_r8, _r9);
                __m256 _tmpa = _mm256_unpacklo_ps(_ra, _rb);
                __m256 _tmpb = _mm256_unpackhi_ps(_ra, _rb);
                __m256 _tmpc = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpd = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpe = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpf = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpg = _mm256_shuffle_ps(_tmp4, _tmp6, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmph = _mm256_shuffle_ps(_tmp4, _tmp6, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpi = _mm256_shuffle_ps(_tmp5, _tmp7, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpj = _mm256_shuffle_ps(_tmp5, _tmp7, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpk = _mm256_shuffle_ps(_tmp8, _tmpa, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpl = _mm256_shuffle_ps(_tmp8, _tmpa, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpm = _mm256_shuffle_ps(_tmp9, _tmpb, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpn = _mm256_shuffle_ps(_tmp9, _tmpb, _MM_SHUFFLE(3, 2, 3, 2));
                _r0 = _mm256_permute2f128_ps(_tmpc, _tmpg, _MM_SHUFFLE(0, 2, 0, 0));
                _r1 = _mm256_permute2f128_ps(_tmpk, _tmpd, _MM_SHUFFLE(0, 2, 0, 0));
                _r2 = _mm256_permute2f128_ps(_tmph, _tmpl, _MM_SHUFFLE(0, 2, 0, 0));
                _r3 = _mm256_permute2f128_ps(_tmpe, _tmpi, _MM_SHUFFLE(0, 2, 0, 0));
                _r4 = _mm256_permute2f128_ps(_tmpm, _tmpf, _MM_SHUFFLE(0, 2, 0, 0));
                _r5 = _mm256_permute2f128_ps(_tmpj, _tmpn, _MM_SHUFFLE(0, 2, 0, 0));
                _r6 = _mm256_permute2f128_ps(_tmpc, _tmpg, _MM_SHUFFLE(0, 3, 0, 1));
                _r7 = _mm256_permute2f128_ps(_tmpk, _tmpd, _MM_SHUFFLE(0, 3, 0, 1));
                _r8 = _mm256_permute2f128_ps(_tmph, _tmpl, _MM_SHUFFLE(0, 3, 0, 1));
                _r9 = _mm256_permute2f128_ps(_tmpe, _tmpi, _MM_SHUFFLE(0, 3, 0, 1));
                _ra = _mm256_permute2f128_ps(_tmpm, _tmpf, _MM_SHUFFLE(0, 3, 0, 1));
                _rb = _mm256_permute2f128_ps(_tmpj, _tmpn, _MM_SHUFFLE(0, 3, 0, 1));

                _mm256_store_ps(tmpptr, _r0);
                _mm256_store_ps(tmpptr + 8, _r1);
                _mm256_store_ps(tmpptr + 8 * 2, _r2);
                _mm256_store_ps(tmpptr + 8 * 3, _r3);
                _mm256_store_ps(tmpptr + 8 * 4, _r4);
                _mm256_store_ps(tmpptr + 8 * 5, _r5);
                _mm256_store_ps(tmpptr + 8 * 6, _r6);
                _mm256_store_ps(tmpptr + 8 * 7, _r7);
                _mm256_store_ps(tmpptr + 8 * 8, _r8);
                _mm256_store_ps(tmpptr + 8 * 9, _r9);
                _mm256_store_ps(tmpptr + 8 * 10, _ra);
                _mm256_store_ps(tmpptr + 8 * 11, _rb);

                tmpptr += 96;
                r0 += bottom_blob_tm.cstep * 8;
            }
        }
        for (; i + 7 < tiles; i += 8)
        {
            float* tmpptr = tm2.row(i / 12 + (i % 12) / 8);

            const float* r0 = bottom_blob_tm;

            r0 += (r * tiles + i) * 8;

            for (int q = 0; q < inch; q++)
            {
                // transpose 8x8
                __m256 _r0 = _mm256_load_ps(r0);
                __m256 _r1 = _mm256_load_ps(r0 + 8);
                __m256 _r2 = _mm256_load_ps(r0 + 8 * 2);
                __m256 _r3 = _mm256_load_ps(r0 + 8 * 3);
                __m256 _r4 = _mm256_load_ps(r0 + 8 * 4);
                __m256 _r5 = _mm256_load_ps(r0 + 8 * 5);
                __m256 _r6 = _mm256_load_ps(r0 + 8 * 6);
                __m256 _r7 = _mm256_load_ps(r0 + 8 * 7);

                __m256 _tmp0 = _mm256_unpacklo_ps(_r0, _r1);
                __m256 _tmp1 = _mm256_unpackhi_ps(_r0, _r1);
                __m256 _tmp2 = _mm256_unpacklo_ps(_r2, _r3);
                __m256 _tmp3 = _mm256_unpackhi_ps(_r2, _r3);
                __m256 _tmp4 = _mm256_unpacklo_ps(_r4, _r5);
                __m256 _tmp5 = _mm256_unpackhi_ps(_r4, _r5);
                __m256 _tmp6 = _mm256_unpacklo_ps(_r6, _r7);
                __m256 _tmp7 = _mm256_unpackhi_ps(_r6, _r7);
                __m256 _tmp8 = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmp9 = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpa = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpb = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpc = _mm256_shuffle_ps(_tmp4, _tmp6, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpd = _mm256_shuffle_ps(_tmp4, _tmp6, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmpe = _mm256_shuffle_ps(_tmp5, _tmp7, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmpf = _mm256_shuffle_ps(_tmp5, _tmp7, _MM_SHUFFLE(3, 2, 3, 2));
                _r0 = _mm256_permute2f128_ps(_tmp8, _tmpc, _MM_SHUFFLE(0, 2, 0, 0));
                _r1 = _mm256_permute2f128_ps(_tmp9, _tmpd, _MM_SHUFFLE(0, 2, 0, 0));
                _r2 = _mm256_permute2f128_ps(_tmpa, _tmpe, _MM_SHUFFLE(0, 2, 0, 0));
                _r3 = _mm256_permute2f128_ps(_tmpb, _tmpf, _MM_SHUFFLE(0, 2, 0, 0));
                _r4 = _mm256_permute2f128_ps(_tmp8, _tmpc, _MM_SHUFFLE(0, 3, 0, 1));
                _r5 = _mm256_permute2f128_ps(_tmp9, _tmpd, _MM_SHUFFLE(0, 3, 0, 1));
                _r6 = _mm256_permute2f128_ps(_tmpa, _tmpe, _MM_SHUFFLE(0, 3, 0, 1));
                _r7 = _mm256_permute2f128_ps(_tmpb, _tmpf, _MM_SHUFFLE(0, 3, 0, 1));

                _mm256_store_ps(tmpptr, _r0);
                _mm256_store_ps(tmpptr + 8, _r1);
                _mm256_store_ps(tmpptr + 8 * 2, _r2);
                _mm256_store_ps(tmpptr + 8 * 3, _r3);
                _mm256_store_ps(tmpptr + 8 * 4, _r4);
                _mm256_store_ps(tmpptr + 8 * 5, _r5);
                _mm256_store_ps(tmpptr + 8 * 6, _r6);
                _mm256_store_ps(tmpptr + 8 * 7, _r7);

                tmpptr += 64;
                r0 += bottom_blob_tm.cstep * 8;
            }
        }
        for (; i + 3 < tiles; i += 4)
        {
            float* tmpptr = tm2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4);

            const float* r0 = bottom_blob_tm;

            r0 += (r * tiles + i) * 8;

            for (int q = 0; q < inch; q++)
            {
                // transpose 8x4
                __m256 _r0 = _mm256_load_ps(r0);
                __m256 _r1 = _mm256_load_ps(r0 + 8);
                __m256 _r2 = _mm256_load_ps(r0 + 8 * 2);
                __m256 _r3 = _mm256_load_ps(r0 + 8 * 3);

                __m256 _tmp0 = _mm256_unpacklo_ps(_r0, _r1);
                __m256 _tmp1 = _mm256_unpackhi_ps(_r0, _r1);
                __m256 _tmp2 = _mm256_unpacklo_ps(_r2, _r3);
                __m256 _tmp3 = _mm256_unpackhi_ps(_r2, _r3);
                __m256 _tmp4 = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmp5 = _mm256_shuffle_ps(_tmp0, _tmp2, _MM_SHUFFLE(3, 2, 3, 2));
                __m256 _tmp6 = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(1, 0, 1, 0));
                __m256 _tmp7 = _mm256_shuffle_ps(_tmp1, _tmp3, _MM_SHUFFLE(3, 2, 3, 2));
                _r0 = _mm256_permute2f128_ps(_tmp4, _tmp5, _MM_SHUFFLE(0, 2, 0, 0));
                _r1 = _mm256_permute2f128_ps(_tmp6, _tmp7, _MM_SHUFFLE(0, 2, 0, 0));
                _r2 = _mm256_permute2f128_ps(_tmp4, _tmp5, _MM_SHUFFLE(0, 3, 0, 1));
                _r3 = _mm256_permute2f128_ps(_tmp6, _tmp7, _MM_SHUFFLE(0, 3, 0, 1));

                _mm256_store_ps(tmpptr, _r0);
                _mm256_store_ps(tmpptr + 8, _r1);
                _mm256_store_ps(tmpptr + 8 * 2, _r2);
                _mm256_store_ps(tmpptr + 8 * 3, _r3);

                tmpptr += 32;
                r0 += bottom_blob_tm.cstep * 8;
            }
        }
        for (; i + 1 < tiles; i += 2)
        {
            float* tmpptr = tm2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4 + (i % 12 % 4) / 2);

            const float* r0 = bottom_blob_tm;

            r0 += (r * tiles + i) * 8;

            for (int q = 0; q < inch; q++)
            {
                // transpose 8x2
                __m256 _r0 = _mm256_load_ps(r0);
                __m256 _r1 = _mm256_load_ps(r0 + 8);

                __m256 _tmp0 = _mm256_unpacklo_ps(_r0, _r1);
                __m256 _tmp1 = _mm256_unpackhi_ps(_r0, _r1);
                _r0 = _mm256_permute2f128_ps(_tmp0, _tmp1, _MM_SHUFFLE(0, 2, 0, 0));
                _r1 = _mm256_permute2f128_ps(_tmp0, _tmp1, _MM_SHUFFLE(0, 3, 0, 1));

                _mm256_store_ps(tmpptr, _r0);
                _mm256_store_ps(tmpptr + 8, _r1);

                tmpptr += 16;
                r0 += bottom_blob_tm.cstep * 8;
            }
        }

        for (; i < tiles; i++)
        {
            float* tmpptr = tm2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4 + (i % 12 % 4) / 2 + i % 12 % 2);

            const float* r0 = bottom_blob_tm;
            r0 += (r * tiles + i) * 8;

            for (int q = 0; q < inch; q++)
            {
                __m256 _val = _mm256_load_ps(r0);
                _mm256_store_ps(tmpptr, _val);

                tmpptr += 8;
                r0 += bottom_blob_tm.cstep * 8;
            }
        }
    }

    bottom_blob_tm = Mat();
    // permute end

    top_blob_tm.create(tiles, batch, outch, 32u, 8, opt.workspace_allocator);

    #pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outch; p++)
    {
        float* output0_tm = top_blob_tm.channel(p);

        const Mat kernel0_tm = kernel_tm.channel(p);

        for (int r = 0; r < batch; r++)
        {
            const Mat bb2 = bottom_blob_tm2.channel(r);

            int i = 0;
            for (; i + 11 < tiles; i += 12)
            {
                const float* r0 = bb2.row(i / 12);
                const float* k0 = kernel0_tm.row(r);

                int nn = inch * 8; // inch always > 0

                __m256 _sum0 = _mm256_setzero_ps();
                __m256 _sum1 = _mm256_setzero_ps();
                __m256 _sum2 = _mm256_setzero_ps();
                __m256 _sum3 = _mm256_setzero_ps();
                __m256 _sum4 = _mm256_setzero_ps();
                __m256 _sum5 = _mm256_setzero_ps();
                __m256 _sum6 = _mm256_setzero_ps();
                __m256 _sum7 = _mm256_setzero_ps();
                __m256 _sum8 = _mm256_setzero_ps();
                __m256 _sum9 = _mm256_setzero_ps();
                __m256 _suma = _mm256_setzero_ps();
                __m256 _sumb = _mm256_setzero_ps();

                for (int j = 0; j < nn; j++)
                {
                    __m256 _w0 = _mm256_load_ps(k0);

                    __m256 _val0 = _mm256_broadcast_ss(r0);
                    __m256 _val1 = _mm256_broadcast_ss(r0 + 1);
                    _sum0 = _mm256_comp_fmadd_ps(_val0, _w0, _sum0);
                    _sum1 = _mm256_comp_fmadd_ps(_val1, _w0, _sum1);
                    __m256 _val2 = _mm256_broadcast_ss(r0 + 2);
                    __m256 _val3 = _mm256_broadcast_ss(r0 + 3);
                    _sum2 = _mm256_comp_fmadd_ps(_val2, _w0, _sum2);
                    _sum3 = _mm256_comp_fmadd_ps(_val3, _w0, _sum3);
                    __m256 _val4 = _mm256_broadcast_ss(r0 + 4);
                    __m256 _val5 = _mm256_broadcast_ss(r0 + 5);
                    _sum4 = _mm256_comp_fmadd_ps(_val4, _w0, _sum4);
                    _sum5 = _mm256_comp_fmadd_ps(_val5, _w0, _sum5);
                    __m256 _val6 = _mm256_broadcast_ss(r0 + 6);
                    __m256 _val7 = _mm256_broadcast_ss(r0 + 7);
                    _sum6 = _mm256_comp_fmadd_ps(_val6, _w0, _sum6);
                    _sum7 = _mm256_comp_fmadd_ps(_val7, _w0, _sum7);
                    __m256 _val8 = _mm256_broadcast_ss(r0 + 8);
                    __m256 _val9 = _mm256_broadcast_ss(r0 + 9);
                    _sum8 = _mm256_comp_fmadd_ps(_val8, _w0, _sum8);
                    _sum9 = _mm256_comp_fmadd_ps(_val9, _w0, _sum9);
                    __m256 _vala = _mm256_broadcast_ss(r0 + 10);
                    __m256 _valb = _mm256_broadcast_ss(r0 + 11);
                    _suma = _mm256_comp_fmadd_ps(_vala, _w0, _suma);
                    _sumb = _mm256_comp_fmadd_ps(_valb, _w0, _sumb);

                    r0 += 12;
                    k0 += 8;
                }

                _mm256_store_ps(output0_tm, _sum0);
                _mm256_store_ps(output0_tm + 8, _sum1);
                _mm256_store_ps(output0_tm + 8 * 2, _sum2);
                _mm256_store_ps(output0_tm + 8 * 3, _sum3);
                _mm256_store_ps(output0_tm + 8 * 4, _sum4);
                _mm256_store_ps(output0_tm + 8 * 5, _sum5);
                _mm256_store_ps(output0_tm + 8 * 6, _sum6);
                _mm256_store_ps(output0_tm + 8 * 7, _sum7);
                _mm256_store_ps(output0_tm + 8 * 8, _sum8);
                _mm256_store_ps(output0_tm + 8 * 9, _sum9);
                _mm256_store_ps(output0_tm + 8 * 10, _suma);
                _mm256_store_ps(output0_tm + 8 * 11, _sumb);

                output0_tm += 8 * 12;
            }
            for (; i + 7 < tiles; i += 8)
            {
                const float* r0 = bb2.row(i / 12 + (i % 12) / 8);
                const float* k0 = kernel0_tm.row(r);

                int nn = inch * 8; // inch always > 0

                __m256 _sum0 = _mm256_setzero_ps();
                __m256 _sum1 = _mm256_setzero_ps();
                __m256 _sum2 = _mm256_setzero_ps();
                __m256 _sum3 = _mm256_setzero_ps();
                __m256 _sum4 = _mm256_setzero_ps();
                __m256 _sum5 = _mm256_setzero_ps();
                __m256 _sum6 = _mm256_setzero_ps();
                __m256 _sum7 = _mm256_setzero_ps();

                for (int j = 0; j < nn; j++)
                {
                    __m256 _w0 = _mm256_load_ps(k0);

                    __m256 _val0 = _mm256_broadcast_ss(r0);
                    __m256 _val1 = _mm256_broadcast_ss(r0 + 1);
                    _sum0 = _mm256_comp_fmadd_ps(_val0, _w0, _sum0);
                    _sum1 = _mm256_comp_fmadd_ps(_val1, _w0, _sum1);
                    __m256 _val2 = _mm256_broadcast_ss(r0 + 2);
                    __m256 _val3 = _mm256_broadcast_ss(r0 + 3);
                    _sum2 = _mm256_comp_fmadd_ps(_val2, _w0, _sum2);
                    _sum3 = _mm256_comp_fmadd_ps(_val3, _w0, _sum3);
                    __m256 _val4 = _mm256_broadcast_ss(r0 + 4);
                    __m256 _val5 = _mm256_broadcast_ss(r0 + 5);
                    _sum4 = _mm256_comp_fmadd_ps(_val4, _w0, _sum4);
                    _sum5 = _mm256_comp_fmadd_ps(_val5, _w0, _sum5);
                    __m256 _val6 = _mm256_broadcast_ss(r0 + 6);
                    __m256 _val7 = _mm256_broadcast_ss(r0 + 7);
                    _sum6 = _mm256_comp_fmadd_ps(_val6, _w0, _sum6);
                    _sum7 = _mm256_comp_fmadd_ps(_val7, _w0, _sum7);

                    r0 += 8;
                    k0 += 8;
                }

                _mm256_store_ps(output0_tm, _sum0);
                _mm256_store_ps(output0_tm + 8, _sum1);
                _mm256_store_ps(output0_tm + 8 * 2, _sum2);
                _mm256_store_ps(output0_tm + 8 * 3, _sum3);
                _mm256_store_ps(output0_tm + 8 * 4, _sum4);
                _mm256_store_ps(output0_tm + 8 * 5, _sum5);
                _mm256_store_ps(output0_tm + 8 * 6, _sum6);
                _mm256_store_ps(output0_tm + 8 * 7, _sum7);

                output0_tm += 8 * 8;
            }
            for (; i + 3 < tiles; i += 4)
            {
                const float* r0 = bb2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4);
                const float* k0 = kernel0_tm.row(r);

                int nn = inch * 8; // inch always > 0

                __m256 _sum0 = _mm256_setzero_ps();
                __m256 _sum1 = _mm256_setzero_ps();
                __m256 _sum2 = _mm256_setzero_ps();
                __m256 _sum3 = _mm256_setzero_ps();

                for (int j = 0; j < nn; j++)
                {
                    __m256 _w0 = _mm256_load_ps(k0);

                    __m256 _val0 = _mm256_broadcast_ss(r0);
                    __m256 _val1 = _mm256_broadcast_ss(r0 + 1);
                    _sum0 = _mm256_comp_fmadd_ps(_val0, _w0, _sum0);
                    _sum1 = _mm256_comp_fmadd_ps(_val1, _w0, _sum1);
                    __m256 _val2 = _mm256_broadcast_ss(r0 + 2);
                    __m256 _val3 = _mm256_broadcast_ss(r0 + 3);
                    _sum2 = _mm256_comp_fmadd_ps(_val2, _w0, _sum2);
                    _sum3 = _mm256_comp_fmadd_ps(_val3, _w0, _sum3);

                    r0 += 4;
                    k0 += 8;
                }

                _mm256_store_ps(output0_tm, _sum0);
                _mm256_store_ps(output0_tm + 8, _sum1);
                _mm256_store_ps(output0_tm + 8 * 2, _sum2);
                _mm256_store_ps(output0_tm + 8 * 3, _sum3);

                output0_tm += 8 * 4;
            }
            for (; i + 1 < tiles; i += 2)
            {
                const float* r0 = bb2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4 + (i % 12 % 4) / 2);
                const float* k0 = kernel0_tm.row(r);

                int nn = inch * 8; // inch always > 0

                __m256 _sum0 = _mm256_setzero_ps();
                __m256 _sum1 = _mm256_setzero_ps();

                for (int j = 0; j < nn; j++)
                {
                    __m256 _w0 = _mm256_load_ps(k0);

                    __m256 _val0 = _mm256_broadcast_ss(r0);
                    __m256 _val1 = _mm256_broadcast_ss(r0 + 1);
                    _sum0 = _mm256_comp_fmadd_ps(_val0, _w0, _sum0);
                    _sum1 = _mm256_comp_fmadd_ps(_val1, _w0, _sum1);

                    r0 += 2;
                    k0 += 8;
                }

                _mm256_store_ps(output0_tm, _sum0);
                _mm256_store_ps(output0_tm + 8, _sum1);

                output0_tm += 8 * 2;
            }

            for (; i < tiles; i++)
            {
                const float* r0 = bb2.row(i / 12 + (i % 12) / 8 + (i % 12 % 8) / 4 + (i % 12 % 4) / 2 + i % 12 % 2);
                const float* k0 = kernel0_tm.row(r);

                int nn = inch * 8; // inch always > 0

                __m256 _sum0 = _mm256_setzero_ps();

                for (int j = 0; j < nn; j++)
                {
                    __m256 _w0 = _mm256_load_ps(k0);
                    __m256 _val0 = _mm256_broadcast_ss(r0);
                    _sum0 = _mm256_comp_fmadd_ps(_val0, _w0, _sum0);

                    r0 += 1;
                    k0 += 8;
                }

                _mm256_store_ps(output0_tm, _sum0);

                output0_tm += 8;
            }
        }
    }
}
