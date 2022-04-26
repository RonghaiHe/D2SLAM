#pragma once
#include <ceres/ceres.h>
#include <Eigen/Eigen>
#include <d2vins/d2vins_types.hpp>

//This is devied from VINS-Mono
namespace D2VINS {
struct ParamInfo;
class PriorFactor : public ceres::CostFunction
{
    std::vector<ParamInfo> keep_params_list; 
    int keep_param_size = 0;
    Eigen::MatrixXd linearized_jac;
    Eigen::VectorXd linearized_res;
  public:
    PriorFactor(std::vector<ParamInfo> _keep_params_list, const SparseMat & A, const VectorXd & b);
    virtual bool Evaluate(double const *const *parameters, double *residuals, double **jacobians) const;
    virtual std::vector<state_type*> getKeepParamsPointers() const;
    
};
}