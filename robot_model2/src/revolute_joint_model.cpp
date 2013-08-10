/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan */

#include <moveit/robot_model/revolute_joint_model.h>
#include <boost/math/constants/constants.hpp>
#include <algorithm>
#include <limits>
#include <cmath>

moveit::core::RevoluteJointModel::RevoluteJointModel(const std::string& name) 
  : JointModel(name)
  , axis_(0.0, 0.0, 0.0)
  , continuous_(false)
{
  type_ = REVOLUTE;
  variable_names_.push_back(name_);
  variable_bounds_.resize(1);
  variable_bounds_[0].position_bounded_ = true;
  variable_bounds_[0].min_position_ = -boost::math::constants::pi<double>();
  variable_bounds_[0].max_position_ = boost::math::constants::pi<double>();
  variable_index_map_[name_] = 0;
  computeVariableBoundsMsg();
}

unsigned int moveit::core::RevoluteJointModel::getStateSpaceDimension() const
{
  return 1;
}

void moveit::core::RevoluteJointModel::setContinuous(bool flag)
{
  if (flag)
  {
    variable_bounds_[0].position_bounded_ = false;
    variable_bounds_[0].min_position_ = -boost::math::constants::pi<double>();
    variable_bounds_[0].max_position_ = boost::math::constants::pi<double>();
  }
  else
    variable_bounds_[0].position_bounded_ = true;
  computeVariableBoundsMsg();
}

double moveit::core::RevoluteJointModel::getMaximumExtent(const Bounds &other_bounds) const
{
  return variable_bounds_[0].max_position_ - variable_bounds_[0].min_position_;
}

void moveit::core::RevoluteJointModel::getVariableDefaultValues(double *values, const Bounds &bounds) const
{
  // if zero is a valid value
  if (bounds[0].min_position_ <= 0.0 && bounds[0].max_position_ >= 0.0)
    values[0] = 0.0;
  else
    values[0] = (bounds[0].min_position_ + bounds[0].max_position_) / 2.0;
}

void moveit::core::RevoluteJointModel::getVariableRandomValues(random_numbers::RandomNumberGenerator &rng, double *values, const Bounds &bounds) const
{
  values[0] = rng.uniformReal(bounds[0].min_position_, bounds[0].max_position_);
}

void moveit::core::RevoluteJointModel::getVariableRandomValuesNearBy(random_numbers::RandomNumberGenerator &rng, double *values, const Bounds &bounds,
                                                                     const double *near, const double distance) const
{
  if (continuous_)
  {
    values[0] = rng.uniformReal(near[0] - distance, near[0] + distance);
    enforceBounds(values, bounds);
  }
  else
    values[0] = rng.uniformReal(std::max(bounds[0].min_position_, near[0] - distance),
                                std::min(bounds[0].max_position_, near[0] + distance));
}

void moveit::core::RevoluteJointModel::interpolate(const double *from, const double *to, const double t, double *state) const
{
  if (continuous_)
  {
    double diff = to[0] - from[0];
    if (fabs(diff) <= boost::math::constants::pi<double>())
      state[0] = from[0] + diff * t;
    else
    {
      if (diff > 0.0)
        diff = 2.0 * boost::math::constants::pi<double>() - diff;
      else
        diff = -2.0 * boost::math::constants::pi<double>() - diff;
      state[0] = from[0] - diff * t;
      // input states are within bounds, so the following check is sufficient
      if (state[0] > boost::math::constants::pi<double>())
        state[0] -= 2.0 * boost::math::constants::pi<double>();
      else
        if (state[0] < -boost::math::constants::pi<double>())
          state[0] += 2.0 * boost::math::constants::pi<double>();
    }
  }
  else
    state[0] = from[0] + (to[0] - from[0]) * t;
}

double moveit::core::RevoluteJointModel::distance(const double *values1, const double *values2) const
{
  if (continuous_)
  {
    double d = fabs(values1[0] - values2[0]);
    return (d > boost::math::constants::pi<double>()) ? 2.0 * boost::math::constants::pi<double>() - d : d;
  }
  else
    return fabs(values1[0] - values2[0]);
}

bool moveit::core::RevoluteJointModel::satisfiesBounds(const double *values, const Bounds &bounds, double margin) const
{
  if (continuous_)
    return true;
  if (values[0] < bounds[0].min_position_ - margin || values[0] > bounds[0].max_position_ + margin)
    return false;
  return true;
}

void moveit::core::RevoluteJointModel::enforceBounds(double *values, const Bounds &bounds) const
{
  if (continuous_)
  {
    double &v = values[0];
    v = fmod(v, 2.0 * boost::math::constants::pi<double>());
    if (v < -boost::math::constants::pi<double>())
      v += 2.0 * boost::math::constants::pi<double>();
    else
      if (v > boost::math::constants::pi<double>())
        v -= 2.0 * boost::math::constants::pi<double>();
  }
  else
  {
    if (values[0] < bounds[0].min_position_)
      values[0] = bounds[0].min_position_;
    else
      if (values[0] > bounds[0].max_position_)
        values[0] = bounds[0].max_position_;
  }
}

void moveit::core::RevoluteJointModel::computeTransform(const double *joint_values, Eigen::Affine3d &transf) const
{
  transf = Eigen::Affine3d(Eigen::AngleAxisd(joint_values[0], axis_));
}

void moveit::core::RevoluteJointModel::computeVariableValues(const Eigen::Affine3d& transf, double *joint_values) const
{
  Eigen::Quaterniond q(transf.rotation());
  q.normalize();
  joint_values[0] = acos(q.w())*2.0f;
}