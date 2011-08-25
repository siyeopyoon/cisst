/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$
  
  Author(s):  Balazs Vagvolgyi
  Created on: 2007 

  (C) Copyright 2006-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include "svlConverters.h"

#define MIN3(a, b, c)       (a <= b) ? \
                                ((a <= c) ? a : c) : \
                                ((b <= c) ? b : c)

#define MAX3(a, b, c)       (a >= b) ? \
                                ((a >= c) ? a : c) : \
                                ((b >= c) ? b : c)


void Converter(svlStreamType intype, svlStreamType outtype, unsigned char* inputbuffer, unsigned char* outputbuffer, unsigned int partsize, int param)
{
    switch (intype) {
        case svlTypeImageRGB:
            if (outtype == svlTypeImageMono8) {
                RGB24toGray8(inputbuffer, outputbuffer, partsize, true, true);
            }
            else {
                RGB24toGray16(inputbuffer, reinterpret_cast<unsigned short*>(outputbuffer), partsize, true, true);
            }
        break;

        case svlTypeImageMono8:
            if (outtype == svlTypeImageRGB) {
                Gray8toRGB24(inputbuffer, outputbuffer, partsize);
            }
            else {
                Gray8toGray16(inputbuffer, reinterpret_cast<unsigned short*>(outputbuffer), partsize);
            }
        break;

        case svlTypeImageMono16:
            if (outtype == svlTypeImageRGB) {
                Gray16toRGB24(reinterpret_cast<unsigned short*>(inputbuffer), outputbuffer, partsize, param);
            }
            else {
                Gray16toGray8(reinterpret_cast<unsigned short*>(inputbuffer), outputbuffer, partsize, param);
            }
        break;

        case svlTypeDepthMap:
            if (outtype == svlTypeImageRGB) {
                    float32toRGB24(reinterpret_cast<float*>(inputbuffer),
                                   outputbuffer,
                                   partsize,
                                   static_cast<float>(0.001 * param));
            }
            else if (outtype == svlTypeImageMono8) {
                   float32toGray8(reinterpret_cast<float*>(inputbuffer),
                                  outputbuffer,
                                  partsize,
                                  static_cast<float>(0.001 * param));
            }
            else {
                    float32toGray16(reinterpret_cast<float*>(inputbuffer),
                                    reinterpret_cast<unsigned short*>(outputbuffer),
                                    partsize,
                                    static_cast<float>(0.001 * param));
            }
        break;

        case svlTypeImageRGBStereo:
            if (outtype == svlTypeImageMono8Stereo) {
                RGB24toGray8(inputbuffer, outputbuffer, partsize, true, true);
            }
            else {
                RGB24toGray16(inputbuffer, reinterpret_cast<unsigned short*>(outputbuffer), partsize, true, true);
            }
        break;

        case svlTypeImageMono8Stereo:
            if (outtype == svlTypeImageRGBStereo) {
                Gray8toRGB24(inputbuffer, outputbuffer, partsize);
            }
            else {
                Gray8toGray16(inputbuffer, reinterpret_cast<unsigned short*>(outputbuffer), partsize);
            }
        break;

        case svlTypeImageMono16Stereo:
            if (outtype == svlTypeImageRGBStereo) {
                Gray16toRGB24(reinterpret_cast<unsigned short*>(inputbuffer), outputbuffer, partsize, param);
            }
            else {
                Gray16toGray8(reinterpret_cast<unsigned short*>(inputbuffer), outputbuffer, partsize, param);
            }
        break;

        case svlTypeInvalid:
        case svlTypeStreamSource:
        case svlTypeStreamSink:
        case svlTypeImageCustom:
        case svlTypeRigidXform:
        case svlTypePointCloud:
        break;
    }
}

void ConvertImage(svlSampleImageBase* inimage, svlSampleImageBase* outimage, int param, unsigned int threads, unsigned int threadid)
{
    unsigned char *inputbuffer, *outputbuffer;
    unsigned int imgsize, partsize, offset, channels;
    svlStreamType intype, outtype;

    intype = inimage->GetType();
    outtype = outimage->GetType();
    channels = inimage->GetVideoChannels();

    if (channels > 1) {
    // multichannel

        for (unsigned int i = 0; i < channels; i ++) {
            imgsize = inimage->GetWidth(i) * inimage->GetHeight(i);
            partsize = imgsize / threads;
            offset = partsize * threadid;

            inputbuffer = reinterpret_cast<unsigned char*>(inimage->GetPointer(i)) + offset * inimage->GetBPP();
            outputbuffer = reinterpret_cast<unsigned char*>(outimage->GetPointer(i)) + offset * outimage->GetBPP();

            Converter(intype, outtype, inputbuffer, outputbuffer, partsize, param);
        }
    }
    else {
    // single channel

        imgsize = inimage->GetWidth() * inimage->GetHeight();
        partsize = imgsize / threads;
        offset = partsize * threadid;

        inputbuffer = reinterpret_cast<unsigned char*>(inimage->GetPointer()) + offset * inimage->GetBPP();
        outputbuffer = reinterpret_cast<unsigned char*>(outimage->GetPointer()) + offset * outimage->GetBPP();

        Converter(intype, outtype, inputbuffer, outputbuffer, partsize, param);
    }
}

void RGB24toGray8(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool accurate, bool bgr)
{
    unsigned int i, sum;
    if (accurate) {
        if (bgr) {
            for (i = 0; i < pixelcount; i ++) {
                sum  = 28  * (*input); input ++;
                sum += 150 * (*input); input ++;
                sum += 77  * (*input); input ++;
                *output = sum >> 8; output ++;
            }
        }
        else {
            for (i = 0; i < pixelcount; i ++) {
                sum  = 77  * (*input); input ++;
                sum += 150 * (*input); input ++;
                sum += 28  * (*input); input ++;
                *output = sum >> 8; output ++;
            }
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            sum  = *input; input ++;
            sum += *input; input ++;
            sum += *input; input ++;
            *output = sum / 3; output ++;
        }
    }
}

void RGB24toGray16(unsigned char* input, unsigned short* output, const unsigned int pixelcount, bool accurate, bool bgr)
{
    unsigned int i, sum;
    if (accurate) {
        if (bgr) {
            for (i = 0; i < pixelcount; i ++) {
                sum  = 56  * (*input); input ++;
                sum += 300 * (*input); input ++;
                sum += 154 * (*input); input ++;
                *output = sum >> 8; output ++;
            }
        }
        else {
            for (i = 0; i < pixelcount; i ++) {
                sum  = 154 * (*input); input ++;
                sum += 300 * (*input); input ++;
                sum += 56  * (*input); input ++;
                *output = sum >> 8; output ++;
            }
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            sum  = *input; input ++;
            sum += *input; input ++;
            sum += *input; input ++;
            *output = sum; output ++;
        }
    }
}

void Gray8toRGB24(unsigned char* input, unsigned char* output, unsigned int pixelcount)
{
    unsigned char chval;
    for (unsigned int i = 0; i < pixelcount; i ++) {
        chval   = *input; input ++;
        *output =  chval; output ++;
        *output =  chval; output ++;
        *output =  chval; output ++;
    }
}

void Gray8toGray16(unsigned char* input, unsigned short* output, unsigned int pixelcount)
{
    for (unsigned int i = 0; i < pixelcount; i ++) {
        *output = *input;
        input ++; output ++;
    }
}

void Gray16toRGB24(unsigned short* input, unsigned char* output, unsigned int pixelcount, const unsigned int shiftdown)
{
    unsigned short shval;
    unsigned char chval;
    for (unsigned int i = 0; i < pixelcount; i ++) {
        shval = (*input) >> shiftdown; input ++;
        if (shval < 256) chval = static_cast<unsigned char>(shval);
        else chval = 255;
        *output =  chval; output ++;
        *output =  chval; output ++;
        *output =  chval; output ++;
    }
}

void Gray16toGray8(unsigned short* input, unsigned char* output, unsigned int pixelcount, const unsigned int shiftdown)
{
    unsigned short shval;
    unsigned char chval;
    for (unsigned int i = 0; i < pixelcount; i ++) {
        shval = (*input) >> shiftdown;
        if (shval < 256) chval = static_cast<unsigned char>(shval);
        else chval = 255;
        *output = chval;
        output ++; input ++;
    }
}

void int32toRGB24(int* input, unsigned char* output, unsigned int pixelcount, const int maxinputvalue)
{
    unsigned int i;
    int ival;
    if (maxinputvalue > 0) {
        int shadingratio = 65280 / maxinputvalue;
        for (i = 0; i < pixelcount; i ++) {
            ival = (*input * shadingratio) >> 8; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
            *output = ival; output ++;
            *output = ival; output ++;
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            ival = *input; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
            *output = ival; output ++;
            *output = ival; output ++;
        }
    }
}

void int32toGray8(int* input, unsigned char* output, unsigned int pixelcount, const int maxinputvalue)
{
    unsigned int i;
    int ival;
    if (maxinputvalue > 0) {
        int shadingratio = 65280 / maxinputvalue;
        for (i = 0; i < pixelcount; i ++) {
            ival = (*input * shadingratio) >> 8; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            ival = *input; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
}

void float32toRGB24(float* input, unsigned char* output, unsigned int pixelcount, const float scalingratio)
{
    int ival;
    unsigned int i;
    float shadingratio = static_cast<float>(256.0 * scalingratio);
    if (scalingratio == 1.0f) {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input); input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
            *output = ival; output ++;
            *output = ival; output ++;
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input * shadingratio) >> 8; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
            *output = ival; output ++;
            *output = ival; output ++;
        }
    }
}

void float32toGray8(float* input, unsigned char* output, unsigned int pixelcount, const float scalingratio)
{
    int ival;
    unsigned int i;
    float shadingratio = static_cast<float>(256.0 * scalingratio);
    if (scalingratio == 1.0f) {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input); input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input * shadingratio) >> 8; input ++;
            if (ival > 255) ival = 255;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
}

void float32toGray16(float* input, unsigned short* output, unsigned int pixelcount, const float scalingratio)
{
    int ival;
    unsigned int i;
    float shadingratio = static_cast<float>(256.0 * scalingratio);
    if (scalingratio == 1.0f) {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input); input ++;
            if (ival > 0xFFFF) ival = 0xFFFF;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
    else {
        for (i = 0; i < pixelcount; i ++) {
            ival = static_cast<int>(*input * shadingratio) >> 8; input ++;
            if (ival > 0xFFFF) ival = 0xFFFF;
            else if (ival < 0) ival = 0;
            *output = ival; output ++;
        }
    }
}

void RGB24toYUV444(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int r, g, b, y, u, v;

    for (unsigned int i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        y = r * 2104 + g * 4130 + b * 802 + 4096 + 131072;
        if (y < 0) y = -y;
        y >>= 13;
        if (y > 235) y = 235;

        u = r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576;
        if (u < 0) u = -u;
        u >>= 13;
        if (u > 240) u = 240;

        v = r * 3598 + g * -3013 + b * -585 + 4096 + 1048576;
        if (v < 0) v = -v;
        v >>= 13;
        if (v > 240) v = 240;

        if (ch1) { *output = static_cast<unsigned char>(y); } output ++;
        if (ch2) { *output = static_cast<unsigned char>(u); } output ++;
        if (ch3) { *output = static_cast<unsigned char>(v); } output ++;
    }
}

void RGB24toYUV444P(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int r, g, b, y, u, v;
    unsigned char* outy = output;
    unsigned char* outu = outy + pixelcount;
    unsigned char* outv = outu + pixelcount;

    for (unsigned int i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        y = r * 2104 + g * 4130 + b * 802 + 4096 + 131072;
        if (y < 0) y = -y;
        y >>= 13;
        if (y > 235) y = 235;

        u = r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576;
        if (u < 0) u = -u;
        u >>= 13;
        if (u > 240) u = 240;

        v = r * 3598 + g * -3013 + b * -585 + 4096 + 1048576;
        if (v < 0) v = -v;
        v >>= 13;
        if (v > 240) v = 240;

        if (ch1) { *outy = static_cast<unsigned char>(y); outy ++; }
        if (ch2) { *outu = static_cast<unsigned char>(u); outu ++; }
        if (ch3) { *outv = static_cast<unsigned char>(v); outv ++; }
    }
}

void RGB24toYUV422P(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int r, g, b, y1, y2, u1, u2, v1, v2;
    const unsigned int pixelcounthalf = pixelcount >> 1;
    unsigned char* outy = output;
    unsigned char* outu = outy + pixelcount;
    unsigned char* outv = outu + pixelcounthalf;

    for (unsigned int i = 0; i < pixelcounthalf; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        y1 = r * 2104 + g * 4130 + b * 802 + 4096 + 131072;
        if (y1 < 0) y1 = -y1;
        y1 >>= 13;
        if (y1 > 235) y1 = 235;

        u1 = r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576;
        if (u1 < 0) u1 = -u1;
        u1 >>= 13;
        if (u1 > 240) u1 = 240;

        v1 = r * 3598 + g * -3013 + b * -585 + 4096 + 1048576;
        if (v1 < 0) v1 = -v1;
        v1 >>= 13;
        if (v1 > 240) v1 = 240;

        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        y2 = r * 2104 + g * 4130 + b * 802 + 4096 + 131072;
        if (y2 < 0) y2 = -y2;
        y2 >>= 13;
        if (y2 > 235) y2 = 235;

        u2 = r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576;
        if (u2 < 0) u2 = -u2;
        u2 >>= 13;
        if (u2 > 240) u2 = 240;

        v2 = r * 3598 + g * -3013 + b * -585 + 4096 + 1048576;
        if (v2 < 0) v2 = -v2;
        v2 >>= 13;
        if (v2 > 240) v2 = 240;

        if (ch1) { *outy = static_cast<unsigned char>(y1); outy ++;
                   *outy = static_cast<unsigned char>(y2); outy ++; }
        if (ch2) { *outu = static_cast<unsigned char>((u1 + u2) >> 1); outu ++; }
        if (ch3) { *outv = static_cast<unsigned char>((v1 + v2) >> 1); outv ++; }
    }
}

void RGB24toHSV24(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int max, min, delta;
    int r, g, b, h, s, v;
    unsigned int i;

    for (i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        // finding min value
        min = MIN3(r, g, b);
        max = MAX3(r, g, b);

        if (ch3) v = max;
        else v = 0;

        delta = max - min;

        if (delta != 0) {
            if (ch2) s = 255 * delta / max;
            else s = 0;

            if (ch1) {
                if (r == max) h = 42 * (g - b) / delta;             // between yellow & magenta
                else if (g == max) h = 42 * (2 + (b - r)) / delta;  // between cyan & yellow
                else h = 42 * (4 + (r - g)) / delta;                // between magenta & cyan

                if (h < 0) h += 256;
            }
            else h = 0;
        }
        else { // r = g = b = 0, thus s = 0, v is undefined
            s = 0;
            h = 0;
        }

        *output = static_cast<unsigned char>(v); output ++;
        *output = static_cast<unsigned char>(s); output ++;
        *output = static_cast<unsigned char>(h); output ++;
    }
}

void RGB24toHSV24P(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int max, min, delta;
    int r, g, b, h, s, v;
    unsigned int i;
    unsigned char* outv = output;
    unsigned char* outs = outv + pixelcount;
    unsigned char* outh = outs + pixelcount;

    for (i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        // finding min value
        min = MIN3(r, g, b);
        max = MAX3(r, g, b);

        if (ch3) v = max;
        else v = 0;

        delta = max - min;

        if (delta != 0) {
            if (ch2) s = 255 * delta / max;
            else s = 0;

            if (ch1) {
                if (r == max) h = 42 * (g - b) / delta;             // between yellow & magenta
                else if (g == max) h = 42 * (2 + (b - r)) / delta;  // between cyan & yellow
                else h = 42 * (4 + (r - g)) / delta;                // between magenta & cyan

                if (h < 0) h += 256;
            }
            else h = 0;
        }
        else { // r = g = b = 0, thus s = 0, v is undefined
            s = 0;
            h = 0;
        }

        if (ch1) { *outv = static_cast<unsigned char>(v); outv ++; }
        if (ch2) { *outs = static_cast<unsigned char>(s); outs ++; }
        if (ch3) { *outh = static_cast<unsigned char>(h); outh ++; }
    }
}

void RGB24toHSL24(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int max, min, delta;
    int r, g, b, h, s, l;
    unsigned int i;

    for (i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        // finding min value
        min = MIN3(r, g, b);
        max = MAX3(r, g, b);

        if (ch3) l = (max + min) >> 1;
        else l = 0;

        delta = max - min;

        if (delta != 0) {
            if (ch2) s = 255 * delta / max;
            else s = 0;

            if (ch1) {
                if (r == max) h = 42 * (g - b) / delta;             // between yellow & magenta
                else if (g == max) h = 42 * (2 + (b - r)) / delta;  // between cyan & yellow
                else h = 42 * (4 + (r - g)) / delta;                // between magenta & cyan

                if (h < 0) h += 256;
            }
            else h = 0;
        }
        else { // r = g = b = 0, thus s = 0, v is undefined
            s = 0;
            h = 0;
        }

        *output = static_cast<unsigned char>(l); output ++;
        *output = static_cast<unsigned char>(s); output ++;
        *output = static_cast<unsigned char>(h); output ++;
    }
}

void RGB24toHSL24P(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int max, min, delta;
    int r, g, b, h, s, l;
    unsigned int i;
    unsigned char* outl = output;
    unsigned char* outs = outl + pixelcount;
    unsigned char* outh = outs + pixelcount;

    for (i = 0; i < pixelcount; i ++) {
        r = *input; input ++;
        g = *input; input ++;
        b = *input; input ++;

        // finding min value
        min = MIN3(r, g, b);
        max = MAX3(r, g, b);

        if (ch3) l = (max + min) >> 1;
        else l = 0;

        delta = max - min;

        if (delta != 0) {
            if (ch2) s = 255 * delta / max;
            else s = 0;

            if (ch1) {
                if (r == max) h = 42 * (g - b) / delta;             // between yellow & magenta
                else if (g == max) h = 42 * (2 + (b - r)) / delta;  // between cyan & yellow
                else h = 42 * (4 + (r - g)) / delta;                // between magenta & cyan

                if (h < 0) h += 256;
            }
            else h = 0;
        }
        else { // r = g = b = 0, thus s = 0, v is undefined
            s = 0;
            h = 0;
        }

        if (ch1) { *outl = static_cast<unsigned char>(l); outl ++; }
        if (ch2) { *outs = static_cast<unsigned char>(s); outs ++; }
        if (ch3) { *outh = static_cast<unsigned char>(h); outh ++; }
    }
}

void YUV444toRGB24(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int y, u, v, r, g, b;

    for (unsigned int i = 0; i < pixelcount; i ++) {
        y = *input; input ++;
        u = *input; input ++;
        v = *input; input ++;

        y = (y - 16) * 9535;
        u -=  128;
        v -=  128;

        r = (y + v * 13074) >> 13;
        if (r < 0) r = 0;
        if (r > 255) r = 255;

        g = (y - v * 6660 - u * 3203) >> 13;
        if (g < 0) g = 0;
        if (g > 255) g = 255;

        b = (y + u * 16531) >> 13;
        if (b < 0) b = 0;
        if (b > 255) b = 255;

        if (ch1) { *output = static_cast<unsigned char>(r); } output ++;
        if (ch2) { *output = static_cast<unsigned char>(g); } output ++;
        if (ch3) { *output = static_cast<unsigned char>(b); } output ++;
    }
}

void YUV444PtoRGB24(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int y, u, v, r, g, b;
    unsigned char* iny = input;
    unsigned char* inu = iny + pixelcount;
    unsigned char* inv = inu + pixelcount;

    for (unsigned int i = 0; i < pixelcount; i ++) {
        y = *iny; iny ++;
        u = *inu; inu ++;
        v = *inv; inv ++;

        y = (y - 16) * 9535;
        u -=  128;
        v -=  128;

        r = (y + v * 13074) >> 13;
        if (r < 0) r = 0;
        if (r > 255) r = 255;

        g = (y - v * 6660 - u * 3203) >> 13;
        if (g < 0) g = 0;
        if (g > 255) g = 255;

        b = (y + u * 16531) >> 13;
        if (b < 0) b = 0;
        if (b > 255) b = 255;

        if (ch1) { *output = static_cast<unsigned char>(r); } output ++;
        if (ch2) { *output = static_cast<unsigned char>(g); } output ++;
        if (ch3) { *output = static_cast<unsigned char>(b); } output ++;
    }
}

void YUV422PtoRGB24(unsigned char* input, unsigned char* output, const unsigned int pixelcount, bool ch1, bool ch2, bool ch3)
{
    int y, u, v, r, g, b, rcr, gcr, bcr;
    const unsigned int pixelcounthalf = pixelcount >> 1;
    unsigned char* iny = input;
    unsigned char* inu = iny + pixelcount;
    unsigned char* inv = inu + pixelcounthalf;

    for (unsigned int i = 0; i < pixelcounthalf; i ++) {
        y = *iny; iny ++;
        u = *inu; inu ++;
        v = *inv; inv ++;

        y = (y - 16) * 9535;
        u -=  128;
        v -=  128;

        rcr = v * 13074;
        r = (y + rcr) >> 13;
        if (r < 0) r = 0;
        if (r > 255) r = 255;

        gcr = v * 6660 + u * 3203;
        g = (y - gcr) >> 13;
        if (g < 0) g = 0;
        if (g > 255) g = 255;

        bcr = u * 16531;
        b = (y + bcr) >> 13;
        if (b < 0) b = 0;
        if (b > 255) b = 255;

        if (ch1) { *output = static_cast<unsigned char>(r); } output ++;
        if (ch2) { *output = static_cast<unsigned char>(g); } output ++;
        if (ch3) { *output = static_cast<unsigned char>(b); } output ++;

        y = *iny; iny ++;
        y = (y - 16) * 9535;

        r = (y + rcr) >> 13;
        if (r < 0) r = 0;
        if (r > 255) r = 255;

        g = (y - gcr) >> 13;
        if (g < 0) g = 0;
        if (g > 255) g = 255;

        b = (y + bcr) >> 13;
        if (b < 0) b = 0;
        if (b > 255) b = 255;

        if (ch1) { *output = static_cast<unsigned char>(r); } output ++;
        if (ch2) { *output = static_cast<unsigned char>(g); } output ++;
        if (ch3) { *output = static_cast<unsigned char>(b); } output ++;
    }
}
