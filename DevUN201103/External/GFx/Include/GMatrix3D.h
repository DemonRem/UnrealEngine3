/*****************************************************************

Filename    :   GMatrix3D.h
Content     :   3D Matrix class 
Created     :   Jan 15, 2010
Authors     :   Mustafa Thamer

History     :   
Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMATRIX3D_H
#define INC_GMATRIX3D_H

#include "GTypes.h"
#include "GTypes2DF.h"
#include "GMatrix2D.h"

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif
#include <string.h>     // memcpy
#include "GPoint3.h"

class GMatrix3D
{
    /*
    Row major. It is the same format as D3D and AS3 matrices.
    (currently transposed from GMatrix2D)

    The default GFx coord system is RH with +X to the right, +Y down and +Z going into the screen.  
    The default camera is looking down the +Z axis.

    Different views of the matrix:  

    1. using [i][j] indices, where i is row and j is column
    2. showing scale (S1-S3) and translation (T1-T3)
    3. using [i] indices

    | 0,0     0,1     0,2     0,3   |   S1  X   X   X   |   0   1   2   3   |
    |                               |                   |                   |
    | 1,0     1,1     1,2     1,3   |   X   S2  X   X   |   4   5   6   7   |
    |                               |                   |                   |
    | 2,0     2,1     2,2     2,3   |   X   X   S3  X   |   8   9   10  11  |
    |                               |                   |                   |
    | 3,0     3,1     3,2     3,3   |   T1  T2  T3  X   |   12  13  14  15  |

    */
public:
    Float   M_[4][4];       // accessing via [i][j] is equivalent to [i*4+j]

    static GMatrix3D    Identity;

    GINLINE GMatrix3D()                             { SetIdentity(); }
    GINLINE GMatrix3D(const Float *pVals, int count=16)   { Set(pVals, count);  }
    GINLINE GMatrix3D(const Float pVals[4][4])      { Set(&pVals[0][0], 16);  }
    GINLINE GMatrix3D(const GMatrix3D &mat)         { *this = mat;  }
    GEXPORT GMatrix3D(const GMatrix2D &m);

    // Checks if all matrix values are within the -MAX..MAX value range
    GEXPORT bool IsValid() const;

    // array accessors, [i][j] is equivalent to [i*4+j]
    GINLINE operator Float * ()                     { return FloatPtr(); } //conversion operator
    GINLINE operator const Float * () const         { return FloatPtr(); } //conversion operator
    GINLINE Float *GetAsFloatArray()                { return FloatPtr();    }
    GINLINE const Float *GetAsFloatArray() const    { return FloatPtr();    }
    GINLINE Float *FloatPtr()                       { return (Float*)M_;    }
    GINLINE const Float *FloatPtr() const           { return (Float*)M_;    }
    GINLINE Float operator [] (int pos)             { return FloatPtr()[pos]; }
    GINLINE Float operator [] (int pos) const       { return FloatPtr()[pos]; }

    GINLINE void Set(const Float *pVals, int count)
    {
        if (count == 16)
            memcpy(M_, pVals, sizeof(M_));
    }

    GINLINE void Set(const Float pVals[4][4])       { Set(&pVals[0][0], 16); }
    GEXPORT void Transpose();
    GEXPORT void SetIdentity();
    GINLINE void Clear()                            {  memset(M_, 0, sizeof(M_)); }

    GINLINE Float GetXScale() const                 { return sqrtf(M_[0][0]*M_[0][0]+M_[0][1]*M_[0][1]+M_[0][2]*M_[0][2]); }
    GINLINE Float GetYScale() const                 { return sqrtf(M_[1][0]*M_[1][0]+M_[1][1]*M_[1][1]+M_[1][2]*M_[1][2]); }
    GINLINE Float GetZScale() const                 { return sqrtf(M_[2][0]*M_[2][0]+M_[2][1]*M_[2][1]+M_[2][2]*M_[2][2]); }

    GINLINE void SetXScale(Float s)                 { s /= GetXScale(); M_[0][0] *= s; M_[0][1] *= s; M_[0][2] *= s; }
    GINLINE void SetYScale(Float s)                 { s /= GetYScale(); M_[1][0] *= s; M_[1][1] *= s; M_[1][2] *= s; }
    GINLINE void SetZScale(Float s)                 { s /= GetZScale(); M_[2][0] *= s; M_[2][1] *= s; M_[2][2] *= s; }

    GEXPORT void GetScale(Float *tX, Float *tY, Float *tZ) const;
    GINLINE void Scaling(Float sX, Float sY, Float sZ)
    {
        SetIdentity();
        M_[0][0] = sX;
        M_[1][1] = sY;
        M_[2][2] = sZ;
    }

    GEXPORT void GetTranslation(Float *tX, Float *tY, Float *tZ) const;
    GINLINE Float GetX() const                      { return M_[3][0]; }
    GINLINE Float GetY() const                      { return M_[3][1]; }
    GINLINE Float GetZ() const                      { return M_[3][2]; }

    GINLINE void SetX(Float tX)                     { M_[3][0] = tX;   }
    GINLINE void SetY(Float tY)                     { M_[3][1] = tY;   }
    GINLINE void SetZ(Float tZ)                     { M_[3][2] = tZ;   }

    GINLINE void Translation(Float tX, Float tY, Float tZ)
    {
        SetIdentity();
        M_[3][0] = tX;
        M_[3][1] = tY;
        M_[3][2] = tZ;
    }

    GEXPORT void MultiplyMatrix(const GMatrix3D &matrixA, const GMatrix3D &matrixB);

    GINLINE friend const GMatrix3D      operator * (const GMatrix3D &m1, const GMatrix3D &m2)
    {
        GMatrix3D outMat;
        outMat.MultiplyMatrix(m1, m2);
        return outMat;
    }

    GINLINE void Prepend(const GMatrix3D &matrixA)
    {
        GMatrix3D matrixThis = *this;
        MultiplyMatrix(matrixA, matrixThis);
    }

    GINLINE void Append(const GMatrix3D &matrixA)
    {
        GMatrix3D matrixThis = *this;
        MultiplyMatrix(matrixThis, matrixA);
    }

    GINLINE void    SetInverse(const GMatrix3D& mIn)    { MatrixInverse(FloatPtr(), mIn.FloatPtr());   }
    GINLINE void    Invert()                              { SetInverse( *this ); }

    GEXPORT void        Transform(GPoint3F* result, const GPoint3F& p, bool divideByW = false) const
    {
        *result = Transform(p, divideByW);
    }

    GINLINE GPoint3F    Transform(const GPoint3F& p, bool divideByW = false) const
    {
        GPoint3F result;
        result.x = M_[0][0] * p.x + M_[1][0] * p.y + M_[2][0] * p.z + M_[3][0];
        result.y = M_[0][1] * p.x + M_[1][1] * p.y + M_[2][1] * p.z + M_[3][1];
        result.z = M_[0][2] * p.x + M_[1][2] * p.y + M_[2][2] * p.z + M_[3][2];
        if (divideByW)
        {
            float w  = M_[0][3] * p.x + M_[1][3] * p.y + M_[2][3] * p.z + M_[3][3];
            result /= w; 
        }
        return result;
    }

    GINLINE GPoint3F     TransformNormal(const GPoint3F& p) const
    {
        GMatrix3D invMat = *this;
        MatrixInverse(invMat.FloatPtr(), FloatPtr());
        invMat.Transpose();
        return invMat.Transform(p);
    }

    GEXPORT void        Transform(GPointF* result, const GPointF& p, bool bDivideByW = false) const
    {
        *result = Transform(p, bDivideByW);
    }

    GINLINE GPointF     Transform(const GPointF& p, bool bDivideByW = false) const
    {
        GPoint3F p3(p.x, p.y, 1), result;
        result = Transform(p3, bDivideByW);
        return GPointF(result.x, result.y);
    }

    GINLINE GPointF     TransformByInverse(const GPointF& p) const
    {
        GMatrix3D matCopy = *this;
        matCopy.Invert();
        return matCopy.Transform(p);
    }

    GEXPORT void        EncloseTransform(GPointF *pts, const GRectF& r, bool bDivideByW = false) const;
    GEXPORT void        EncloseTransform(GRectF *pr, const GRectF& r, bool bDivideByW = false) const;
    GINLINE GRectF      EncloseTransform(const GRectF& r, bool bDivideByW = false) const
    { 
        GRectF d; 
        EncloseTransform(&d, r, bDivideByW); 
        return d; 
    }

    // returns euler angles in radians
    GINLINE void GetRotation(Float *eX, Float *eY, Float *eZ) const { return GetEulerAngles(eX, eY, eZ); }

    GEXPORT void RotateX(Float angleRad);      // bank
    GEXPORT void RotateY(Float angleRad);      // heading
    GEXPORT void RotateZ(Float angleRad);      // attitude

    // create camera view matrix, world to view transform. Right or Left-handed. 
    GEXPORT void ViewRH(const GPoint3F& eyePt, const GPoint3F& lookAtPt, const GPoint3F& upVec);
    GEXPORT void ViewLH(const GPoint3F& eyePt, const GPoint3F& lookAtPt, const GPoint3F& upVec);

    // create perspective matrix, view to screen transform.  Right or Left-handed.
    GEXPORT void PerspectiveRH(Float fovYRad, Float fAR, Float fNearZ, float fFarZ);
    GEXPORT void PerspectiveLH(Float fovYRad, Float fAR, Float fNearZ, float fFarZ);
	GEXPORT void PerspectiveFocalLengthRH(float focalLength, float DisplayWidth, float DisplayHeight, float fNearZ, float fFarZ);
	GEXPORT void PerspectiveFocalLengthLH(float focalLength, float DisplayWidth, float DisplayHeight, float fNearZ, float fFarZ);
    GEXPORT void PerspectiveViewVolRH(Float viewW, Float viewH, Float fNearZ, float fFarZ);
    GEXPORT void PerspectiveViewVolLH(Float viewW, Float viewH, Float fNearZ, float fFarZ);
    GEXPORT void PerspectiveOffCenterLH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ);
    GEXPORT void PerspectiveOffCenterRH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ);

    // create orthographic projection matrix.  Right or Left-handed.
    GEXPORT void OrthoRH(Float viewW, Float viewH, Float fNearZ, float fFarZ);
    GEXPORT void OrthoLH(Float viewW, Float viewH, Float fNearZ, float fFarZ);
    GEXPORT void OrthoOffCenterRH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ);
    GEXPORT void OrthoOffCenterLH(Float viewMinX, Float viewMaxX, Float viewMinY, Float viewMaxY, Float fNearZ, float fFarZ);

private:

    void GetEulerAngles(Float *eX, Float *eY, Float *eZ) const;
    void MatrixInverse(Float *po, const Float *pa) const;
    Float Cofactor(const Float *pa, int i, int j) const;
};

//
// dynamically allocatable matrix
//
#include "GFxPlayerStats.h"
class GMatrix3DNewable: public GMatrix3D, public GNewOverrideBase<GFxStatMV_MovieClip_Mem>
{
public:
    GINLINE void operator = (const GMatrix3D &m)
    {
        memcpy(M_, m.M_, sizeof(M_));
    }

};

#endif  // INC_GMATRIX3D_H
