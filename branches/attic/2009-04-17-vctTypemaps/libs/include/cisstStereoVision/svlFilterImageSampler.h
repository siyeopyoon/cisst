/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: svlImageSampler.h 238 2009-04-10 15:52:37Z bvagvol1 $
  
  Author(s):  Balazs Vagvolgyi
  Created on: 2006 

  (C) Copyright 2006-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#ifndef _svlFilterImageSampler_h
#define _svlFilterImageSampler_h

#include <cisstStereoVision/svlStreamManager.h>

// Always include last!
#include <cisstStereoVision/svlExport.h>

class svlFilterImageSampler;

class CISST_EXPORT svlImageSamplerCallbackBase
{
friend class svlFilterImageSampler;

public:
    svlImageSamplerCallbackBase() { FrameCounter = 0; }
    virtual ~svlImageSamplerCallbackBase() {}

    // Callback
    virtual void FrameCallback(svlSampleImageBase * image,
                               svlBMPFileHeader * fileheader1, svlDIBHeader * dibheader1,
                               svlBMPFileHeader * fileheader2, svlDIBHeader * dibheader2) = 0;

    inline unsigned int GetFrameCounter(void) { return FrameCounter; }
    inline void ResetCounter(void) { FrameCounter = 0; }

private:
    unsigned int FrameCounter;

    virtual void PostCallback() { FrameCounter ++; }
};

class CISST_EXPORT svlFilterImageSampler : public svlFilterBase
{
public:
    svlFilterImageSampler();
    virtual ~svlFilterImageSampler();

    void SetCallback(svlImageSamplerCallbackBase* callbackobj);
    void SetDistanceIntensityRatio(float ratio) { DistanceScaling = ratio; }
    float GetDistanceIntensityRatio() { return DistanceScaling; }

protected:
    virtual int Initialize(svlSample* inputdata);
    virtual int ProcessFrame(ProcInfo* procInfo, svlSample* inputdata);
    virtual int Release();

private:
    svlImageSamplerCallbackBase* CallbackObj;
    svlBMPFileHeader* FileHeader[2];
    svlDIBHeader* DIBHeader[2];
    svlSampleImageBase* ImageBuffer;
    float DistanceScaling;
};

#endif // _svlFilterImageSampler_h
