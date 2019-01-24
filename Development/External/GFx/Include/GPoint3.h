#ifndef INC_GPOINT3_H
#define INC_GPOINT3_H

/*****************************************************************

Filename    :   GPoint3.h
Content     :   3D Point template class 
Created     :   Jan 15, 2010
Authors     :   Mustafa Thamer

History     :   
Copyright   :   (c) 2010 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

template <class T>
class GPoint3
{
public:
    T   x;
    T   y;
    T   z;

    GINLINE GPoint3()                               { }
    GINLINE explicit GPoint3(T value)               { SetPoint(value); }
    GINLINE GPoint3(T x2, T y2, T z2)               { SetPoint(x2,y2, z2); }
    GINLINE GPoint3(const GPoint3<T> &pt)           { SetPoint(pt); }

    // *** Initialization                                                                                               
    GINLINE void SetPoint(T val)                    { x=y=z=val; }
    GINLINE void SetPoint(T x2, T y2, T z2)         { x=x2; y=y2; z=z2;}
    GINLINE void SetPoint(const GPoint3<T> &pt)     { SetPoint(pt.x, pt.y, pt.z); }
    GINLINE void Clear()                            { x=y=z=0; }

    // Offset the position
    GINLINE GPoint3<T>& Offset(const GPoint3<T> &pt) { return Offset(pt.x, pt.y, pt.z); }
    GINLINE GPoint3<T>& Offset(T val)               { return Offset(val,val, val); }
    GINLINE GPoint3<T>& Offset(T x2, T y2, T z2)    { x+=x2; y+=y2; z+=z2; return *this; }

    // Multiply the position
    GINLINE GPoint3<T>& Mul(const GPoint3<T> &pt)   { return Mul(pt.x, pt.y, pt.z); }
    GINLINE GPoint3<T>& Mul(T val)                  { return Mul(val,val, val); }
    GINLINE GPoint3<T>& Mul(T x2, T y2, T z2)       { x*=x2; y*=y2; z*=z2; return *this; }

    // Divide the position                                                      
    GINLINE GPoint3<T>& Div(const GPoint3<T> &pt)   { return Div(pt.x, pt.y, pt.z); }
    GINLINE GPoint3<T>& Div(T val)                  { return Div(val,val, val); }    
    GINLINE GPoint3<T>& Div(T x2, T y2, T z2)       { x/=x2; y/=y2; z/=z2; return *this; }

    // Distance squared (between this and the passed point)
    GINLINE T DistanceSquared(T x2,T y2, T z2) const        { return ((x2-x)*(x2-x)+(y2-y)*(y2-y)+(z2-z)*(z2-z)); }
    GINLINE T DistanceSquared(const GPoint3<T> &pt) const   { return DistanceSquared(pt.x, pt.y, pt.z); }
    GINLINE T DistanceSquared() const                       { return Dot(*this); }

    // Distance
    GINLINE T Distance(T x2,T y2, T z2) const       { return T(sqrt(DistanceSquared(x2,y2,z2))); }
    GINLINE T Distance(const GPoint3<T> &pt) const  { return Distance(pt.x, pt.y, pt.z); }
    GINLINE T Distance() const                      { return T(sqrt(DistanceSquared())); }

    GINLINE T Magnitude() const                     { return sqrt(x*x+y*y+z*z); }
    GINLINE void Normalize() 
    {
        T length = Magnitude();
        x = x/length;
        y = y/length;
        z = z/length;
    }

    // Dot product
    GINLINE T Dot(T x2,T y2, T z2) const            { return (x*x2 + y*y2 + z*z2); }
    GINLINE T Dot(const GPoint3<T> &pt) const       { return Dot(pt.x, pt.y, pt.z); }

    // cross product
    GINLINE void Cross(const GPoint3<T> &a, const GPoint3<T> &b)
    {
        x = a.y*b.z-a.z*b.y;
        y = a.z*b.x-a.x*b.z;
        z = a.x*b.y-a.y*b.x;
    }
    GINLINE const GPoint3<T>& operator = (const GPoint3<T> &pt)  { SetPoint(pt); return *this; }
    GINLINE const GPoint3<T>& operator = (T val)    {  SetPoint(val); return *this; }

    GINLINE const GPoint3<T>   operator - () const  { return GPoint3<T>(-x, -y, -z); }

    GINLINE const GPoint3<T>& operator += (const GPoint3<T> &pt)    { return Offset(pt); }
    GINLINE const GPoint3<T>& operator += (T val)                   { return Offset(val); }

    GINLINE const GPoint3<T>& operator -= (const GPoint3<T> &pt)    { return Offset(-pt.x, -pt.y, -pt.z); }
    GINLINE const GPoint3<T>& operator -= (T val)                   { return Offset(-val); }

    GINLINE const GPoint3<T>& operator *= (const GPoint3<T> &pt)    { return Mul(pt); }
    GINLINE const GPoint3<T>& operator *= (T val)                   { return Mul(val); }

    GINLINE const GPoint3<T>& operator /= (const GPoint3<T> &pt)    { return Div(pt); }
    GINLINE const GPoint3<T>& operator /= (T val)                   { return Div(val); }

    GINLINE bool operator == (const GPoint3<T> &pt) const    { return ( (x==pt.x)&&(y==pt.y)&&(z==pt.z) ); }
    GINLINE bool operator != (const GPoint3<T> &pt) const    { return ( (x!=pt.x)||(y!=pt.y) || (z!=pt.z)); }

    GINLINE const GPoint3<T>   operator + (const GPoint3<T> &pt) const  { return GPoint3<T>(x+pt.x, y+pt.y, z+pt.z); }
    GINLINE const GPoint3<T>   operator + (T val) const                 { return GPoint3<T>(x+val, y+val, z+val); }
    GINLINE const GPoint3<T>   operator - (const GPoint3<T> &pt) const  { return GPoint3<T>(x-pt.x, y-pt.y, z-pt.z); }
    GINLINE const GPoint3<T>   operator - (T val) const                 { return GPoint3<T>(x-val, y-val, z-val); }
    GINLINE const GPoint3<T>   operator * (const GPoint3<T> &pt) const  { return GPoint3<T>(x*pt.x, y*pt.y, z*pt.z); }
    GINLINE const GPoint3<T>   operator * (T val) const                 { return GPoint3<T>(x*val, y*val, z*val); }
    //GINLINE const GPoint3<T>   operator * (T val, const GPoint3<T> &pt)               { return GPoint3<T>(pt.x*val, pt.y*val); }
    GINLINE const GPoint3<T>   operator / (const GPoint3<T> &pt) const  { return GPoint3<T>(x/pt.x, y/pt.y, z/pt.z); }
    GINLINE const GPoint3<T>   operator / (T val) const                 { return GPoint3<T>(x/val, y/val, z/val); }
};

typedef GPoint3<Float> GPoint3F;

#endif      // #ifndef INC_GPOINT3_H

