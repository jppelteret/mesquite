/*!
  \file   LPtoPTemplate.cpp
  \brief  

  This Objective Function is evaluated using an L P norm to the pth power.
  total=(sum (x_i)^pVal)
  \author Michael Brewer
  \author Thomas Leurent
  \date   2002-01-23
*/
#include <math.h>
#include "LPtoPTemplate.hpp"
#include "MsqFreeVertexIndexIterator.hpp"
#include "MsqMessage.hpp"
#include "MsqTimer.hpp"

using  namespace Mesquite;  

using std::cout;
using std::endl;

#undef __FUNC__
#define __FUNC__ "LPtoPTemplate::LPtoPTemplate"

LPtoPTemplate::LPtoPTemplate(QualityMetric *qualitymetric, short Pinput, MsqError &err){
  set_quality_metric(qualitymetric);
  pVal=Pinput;
  if(pVal<1){
    err.set_msg("P_VALUE must be greater than 0.");
  }
  set_gradient_type(ObjectiveFunction::ANALYTICAL_GRADIENT);
    //set_use_local_gradient(true);
  set_negate_flag(qualitymetric->get_negate_flag());
}

#undef __FUNC__
#define __FUNC__ "LPtoPTemplate::~LPtoPTemplate"

//Michael:  need to clean up here
LPtoPTemplate::~LPtoPTemplate(){

}

#undef __FUNC__
#define __FUNC__ "LPtoPTemplate::concrete_evaluate"
bool LPtoPTemplate::concrete_evaluate(PatchData &pd, double &fval,
                                      MsqError &err){
  size_t index=0;
  MsqMeshEntity* elems=pd.get_element_array(err);
  bool obj_bool=true;
    //double check for pVal=0;
  if(pVal==0){
    err.set_msg("pVal equal zero not allowed.  L_0 is not a valid norm.");
    return 0;
  }
  
    //Michael:  this may not do what we want
    //Set currentQM to be the first quality metric* in the list
  QualityMetric* currentQM = get_quality_metric();
  if(currentQM==NULL)
    currentQM=get_quality_metric_list().front();
  if(currentQM==NULL)
    err.set_msg("NULL QualityMetric pointer in LPtoPTemplate");
  size_t num_elements=pd.num_elements();
  size_t num_vertices=pd.num_vertices();
  size_t total_num=0;
  if(currentQM->get_metric_type()==QualityMetric::ELEMENT_BASED)   
    total_num=num_elements;
  else if (currentQM->get_metric_type()==QualityMetric::VERTEX_BASED)
    total_num=num_vertices;
  else
    err.set_msg("Make sure MetricType is initialised in concrete QualityMetric constructor.");
  double *metric_values= new double[total_num];
  if(currentQM->get_metric_type()==QualityMetric::ELEMENT_BASED)
  {
    for (index=0; index<num_elements;index++)
    {
        //if invalid return false after clean-up
      obj_bool=currentQM->evaluate_element(pd, (&elems[index]),
                                           metric_values[index], err);
      MSQ_CHKERR(err);
      if(!obj_bool){
        fval=0.0;
        delete[] metric_values;
        return false;
      }
      
      metric_values[index]=fabs(metric_values[index]);
      MSQ_DEBUG_ACTION(3,{std::cout<< "      o  Quality metric value for element "
                          << index << "\t: " << metric_values[index] << "\n";});
    }
  }
  else if(currentQM->get_metric_type()==QualityMetric::VERTEX_BASED)
  {
    MsqVertex* vertices=pd.get_vertex_array(err);
    for (index=0; index<num_vertices;index++)
    {
        //evaluate metric for this vertex
      obj_bool=currentQM->evaluate_vertex(pd, (&vertices[index]),
                                          metric_values[index], err);
      MSQ_CHKERR(err);
        //if invalid return false after clean-up
      if(!obj_bool){
        fval=0.0;
        delete[] metric_values;
        return false;
      }
      
      metric_values[index]=fabs(metric_values[index]);
    }
  }
  fval=compute_function(metric_values, total_num, err);
  delete[] metric_values;
  return true;
}

#undef __FUNC__
#define __FUNC__ "LPtoPTemplate::compute_analytical_gradient"
/* virtual function reimplemented from QualityMetric. No doxygen doc needed. */
bool LPtoPTemplate::compute_analytical_gradient(PatchData &pd,
                                              Vector3D *const &grad,
                                              double &OF_val,
                                              MsqError &err, size_t array_size)
{
  FUNCTION_TIMER_START(__FUNC__);
  
  size_t num_elements=pd.num_elements();
  size_t num_vertices=pd.num_vertices();
  if( num_vertices!=array_size && array_size>0)
    err.set_msg("Incorrect array size.");
  MsqMeshEntity* elems=pd.get_element_array(err);
  MsqVertex* vertices=pd.get_vertex_array(err);
  bool qm_bool=true;
  double QM_val;
  OF_val = 0.;
  size_t i, p;
  
  // If MSQ_DBG1 is defined, check to make sure that num_vert == array_size.
  MSQ_DEBUG_ACTION(1,{
    size_t num_vert=pd.num_vertices(); 
    if(num_vert!=array_size && array_size!=0){
      err.set_msg("Analytical Gradient passed arrays of incorrect size.");
      MSQ_CHKERR(err); cout << num_vert << " instead of " << array_size << endl; }
  });
  
  //Set currentQM to be quality metric (possibly composite) associated with the objective function
  QualityMetric* currentQM = get_quality_metric();
  if(currentQM==NULL)
    err.set_msg("LPtoPTemplate has NULL QualityMetric pointer.");
  enum QualityMetric::MetricType qm_type=currentQM->get_metric_type();
  
  if (!qm_type==QualityMetric::ELEMENT_BASED &&
      !qm_type==QualityMetric::VERTEX_BASED)
    err.set_msg("Make sure MetricType is initialised"
                "in concrete QualityMetric constructor.");


  // zeros out objective function gradient
  for (i=0; i<num_vertices; ++i)
    grad[i] =0;
  
  // Computes objective function gradient for an element based metric
  if(qm_type==QualityMetric::ELEMENT_BASED){

    size_t e, ve, fve;
    size_t nfve; // num free vtx in element
    size_t nve; // num vtx in element
    MsqVertex* ele_free_vtces[MSQ_MAX_NUM_VERT_PER_ENT];
    const size_t *ele_vtces_ind;

    // loops over all elements.
    for (e=0; e<num_elements; ++e) {
      // stores the pointers to the free vertices within the element
      // (using pointer arithmetic).
      nfve = 0;
      nve = elems[e].vertex_count();
      ele_vtces_ind = elems[e].get_vertex_index_array();
      for (ve=0; ve<nve; ++ve) {
        if (vertices[ele_vtces_ind[ve]].is_free_vertex()) {
          ele_free_vtces[nfve] = vertices + ele_vtces_ind[ve];
          ++nfve;
        }
      }

      // Computes q and grad(q)
      Vector3D grad_vec[MSQ_MAX_NUM_VERT_PER_ENT];
      qm_bool = currentQM->compute_element_gradient(
                                     pd, &elems[e],
                                     ele_free_vtces,
                                     grad_vec, nfve, QM_val, err);
      if(!qm_bool) return false;

      // computes p*|Q(e)|^{p-1}
      QM_val = fabs(QM_val);
      double QM_pow=1.0;
      double factor;
      if (pVal==1) factor=1;
      else {
        QM_pow=QM_val;
        for (p=1; p<pVal-1; ++p)
          QM_pow*=QM_val;
        factor = QM_pow * pVal;
      }
      factor *= get_negate_flag();

      // computes Objective Function value \sum_{i=1}^{N_e} |q_i|^P
      OF_val += QM_pow * QM_val;
      
      // For each free vertex in the element ... 
      for (i=0; i<nfve; ++i) {
        // ... computes p*q^{p-1}*grad(q) ...
        grad_vec[i] *= factor;
        // ... and accumulates it in the objective function gradient.
        grad[pd.get_vertex_index(ele_free_vtces[i])] += grad_vec[i];
      }
    }
  }
  
  // Computes objective function gradient for a vertex based metric  
  else if (qm_type==QualityMetric::VERTEX_BASED){

    //vector for storing indices of vertex's connected elems
    std::vector<size_t> vert_on_vert_ind;
    //position in pd's vertex array
    size_t vert_count=0;
    //position in vertex array
    size_t vert_pos=0;
    //loop over the free vertex indices to find the gradient...
    int vfv_array_length=10;//holds the current legth of vert_free_vtces
    MsqVertex** vert_free_vtces = new MsqVertex*[vfv_array_length];
    Vector3D* grad_vec = new Vector3D[vfv_array_length];
    for(vert_count=0; vert_count<num_vertices; ++vert_count){
      //For now we compute the metric for attached vertices and this
      //vertex, the above line gives us the attached vertices.  Now,
      //we must add this vertex.
      pd.get_adjacent_vertex_indices(vert_count,
                                     vert_on_vert_ind,err);
      vert_on_vert_ind.push_back(vert_count);
      size_t vert_num_vtces = vert_on_vert_ind.size();
      
      // dynamic memory management if arrays are too small.
      if(vert_num_vtces > vfv_array_length){
        vfv_array_length=vert_num_vtces+5;
        delete [] vert_free_vtces;
        MsqVertex** vert_free_vtces = new MsqVertex*[vfv_array_length];
        delete [] grad_vec;
        Vector3D* grad_vec = new Vector3D[vfv_array_length];
      }
      
      size_t vert_num_free_vtces=0;
      //loop over the vertices connected to this one (vert_count)
      //and put the free ones into vert_free_vtces
      while(!vert_on_vert_ind.empty()){
        vert_pos=(vert_on_vert_ind.back());
        //clear the vector as we go
        vert_on_vert_ind.pop_back();
        //if the vertex is free, add it to ver_free_vtces
        if(vertices[vert_pos].is_free_vertex()){
          vert_free_vtces[vert_num_free_vtces]=&vertices[vert_pos];
          ++vert_num_free_vtces ;
        }
      }
      
      qm_bool=currentQM->compute_vertex_gradient(pd,
                                                 vertices[vert_count],
                                                 vert_free_vtces,
                                                 grad_vec,
                                                 vert_num_free_vtces,
                                                 QM_val, err);
      if(!qm_bool){
        delete[] vert_free_vtces;
        delete[] grad_vec;
        return false;
      }
       // computes p*|Q(e)|^{p-1}
      QM_val = fabs(QM_val);
      double QM_pow, factor;
      if (pVal==1) factor=1;
      else {
        QM_pow=QM_val;
        for (p=1; p<pVal-1; ++p)
          QM_pow*=QM_val;
        factor = QM_pow * pVal;
      }
      factor *= get_negate_flag();

      // computes Objective Function value \sum_{i=1}^{N_v} |q_i|^P
      OF_val += QM_pow * QM_val;
      
      // For each free vertex around the vertex (and the vertex itself if free) ... 
      for (i=0; i < vert_num_free_vtces ; ++i) {
        // ... computes p*q^{p-1}*grad(q) ...
        grad_vec[i] *= factor;
        // ... and accumulates it in the objective function gradient.
        grad[pd.get_vertex_index(vert_free_vtces[i])] += grad_vec[i];
      }
    }
    delete [] vert_free_vtces;
    delete [] grad_vec;
  }

  OF_val *= get_negate_flag();
  
  
  FUNCTION_TIMER_END();
  return true;  

}
  
	
#undef __FUNC__
#define __FUNC__ "LPtoPTemplate::compute_analytical_hessian"
/*! \fn LPtoPTemplate::compute_analytical_hessian(PatchData &pd, MsqHessian &hessian, MsqError &err)

    For each element, each entry to be accumulated in the Hessian for
    this objective function (\f$ \sum_{e \in E} Q(e)^p \f$ where \f$ E \f$
    is the set of all elements in the patch) has the form:
    \f$ pQ(e)^{p-1} \nabla^2 Q(e) + p(p-1)Q(e)^{p-2} \nabla Q(e) [\nabla Q(e)]^T \f$.

    For \f$ p=2 \f$, this simplifies to
    \f$ 2Q(e) \nabla^2 Q(e) + 2 \nabla Q(e) [\nabla Q(e)]^T \f$.

    For \f$ p=1 \f$, this simplifies to \f$ \nabla^2 Q(e) \f$.

    The \f$ p=1 \f$ simplified version is implemented directly
    to speed up computation. 
    
    \param pd The PatchData object for which the objective function
           hessian is computed.
    \param hessian: this object must have been previously initialized.
*/
bool LPtoPTemplate::compute_analytical_hessian(PatchData &pd,
                                               MsqHessian &hessian,
                                               Vector3D *const &grad,
                                               double &OF_val,
                                               MsqError &err)
{
  FUNCTION_TIMER_START(__FUNC__);

  MsqMeshEntity* elements = pd.get_element_array(err); MSQ_CHKERR(err);
  MsqVertex* vertices = pd.get_vertex_array(err); MSQ_CHKERR(err);
  size_t num_elems = pd.num_elements();
  size_t num_vertices = pd.num_vertices();
  Matrix3D elem_hessian[MSQ_MAX_NUM_VERT_PER_ENT*(MSQ_MAX_NUM_VERT_PER_ENT+1)/2];
  Matrix3D elem_outer_product[MSQ_MAX_NUM_VERT_PER_ENT*(MSQ_MAX_NUM_VERT_PER_ENT+1)/2];
  Vector3D grad_vec[MSQ_MAX_NUM_VERT_PER_ENT];
  double QM_val;
  double fac1, fac2;
  Matrix3D grad_outprod;
  bool qm_bool;
//  Vector3D zero3D(0,0,0);
  QualityMetric* currentQM = get_quality_metric();
  
  MsqVertex* ele_free_vtces[MSQ_MAX_NUM_VERT_PER_ENT];
  const size_t* vtx_indices;
    
  size_t e, v;
  size_t nfve; // number of free vertices in element
  short i,j,n;

  hessian.zero_out();
  for (v=0; v<num_vertices; ++v) grad[v] = 0.;
  OF_val = 0.;
  
  for (e=0; e<num_elems; ++e) {
    short nve = elements[e].vertex_count();
    
    // Gets a list of free vertices in the element.
    vtx_indices = elements[e].get_vertex_index_array();
    nfve=0;
    for (i=0; i<nve; ++i) {
      if ( vertices[vtx_indices[i]].is_free_vertex() ) {
        ele_free_vtces[nfve] = vertices + vtx_indices[i];
        ++nfve;
      }
    }
    
    // Computes \nabla^2 Q(e). Only the free vertices will have non-zero entries. 
    qm_bool = currentQM->compute_element_hessian(pd,
                                    elements+e, ele_free_vtces,
                                    grad_vec, elem_hessian,
                                    nfve, QM_val, err);
    MSQ_CHKERR(err);
    if (!qm_bool) return false;


    // **** Computes Hessian ****
    double QM_pow=1.;
    if (pVal == 1) {
      hessian.accumulate_entries(pd, e, elem_hessian, err);
      fac1 = 1;
    }
    else if (pVal >= 2) {
      // Computes \nabla Q(e) [\nabla Q(e)]^T 
      n=0;
      for (i=0; i<nve; ++i) {
        for (j=i; j<nve; ++j) {
          if ( vertices[vtx_indices[i]].is_free_vertex() &&
               vertices[vtx_indices[j]].is_free_vertex() ) {
            elem_outer_product[n].outer_product(grad_vec[i], grad_vec[j]);
          } else {
            elem_outer_product[n] = 0.;
          }
          ++n;
        }
      }

      QM_val = fabs(QM_val);
      QM_pow = 1;
      for (i=0; i<pVal-2; ++i)
        QM_pow *= QM_val;
      // Computes p(p-1)Q(e)^{p-2}
      fac2 = pVal* (pVal-1) * QM_pow;
      // Computes  pQ(e)^{p-1}
      QM_pow *= QM_val;
      fac1 = pVal * QM_pow;

      fac1 *= get_negate_flag();
      fac2 *= get_negate_flag();

      for (i=0; i<nve*(nve+1)/2; ++i) {
        elem_hessian[i] *= fac1;
        elem_outer_product[i] *= fac2;
      }
      
      hessian.accumulate_entries(pd, e, elem_hessian, err);
      hessian.accumulate_entries(pd, e, elem_outer_product, err);

    } else {
      err.set_msg(" invalid P value.");
      return false;
    }


    // **** Computes Gradient ****

    // For each vertex in the element ... 
    for (i=0; i<nve; ++i) {
      // ... computes p*q^{p-1}*grad(q) ...
      grad_vec[i] *= fac1;
      // ... and accumulates it in the objective function gradient.
      grad[vtx_indices[i]] += grad_vec[i];
    }
    
    // **** computes Objective Function value \sum_{i=1}^{N_e} |q_i|^P ****
    OF_val += QM_pow * QM_val;
    
  }

  OF_val *= get_negate_flag();
  
  FUNCTION_TIMER_END();
  return true;
}
