//
//  Deformabel Simplicial Complex (DSC) method
//  Copyright (C) 2013  Technical University of Denmark
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  See licence.txt for a copy of the GNU General Public License.

#ifndef DSC_GLM_types_h
#define DSC_GLM_types_h

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class GLMTypes
{
public:
    
    typedef float            real_type;
    
    typedef glm::vec3       vector3_type;
    typedef glm::vec4       vector4_type;
    typedef glm::mat4     matrix4x4_type;
    typedef glm::mat3     matrix3x3_type;
public:
    
    static real_type dot(vector3_type const & v1, vector3_type const & v2)
    {
        return glm::dot(v1, v2);
    }
    
    static vector3_type cross(vector3_type const & v1, vector3_type const & v2)
    {
        return glm::cross(v1, v2);
    }
    
    static real_type length(vector3_type const & v)
    {
        return glm::length(v);
    }
    
    static bool is_nan(real_type const & t)
    {
        return glm::isnan(t);
    }
    
    static real_type determinant(matrix3x3_type const & m)
    {
        return glm::determinant(m);
    }
    
    static real_type determinant(matrix4x4_type const & m)
    {
        return glm::determinant(m);
    }
    
    
    static matrix4x4_type transpose(matrix4x4_type const & m)
    {
        return glm::transpose(m);
    }
    
//    static matrix4x4_type invert(matrix4x4_type const & m)
//    {
//        return glm::invert(m);
//    }
//    
//    
    static real_type sqr_length(vector3_type const & v)
    {
        return glm::dot(v, v);
    }
    
    static vector3_type normalize(vector3_type const & v)
    {
        return glm::normalize(v);
    }
    
    static matrix3x3_type get_rotation_matrix(vector3_type const & axis, real_type const & angle)
    {
        glm::mat4 m(1.0f);
        matrix4x4_type r = glm::rotate(m, (float)angle, axis);
        matrix3x3_type r3;
        return r3;
    }
    
    static vector3_type get_x_axis()
    {
        return vector3_type(1.,0.,0.);
    }
    
    static vector3_type get_y_axis()
    {
        return vector3_type(0.,1.,0.);
    }
    
    static vector3_type get_z_axis()
    {
        return vector3_type(0.,0.,1.);
    }
    
};

#endif
