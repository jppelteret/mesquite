/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Sandia National Laboratories.  Developed at the
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 
    (2010) kraftche@cae.wisc.edu
   
  ***************************************************************** */


/** \file AWSizeNB1.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "Mesquite_AWSizeNB1.hpp"
#include "Mesquite_MsqMatrix.hpp"
#include "Mesquite_TMPDerivs.hpp"
#include "Mesquite_TMPCommon.hpp"

namespace MESQUITE_NS {

std::string AWSizeNB1::get_name() const
  { return "AWSizeNB1"; }

AWSizeNB1::~AWSizeNB1() {}

template <unsigned DIM> static inline
bool eval( const MsqMatrix<DIM,DIM>& A, 
           const MsqMatrix<DIM,DIM>& W, 
           double& result)
{
  result = det(A) - det(W);
  result *= result;
  return true;
}

template <unsigned DIM> static inline
bool grad( const MsqMatrix<DIM,DIM>& A, 
           const MsqMatrix<DIM,DIM>& W, 
           double& result, 
           MsqMatrix<DIM,DIM>& deriv )
{
  result = det(A) - det(W);
  deriv = transpose_adj(A);
  deriv *= 2*result;
  result *= result;
  return true;
}

template <unsigned DIM> static inline
bool hess( const MsqMatrix<DIM,DIM>& A, 
           const MsqMatrix<DIM,DIM>& W, 
           double& result, 
           MsqMatrix<DIM,DIM>& deriv, 
           MsqMatrix<DIM,DIM>* second )
{
  result = det(A) - det(W);
  deriv = transpose_adj(A);
  set_scaled_outer_product( second, 2.0, deriv );
  pluseq_scaled_2nd_deriv_of_det( second, 2*result, A );
  deriv *= 2*result;
  result *= result;
  return true;
}

TMP_AW_TEMPL_IMPL_COMMON(AWSizeNB1)

} // namespace Mesquite
