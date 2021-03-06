/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  
  Author(s):  Marcin Balicki
  Created on: 2011-02-18

  (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#ifndef _svlFilterImageQWidget_h
#define _svlFilterImageQWidget_h

#include <QGLWidget>
#include <cisstStereoVision/svlFilterBase.h>

// Always include last!
#include <cisstStereoVision/svlExportQt.h>

//! this class is useful for embedding an svl stream inside a qt widget
class CISST_EXPORT svlFilterImageQtWidget: public QGLWidget, public svlFilterBase
{
    Q_OBJECT

    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_ALLOW_DEFAULT);

 public:
    svlFilterImageQtWidget(QWidget *parent = 0);
    ~svlFilterImageQtWidget();

 protected:
    // Qt derived methods
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

    void CheckGLError();

 private:
    QPoint LastPosition;
    svlSampleImage * Image;

 protected:
    // svl derived methods
    int Initialize(svlSample * syncInput, svlSample* &syncOutput);
    int Process(svlProcInfo * procInfo, svlSample * syncInput, svlSample* &syncOutput);
    int Release();

 signals:
    void QSignalUpdateGL();

};

CMN_DECLARE_SERVICES_INSTANTIATION_EXPORT(svlFilterImageQtWidget)

#endif // _svlFilterImageQWidget_h
