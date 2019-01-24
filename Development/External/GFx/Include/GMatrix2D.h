/*****************************************************************

Filename    :   GMatrix2D.h
Content     :   2D Matrix class 
Created     :   May 29, 2006
Authors     :   Michael Antonov, Brendan Iribe

History     :   
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMATRIX2D_H
#define INC_GMATRIX2D_H

#include "GTypes.h"
#include "GTypes2DF.h"

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif



// ***** Declared classes
class   GMatrix2D;

//    This Matrix represents the matrix of type:
//    | Sx             Sky |
//    | Skx            Sy  |
//    | Tx             Ty  |
//    where M_[0][1] - is the element of the 0-column, 1st row (Skx)
//          M_[1][0] - is the element of the 1st column, 0 row (Sky)
// 
//    The rotational matrix (2x2) for this kind of matrix is
//    | cos    -sin |
//    | sin    cos  |
// 
//    Scaling matrix:
//    | Sx     0  |
//    | 0      Sy |
// 
//    Shearing matrix:
//    | 0      Sv |
//    | Sh     0  |
// 
//    Flash (flash.geom.Matrix) uses the matrix of type:
//    |    a   b   tx  |
//    |    c   d   ty  |
//    where:   a = Sx      (M_[0][0])
//             b = Sky     (M_[1][0])
//             c = Skx     (M_[0][1])
//             d = Sy      (M_[1][1])
//             tx = Tx     (M_[0][2])
//             ty = Ty     (M_[1][2])


class GMatrix2D
{
public:

    Float   M_[2][3];

    static GMatrix2D    Identity;

    // Construct an identity matrix
    GINLINE GMatrix2D();
    // Construct a matrix by copying the passed matrix
    GINLINE GMatrix2D(const GMatrix2D &m);
    // Construct a matrix using the passed values
    GINLINE GMatrix2D(Float v0, Float v1, Float v2, Float v3, Float v4, Float v5);

    // Set the matrix by copying the passed matrix
    GINLINE void        SetMatrix(const GMatrix2D &m);
    // Set the matrix using the passed values
    GINLINE void        SetMatrix(Float v0, Float v1, Float v2, Float v3, Float v4, Float v5);
    
    // Set the matrix to the identity matrix
    GEXPORT void        SetIdentity();

    // Set the matrix to the inverse of the passed matrix
    GEXPORT void        SetInverse(const GMatrix2D& m);
    // Return the inverse matrix
    GINLINE GMatrix2D   GetInverse() const;
    
    // Set this matrix to a blend of m1 and m2, parameterized by t
    GEXPORT void        SetLerp(const GMatrix2D& m1, const GMatrix2D& m2, Float t);

    // Reset back to the identity matrix
    GINLINE void        Reset();

    // Swap the matrix values
    GEXPORT void        Swap(GMatrix2D *pm);

    // Multiply this matrix by another one
    GEXPORT GMatrix2D&  Append  (const GMatrix2D& m);
    GEXPORT GMatrix2D&  Prepend (const GMatrix2D& m);

    // Scaling, Shearing, Rotation, Translation operations
    GINLINE GMatrix2D&  AppendScaling (Float scale);
    GINLINE GMatrix2D&  AppendScaling (Float sx, Float sy);
    GINLINE GMatrix2D&  PrependScaling(Float scale);
    GINLINE GMatrix2D&  PrependScaling(Float sx, Float sy);

    GINLINE GMatrix2D&  AppendShearing (Float sh, Float sv);
    GINLINE GMatrix2D&  PrependShearing(Float sh, Float sv);

    GINLINE GMatrix2D&  AppendRotation (Float radians);
    GINLINE GMatrix2D&  PrependRotation(Float radians);

    GINLINE GMatrix2D&  AppendTranslation (Float sx, Float sy);
    GINLINE GMatrix2D&  PrependTranslation(Float sx, Float sy);

    // Matrices defined by parallelograms. Src and dst are 
    // pointers to arrays of three points (Float[6], x1,y1,...) that 
    // identify three corners of the parallelograms assuming implicit 
    // fourth point. The arguments are arrays of Float[6] mapped 
    // to x1,y1, x2,y2, x3,y3  where the coordinates are:
    //        *-----------------*
    //       /          (x3,y3)/
    //      /                 /
    //     /(x1,y1)   (x2,y2)/
    //    *-----------------*
    GMatrix2D& SetParlToParl(const Float* src, const Float* dst);
    GMatrix2D& SetRectToParl(Float x1, Float y1, Float x2, Float y2, 
                             const Float* parl);
    GMatrix2D& SetParlToRect(const Float* parl, 
                             Float x1, Float y1, Float x2, Float y2);

    // Inverts the matrix
    GINLINE GMatrix2D&  Invert();

    // Checks if all matrix values are within the -MAX..MAX value range
    GEXPORT bool        IsValid() const;

    // Return true if we flip handedness
    GEXPORT bool        DoesFlip() const;
    
    // Determinant of the 2x2 rotation/scale part only
    GEXPORT Float       GetDeterminant() const;     
    // Return the maximum scale factor that this transform applies
    GEXPORT Float       GetMaxScale() const;        
    // Return x
    GEXPORT Float       GetX() const;               
    // Return y
    GEXPORT Float       GetY() const;               
    // Return the magnitude scale of our x coord output
    GEXPORT Double      GetXScale() const;          
    // Return the magnitude scale of our y coord output
    GEXPORT Double      GetYScale() const;          
    // Return our rotation component (in radians)
    GEXPORT Double      GetRotation() const;        

    // Check whether the matrix has arbitrary rotation, 
    // that is, NOT a multiple of 90 degrees.
    GINLINE bool        IsFreeRotation(Float epsilon=1e-6f) const;


    // *** Transform points

    // Transform the point by our matrix
    GEXPORT void        Transform(GPointF* result, const GPointF& p) const;
    GINLINE GPointF     Transform(const GPointF& p) const;
    GINLINE void        Transform(Float* x, Float* y) const;
    GINLINE void        Transform2x2(Float* x, Float* y) const;

    // Transform the vector by our matrix, but does not apply translation
    GEXPORT void        TransformVector(GPointF* result, const GPointF& p) const;

    // Transform the point by the inverse of our matrix
    GEXPORT void        TransformByInverse(GPointF* result, const GPointF& p) const;
    GINLINE GPointF     TransformByInverse(const GPointF& p) const;
    GINLINE void        TransformByInverse(Float* x, Float* y) const;
    
    // Transform passed rectangle 'r' and return its bounding area
    GEXPORT void        EncloseTransform(GRectF *pr, const GRectF& r) const;
    GINLINE GRectF      EncloseTransform(const GRectF& r) const;

    // Formats GMatrix2D message to a buffer, at least 512 bytes
    GEXPORT void        Format(char *pbuffer) const;


    // *** Static matrix initializers

    // Creates a translation matrix
    GINLINE static GMatrix2D        Translation(Float dx, Float dy);    
    // Creates a scaling matrix
    GINLINE static GMatrix2D        Scaling(Float scale);
    GINLINE static GMatrix2D        Scaling(Float sx, Float sy);
    // Creates a shearing matrix
    GINLINE static GMatrix2D        Shearing(Float sh, Float sv);   
    // Creates a rotation matrix 
    GINLINE static GMatrix2D        Rotation(Float radians);


    // *** Operator overrides

    // Assignment
    GINLINE const GMatrix2D&            operator =  (const GMatrix2D &m);
    GINLINE const GMatrix2D&            operator *= (const GMatrix2D &m);
    
    // Matrix equality
    GINLINE friend bool                 operator == (const GMatrix2D &m1, const GMatrix2D &m2);
    GINLINE friend bool                 operator != (const GMatrix2D &m1, const GMatrix2D &m2);
    
    // Matrix multiplication (transform values)
    // - note that GMatrix2D multiplication is not commutative
    GINLINE friend const GMatrix2D      operator * (const GMatrix2D &m1, const GMatrix2D &m2);
    GINLINE friend const GPointF        operator * (const GMatrix2D &m,  const GPointF &pt);
    GINLINE friend const GRectF         operator * (const GMatrix2D &m,  const GRectF &r);
};


// ** Inline Implementation

GINLINE GMatrix2D::GMatrix2D()
{
    SetIdentity();
}
GINLINE GMatrix2D::GMatrix2D(const GMatrix2D &m)
{
    SetMatrix(m);
}
GINLINE GMatrix2D::GMatrix2D(Float v0, Float v1, Float v2, Float v3, Float v4, Float v5)
{
    SetMatrix(v0,v1,v2,v3,v4,v5);
}

GINLINE void    GMatrix2D::SetMatrix(const GMatrix2D &m)
{
    M_[0][0] = m.M_[0][0];
    M_[0][1] = m.M_[0][1];
    M_[0][2] = m.M_[0][2];
    M_[1][0] = m.M_[1][0];
    M_[1][1] = m.M_[1][1];
    M_[1][2] = m.M_[1][2];
}
GINLINE void    GMatrix2D::SetMatrix(Float v0, Float v1, Float v2, Float v3, Float v4, Float v5)
{
    M_[0][0] = v0;
    M_[0][1] = v1;
    M_[0][2] = v4;
    M_[1][0] = v2;
    M_[1][1] = v3;
    M_[1][2] = v5;
}

GINLINE void    GMatrix2D::Reset()
{
    SetIdentity();
}

GINLINE void    GMatrix2D::Swap(GMatrix2D *pm)
{
    G_Swap(M_[0][0], pm->M_[0][0]);
    G_Swap(M_[0][1], pm->M_[0][1]);
    G_Swap(M_[0][2], pm->M_[0][2]);
    G_Swap(M_[1][0], pm->M_[1][0]);
    G_Swap(M_[1][1], pm->M_[1][1]);
    G_Swap(M_[1][2], pm->M_[1][2]);
}

GINLINE GMatrix2D&      GMatrix2D::Invert()
{
    SetInverse( GMatrix2D(*this) );
    return *this;
}
GINLINE GMatrix2D       GMatrix2D::GetInverse() const
{
    GMatrix2D m;
    m.SetInverse( *this );
    return m;
}
GINLINE GPointF GMatrix2D::Transform(const GPointF& p) const    
{ 
    GPointF pt; 
    Transform(&pt, p); 
    return pt; 
}

GINLINE GPointF GMatrix2D::TransformByInverse(const GPointF& p) const   
{ 
    GPointF pt; 
    TransformByInverse(&pt, p); 
    return pt; 
}
 
GINLINE void GMatrix2D::Transform(Float* x, Float* y) const
{
    Float tx = *x;
    Float ty = *y;
    *x = M_[0][0] * tx + M_[0][1] * ty + M_[0][2];
    *y = M_[1][0] * tx + M_[1][1] * ty + M_[1][2];
}

GINLINE void GMatrix2D::Transform2x2(Float* x, Float* y) const
{
    Float tx = *x;
    Float ty = *y;
    *x = M_[0][0] * tx + M_[0][1] * ty;
    *y = M_[1][0] * tx + M_[1][1] * ty;
}

GINLINE void GMatrix2D::TransformByInverse(Float* x, Float* y) const
{
    GMatrix2D(*this).Invert().Transform(x, y);
}

GINLINE GRectF  GMatrix2D::EncloseTransform(const GRectF& r) const      
{ 
    GRectF d; 
    EncloseTransform(&d, r); 
    return d; 
}

GINLINE GMatrix2D       GMatrix2D::Translation(Float dx, Float dy)
{
    return GMatrix2D(1.0, 0.0, 0.0, 1.0, dx, dy);
}
GINLINE GMatrix2D       GMatrix2D::Scaling(Float s)
{
    return GMatrix2D(s, 0.0, 0.0, s, 0.0, 0.0);
}
GINLINE GMatrix2D       GMatrix2D::Scaling(Float sx, Float sy)
{
    return GMatrix2D(sx, 0.0, 0.0, sy, 0.0, 0.0);
}
GINLINE GMatrix2D       GMatrix2D::Shearing(Float sh, Float sv)
{
    return GMatrix2D(1.0, tanf(sv), tanf(sh), 1.0, 0.0, 0.0);
}
GINLINE GMatrix2D       GMatrix2D::Rotation(Float r)
{
    Float cosa = cosf(r);
    Float sina = sinf(r);
    return GMatrix2D(cosa, -sina, sina, cosa, 0.0, 0.0);
}

GINLINE GMatrix2D&      GMatrix2D::PrependScaling(Float sx, Float sy)
{
    return Prepend(Scaling(sx, sy));
}

GINLINE GMatrix2D&      GMatrix2D::PrependScaling(Float scale)
{
    return PrependScaling(scale, scale);
}

GINLINE GMatrix2D&      GMatrix2D::AppendScaling (Float sx, Float sy)
{
    M_[0][0] *= sx;
    M_[0][1] *= sx;
    M_[0][2] *= sx;
    M_[1][0] *= sy;
    M_[1][1] *= sy;
    M_[1][2] *= sy;
    return *this;
}

GINLINE GMatrix2D&      GMatrix2D::AppendScaling (Float scale)
{
    return AppendScaling (scale, scale);
}

GINLINE GMatrix2D&      GMatrix2D::PrependShearing(Float sh, Float sv)
{
    return Prepend(Shearing(sh, sv));
}

GINLINE GMatrix2D&      GMatrix2D::AppendShearing (Float sh, Float sv)
{
    return Append (Shearing(sh, sv));
}

GINLINE GMatrix2D&      GMatrix2D::PrependRotation(Float radians)
{
    return Prepend(Rotation(radians));
}

GINLINE GMatrix2D&      GMatrix2D::AppendRotation (Float radians)
{
    return Append (Rotation(radians));
}

GINLINE GMatrix2D&      GMatrix2D::PrependTranslation(Float dx, Float dy)
{
    M_[0][2] += M_[0][0] * dx + M_[0][1] * dy;
    M_[1][2] += M_[1][0] * dx + M_[1][1] * dy;
    return *this;
}

GINLINE GMatrix2D&      GMatrix2D::AppendTranslation (Float dx, Float dy)
{
    M_[0][2] += dx;
    M_[1][2] += dy;
    return *this;
}

GINLINE bool            GMatrix2D::IsFreeRotation(Float epsilon) const
{
    GMatrix2D mat2(*this);
    mat2.M_[0][2] = 0;
    mat2.M_[1][2] = 0;
    GPointF p10 = mat2.Transform(GPointF(1,0));
    return fabsf(p10.x) > epsilon && fabsf(p10.y) > epsilon;
}

GINLINE bool    operator == (const GMatrix2D &m1, const GMatrix2D &m2)
{
    return  (m1.M_[0][0] == m2.M_[0][0]) && 
            (m1.M_[0][1] == m2.M_[0][1]) && 
            (m1.M_[0][2] == m2.M_[0][2]) && 
            (m1.M_[1][0] == m2.M_[1][0]) && 
            (m1.M_[1][1] == m2.M_[1][1]) && 
            (m1.M_[1][2] == m2.M_[1][2]);
}

GINLINE bool    operator != (const GMatrix2D &m1, const GMatrix2D &m2)
{
    return  (m1.M_[0][0] != m2.M_[0][0]) || 
            (m1.M_[0][1] != m2.M_[0][1]) || 
            (m1.M_[0][2] != m2.M_[0][2]) || 
            (m1.M_[1][0] != m2.M_[1][0]) || 
            (m1.M_[1][1] != m2.M_[1][1]) || 
            (m1.M_[1][2] != m2.M_[1][2]);
}

GINLINE const GMatrix2D&      GMatrix2D::operator =   (const GMatrix2D &m)
{
    SetMatrix(m);
    return *this;
}

GINLINE const GMatrix2D&      GMatrix2D::operator *= (const GMatrix2D &m)
{
    return Append(m);
}

GINLINE const GMatrix2D     operator * (const GMatrix2D &m1, const GMatrix2D &m2)
{
    return GMatrix2D(m1).Append(m2);
}

GINLINE const GPointF       operator * (const GMatrix2D &m,  const GPointF &pt)
{
    return m.Transform(pt);
}
GINLINE const GRectF        operator * (const GMatrix2D &m,  const GRectF &r)
{
    return m.EncloseTransform(r);
}

// ** End Inline Implementation


#endif
