/*

  Author(s): Simon Leonard
  Created on: Dec 17 2009

  (C) Copyright 2008 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstRobot/robLink.h>

robLink::robLink() : kinematics( NULL ){}

robLink::robLink( const robLink& link ){
  kinematics = link.kinematics->Clone();
  mass       = link.mass;
}

robLink::robLink( robKinematics* kinematics, const robMass& mass ) : 
  kinematics( kinematics ), 
  mass( mass ) {}

robLink::~robLink(){
  if( kinematics != NULL ){ 
    delete kinematics;
    kinematics = NULL;
  }
}

robKinematics* robLink::GetKinematics() const
{ return kinematics; }
robMass        robLink::GetMass() const
{ return mass; }

robLink::Errno robLink::Read( std::istream& is ){ 
  if( kinematics != NULL ) { kinematics->Read( is ); }
  mass.ReadMass( is );
  return robLink::ESUCCESS;
}

robLink::Errno robLink::Write( std::ostream& os ) const { 
  if( kinematics != NULL ) { kinematics->Write( os ); }
  mass.WriteMass( os );
  return robLink::ESUCCESS;
}

vctFrame4x4<double> robLink::ForwardKinematics( double q ) const {
  if( kinematics != NULL ) { return kinematics->ForwardKinematics( q ); }
  else                     { return vctFrame4x4<double>(); }
}

vctMatrixRotation3<double> robLink::Orientation( double q ) const {
  if( kinematics != NULL ) { return kinematics->Orientation( q ); }
  else                     { return vctMatrixRotation3<double>(); }
}

vctFixedSizeVector<double,3> robLink::PStar() const {
  if( kinematics != NULL ) { return kinematics->PStar(); }
  else                     { return vctFixedSizeVector<double,3>(0.0);  }
}

robKinematics::Convention robLink::GetConvention() const {
  if( kinematics != NULL ) { return kinematics->GetConvention(); }
  else                     { return robKinematics::UNDEFINED;  }
}


robJoint::Type robLink::GetType() const {
  if( kinematics != NULL ) { return kinematics->GetType(); }
  else                     { return robJoint::UNDEFINED;  }
}

