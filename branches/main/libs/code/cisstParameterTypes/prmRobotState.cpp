/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
Author(s):	Marcin Balicki
Created on:   2008-09-14

(C) Copyright 2008 Johns Hopkins University (JHU), All Rights
Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/
#include <cisstParameterTypes/prmRobotState.h>

CMN_IMPLEMENT_SERVICES(prmRobotState);

prmRobotState::prmRobotState()
{
    CartesianPosition().SetSize(6);
    CartesianVelocity().SetSize(6);
    CartesianPositionGoal().SetSize(6);
    CartesianVelocityGoal().SetSize(6);
    CartesianVelocityError().SetSize(6);
    CartesianPositionError().SetSize(6);

}
prmRobotState::prmRobotState(size_type size)
{
    SetSize(size);
}

void prmRobotState::ToStream(std::ostream & outputStream) const
{

    outputStream << "JointPosition: "               << this->JointPositionMember
                 << "\nJointVelocity : "		    << this->JointVelocityMember
                 << "\nJointPositionGoal: "		    << this->JointPositionGoalMember
                 << "\nJointVelocityGoal: "         << this->JointVelocityGoalMember
                 << "\nJointPositionError: "	    << this->JointPositionErrorMember
                 << "\nJointVelocityError: "	    << this->JointVelocityErrorMember
                 << "\nCartesianPosition: "         << this->CartesianPositionMember
                 << "\nCartesianVelocity : "		<< this->CartesianVelocityMember
                 << "\nCartesianPositionGoal: "		<< this->CartesianPositionGoalMember
                 << "\nCartesianVelocityGoal: "     << this->CartesianVelocityGoalMember
                 << "\nCartesianVelocityError: "	<< this->CartesianVelocityErrorMember
                 << "\nCartesianPositionError: "	<< this->CartesianPositionErrorMember
                 << "\nEndEffectorFrame: "          << this->EndEffectorFrameMember;
}

void prmRobotState::SetSize(size_type size){

    JointPositionMember.SetSize(size);
    JointVelocityMember.SetSize(size);
    JointPositionGoalMember.SetSize(size);
    JointVelocityGoalMember.SetSize(size);
    JointPositionErrorMember.SetSize(size);
    JointVelocityErrorMember.SetSize(size);
    /* these are 6 by nature.   */
    CartesianPositionMember.SetSize(6);
    CartesianVelocityMember.SetSize(6);
    CartesianPositionGoalMember.SetSize(6);
    CartesianVelocityGoalMember.SetSize(6);
    CartesianVelocityErrorMember.SetSize(6);
    CartesianPositionErrorMember.SetSize(6);

}

//  $Log 
