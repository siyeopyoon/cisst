#include <stdlib.h>

#include <cisstRobot/robDH.h>

#include "robDHTest.h"
#include "robWAMKinematics.h"

void robDHTest::TestRandomFK( size_t i, const robDH& dh ){

  for( size_t j=0; j<10; j++ ){
    double q = ((double)rand()) / ((double)RAND_MAX);
    q = q * cmnPI * 2.0 - cmnPI;
    vctFrame4x4<double> Rt = dh.ForwardKinematics( q );
    CPPUNIT_ASSERT( ( Rt.AlmostEqual( WAM7FKine( i, q ) ) ) );
  }
  
}

void robDHTest::TestForwardKinematics(){

  std::ifstream ifs;
  std::string fname( CISST_SOURCE_ROOT"/libs/etc/cisstRobot/WAM/wam7.rob" );

  ifs.open( fname.data() );
  if( !ifs ){ 
    std::cout << "Failed to open the file: " << fname << std::endl;
  }

  {
    size_t N;       // the number of links
    std::string line;
    getline( ifs, line );
    std::istringstream stringstream(line);
    stringstream >> N;
    CPPUNIT_ASSERT( N == 7 );
  }


  for( size_t i=0; i<7; i++ ){

    // Read a line from the file
    std::string line;
    getline( ifs, line );
    std::istringstream stringstream(line);
    
    // Find the type of kinematics convention
    std::string convention;
    stringstream >> convention;

    // instantiate a standard DH
    robKinematics* kinematics = NULL;
    kinematics = robKinematics::Instantiate( convention );
    CPPUNIT_ASSERT( kinematics );
    
    // cast to DH
    robDH* dh = dynamic_cast<robDH*>( kinematics );
    CPPUNIT_ASSERT( dh );
    
    // Read
    dh->Read( stringstream );

    // test the fkin
    TestRandomFK( i, *dh );

    delete dh;
  }

  ifs.close();

}

void robDHTest::TestRead(){

  std::ifstream ifs;
  std::string fname( CISST_SOURCE_ROOT"/libs/etc/cisstRobot/WAM/wam7.rob" );

  ifs.open( fname.data() );
  if( !ifs ){ 
    std::cout << "Failed to open the file: " << fname << std::endl;
  }

  {
    size_t N;       // the number of links
    std::string line;
    getline( ifs, line );
    std::istringstream stringstream(line);
    stringstream >> N;
    CPPUNIT_ASSERT( N == 7 );
  }

  for( size_t i=0; i<7; i++ ){

    // Read a line from the file
    std::string line;
    getline( ifs, line );
    std::istringstream stringstream(line);
    
    // Find the type of kinematics convention
    std::string convention;
    stringstream >> convention;

    // Instantiate a DH
    robKinematics* kinematics = NULL;
    kinematics = robKinematics::Instantiate( convention );
    CPPUNIT_ASSERT( kinematics );
    
    // Convert to DH
    robDH* dh = dynamic_cast<robDH*>( kinematics );
    CPPUNIT_ASSERT( dh );
    
    // Read
    dh->Read( stringstream );

    // Check
    CPPUNIT_ASSERT( dh->GetConvention() == robKinematics::STANDARD_DH  );
    CPPUNIT_ASSERT( dh->GetType()       == robJoint::HINGE  );
    CPPUNIT_ASSERT( dh->GetMode()       == robJoint::ACTIVE );

    delete dh;
  }

  ifs.close();

}

CPPUNIT_TEST_SUITE_REGISTRATION( robDHTest );
