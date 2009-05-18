/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2009 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    (2009) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file LambdaTarget.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "LambdaTarget.hpp"
#include "MsqMatrix.hpp"
#include "MsqError.hpp"

namespace MESQUITE_NS {

LambdaTarget::LambdaTarget( TargetCalculator* lambda_source,
                            TargetCalculator* composite_source )
  : lambdaSource(lambda_source), compositeSource(composite_source)
  {}

LambdaTarget::~LambdaTarget()
  {}

bool LambdaTarget::get_3D_target( PatchData& pd, 
                                  size_t element,
                                  Sample sample,
                                  MsqMatrix<3,3>& W_out,
                                  MsqError& err )
{
  bool valid = lambdaSource->get_3D_target( pd, element, sample, W_out, err );
  if (MSQ_CHKERR(err) && !valid)
    return false;
  double lambda = Mesquite::cbrt( fabs( det( W_out ) ) );
  
  valid = compositeSource->get_3D_target( pd, element, sample, W_out, err );
  W_out *= lambda;
  return !MSQ_CHKERR(err) && valid;
}

bool LambdaTarget::get_2D_target( PatchData& pd, 
                                  size_t element,
                                  Sample sample,
                                  MsqMatrix<3,2>& W_out,
                                  MsqError& err )
{
  bool valid = lambdaSource->get_2D_target( pd, element, sample, W_out, err );
  if (MSQ_CHKERR(err) && !valid)
    return false;
  double d = det( transpose(W_out) * W_out );
  double lambda = sqrt( sqrt( d ) );
  
  valid = compositeSource->get_2D_target( pd, element, sample, W_out, err );
  W_out *= lambda;
  return !MSQ_CHKERR(err) && valid;
}



} // namespace MESQUITE_NS
