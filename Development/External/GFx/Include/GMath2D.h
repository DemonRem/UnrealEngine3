/**********************************************************************

Filename    :   GMath2D.h
Content     :   
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Some simple 2D geometry math functions and constants

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#ifndef INC_GMATH2D_H
#define INC_GMATH2D_H

#include "GTypes.h"

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

// ***** Declared Classes
struct GPointType;



//---------------------------------------------------------------------------
typedef Float GCoordType;

struct GPointType
{
    GCoordType x,y;

    GPointType() {}
    GPointType(GCoordType x_, GCoordType y_) : x(x_), y(y_) {}
    bool operator == (const GPointType& p) const
    {
        return p.x == x && p.y == y;
    }
};

namespace GMath2D
{
    const GCoordType Pi = (GCoordType)3.14159265358979323846;

    //-----------------------------------------------------------------------
    GINLINE GCoordType CrossProduct(GCoordType x1, GCoordType y1,
                                    GCoordType x2, GCoordType y2,
                                    GCoordType x,  GCoordType y)
    {
        return (x - x2) * (y2 - y1) - (y - y2) * (x2 - x1);
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE GCoordType CrossProduct(const Vertex1& v1, 
                                    const Vertex1& v2,
                                    const Vertex2& v)
    {
        return (v.x - v2.x) * (v2.y - v1.y) - 
               (v.y - v2.y) * (v2.x - v1.x);
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE GCoordType AbsCrossProduct(const Vertex1& v1, 
                                       const Vertex1& v2,
                                       const Vertex2& v)
    {
        GCoordType c = (v.x - v2.x) * (v2.y - v1.y) - 
                       (v.y - v2.y) * (v2.x - v1.x);
        return (c < 0.0) ? -c : c;
    }

    //-----------------------------------------------------------------------
    template<class Vertex>
    bool PointInTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3, 
                         GCoordType x, GCoordType y)
    {
        GPointType v(x, y);
        bool cp1 = CrossProduct(v1, v2, v) < 0.0;
        bool cp2 = CrossProduct(v2, v3, v) < 0.0;
        bool cp3 = CrossProduct(v3, v1, v) < 0.0;
        return cp1 == cp2 && cp2 == cp3 && cp3 == cp1;
    }

    //-----------------------------------------------------------------------
    GINLINE GCoordType CalcDistance(GCoordType x1, GCoordType y1,
                                    GCoordType x2, GCoordType y2)
    {
        GCoordType dx = x2 - x1;
        GCoordType dy = y2 - y1;
        return (GCoordType)sqrt(dx * dx + dy * dy);
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE GCoordType CalcDistance(const Vertex1& v1, const Vertex2& v2)
    {
        GCoordType dx = v2.x - v1.x;
        GCoordType dy = v2.y - v1.y;
        return (GCoordType)sqrt(dx * dx + dy * dy);
    }


    //-----------------------------------------------------------------------
    GINLINE GCoordType CalcLinePointDistance(GCoordType x1, GCoordType y1, 
                                             GCoordType x2, GCoordType y2, 
                                             GCoordType x,  GCoordType y)
    {
        GCoordType dx = x2-x1;
        GCoordType dy = y2-y1;
        GCoordType d = sqrtf(dx * dx + dy * dy);
        if(d == 0)
        {
            return CalcDistance(x1, y1, x, y);
        }
        return ((x - x2) * dy - (y - y2) * dx) / d;
    }


    // Calculate the slope ratio of the line segment as a function
    // of the angle with respect to the 0X axis. 
    // The function is typically used to arrange a bundle of vectors in 
    // the clockwise or counterclockwise direction.
    // The return value is in range [-1...1) and essentially it corresponds 
    // to the return value of atan2. But it's not an angle, it's a kind of
    // a sinusoidal function.
    //-----------------------------------------------------------------------
    GINLINE GCoordType CalcSlopeRatio(GCoordType x1, GCoordType y1,
                                      GCoordType x2, GCoordType y2)
    {
        GCoordType bx = x2 - x1;
        GCoordType by = y2 - y1;
        GCoordType a = (bx*bx) / ((GCoordType)(bx*bx + by*by) * 2);
        if(bx < 0) a = -a;
        if(by > 0) a = 1-a;
        return a - (GCoordType)0.5;
    }

    //-----------------------------------------------------------------------
    template<class Vertex>
    GINLINE GCoordType CalcSlopeRatio(const Vertex& v1, const Vertex& v2)
    {
        return CalcSlopeRatio(v1.x, v1.y, v2.x, v2.y);
    }


    // Calculate the turn ratio of two consecutive line segments as a function
    // of the angle between them. 
    // The function is typycally used to find a leftmost of a rightmost turn
    // in a bundle of vectors with respect to the given vector.
    // The return value is in range [-1...1) and corresponds to a sinusoidal
    // function. The characteristic values are:
    // -1.0 - the second line segment goes back,
    // -0.5 - left turn with the right angle between the segments,
    //  0.0 - the three points lie in a straight line, 
    //  0.5 - right turn with the right angle between the segments.
    //-----------------------------------------------------------------------
    GINLINE GCoordType CalcTurnRatio(GCoordType x1, GCoordType y1,
                                     GCoordType x2, GCoordType y2,
                                     GCoordType x3, GCoordType y3)
    {
        GCoordType ax = x2 - x1;
        GCoordType ay = y2 - y1;
        GCoordType bx = x3 - x2;
        GCoordType by = y3 - y2;
        GCoordType a = (ax*bx + ay*by) / 
                           ((GCoordType)sqrt(ax*ax + ay*ay) * 
                            (GCoordType)sqrt(bx*bx + by*by) * 2);
        if(bx*ay > by*ax) a = 1-a;
        return a - (GCoordType)0.5;
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE GCoordType CalcTurnRatio(const Vertex1& v1, 
                                     const Vertex1& v2,
                                     const Vertex2& v3)
    {
        return CalcTurnRatio(v1.x, v1.y, v2.x, v2.y, v3.x, v3.y);
    }

    //-----------------------------------------------------------------------
    GINLINE GCoordType TriangleInscribedRadius(GCoordType x1, GCoordType y1,
                                               GCoordType x2, GCoordType y2,
                                               GCoordType x3, GCoordType y3)
    {
        GCoordType a = CalcDistance(x1, y1, x2, y2);
        GCoordType b = CalcDistance(x2, y2, x3, y3);
        GCoordType c = CalcDistance(x3, y3, x1, y1);
        GCoordType p = (a + b + c) / 2;
        return (GCoordType)sqrt((p - a) * (p - b) * (p - c) / p);
    }

    //-----------------------------------------------------------------------
    template<class Vertex>
    GINLINE GCoordType TriangleInscribedRadius(const Vertex& v1, 
                                               const Vertex& v2,
                                               const Vertex& v3)
    {
        return TriangleInscribedRadius(v1.x, v1.y, v2.x, v2.y, v3.x, v3.y);
    }

    //-----------------------------------------------------------------------
    template<class Vertex>
    GINLINE void CalcParallel(Vertex* v1, Vertex* v2, GCoordType w)
    {
        GCoordType dx  = v2->x - v1->x;
        GCoordType dy  = v1->y - v2->y;
        GCoordType d   = (GCoordType)sqrt(dx*dx + dy*dy);
        GCoordType dx2 = w * dy / d;
        GCoordType dy2 = w * dx / d;
        v1->x -= dx2;  
        v1->y -= dy2;
        v2->x -= dx2;  
        v2->y -= dy2;
    }

    //-----------------------------------------------------------------------
    GINLINE bool CalcIntersection(GCoordType ax, GCoordType ay, 
                                  GCoordType bx, GCoordType by,
                                  GCoordType cx, GCoordType cy, 
                                  GCoordType dx, GCoordType dy,
                                  GCoordType* x, GCoordType* y,
                                  GCoordType epsilon)
    {
        GCoordType na  = (ay-cy) * (dx-cx) - (ax-cx) * (dy-cy);
    //  GCoordType nb  = (bx-ax) * (ay-cy) - (by-ay) * (ax-cx);
        GCoordType den = (bx-ax) * (dy-cy) - (by-ay) * (dx-cx);
        if(fabs(den) < epsilon) 
        {
            // When the equation has no solution we just take the 
            // midpoint of midpoints of the segments.
            //----------------------
            *x = ax/4 + bx/4 + cx/4 + dx/4;
            *y = ay/4 + by/4 + cy/4 + dy/4;
            return false; 
        }
        GCoordType u = na / den;
        *x = ax + u * (bx-ax);
        *y = ay + u * (by-ay);
        return true;
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE bool CalcIntersection(const Vertex1& a, const Vertex1& b, 
                                  const Vertex2& c, const Vertex2& d, 
                                  GCoordType* x, GCoordType* y,
                                  GCoordType epsilon)
    {
        return CalcIntersection(a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y, 
                                x, y,
                                epsilon);
    }

    //-----------------------------------------------------------------------
    GINLINE bool CalcSegmentIntersection(GCoordType ax, GCoordType ay, 
                                         GCoordType bx, GCoordType by,
                                         GCoordType cx, GCoordType cy, 
                                         GCoordType dx, GCoordType dy,
                                         GCoordType* x, GCoordType* y,
                                         GCoordType epsilon)
    {
        GCoordType na  = (ay-cy) * (dx-cx) - (ax-cx) * (dy-cy);
        GCoordType nb  = (bx-ax) * (ay-cy) - (by-ay) * (ax-cx);
        GCoordType den = (bx-ax) * (dy-cy) - (by-ay) * (dx-cx);

        if(fabs(den) < epsilon) 
        {
            return false; 
        }

        GCoordType ua = na / den;
        GCoordType ub = nb / den;

        if(ua <= 0 || ua >= 1 || ub <= 0 || ub >= 1)
        {
            return false;
        }
        *x = ax + ua * (bx-ax);
        *y = ay + ua * (by-ay);
        return true;
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE bool CalcSegmentIntersection(const Vertex1& a, const Vertex1& b, 
                                         const Vertex2& c, const Vertex2& d, 
                                         GCoordType* x, GCoordType* y,
                                         GCoordType epsilon)
    {
        return CalcSegmentIntersection(a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y, 
                                       x, y,
                                       epsilon);
    }

    //-----------------------------------------------------------------------
    GINLINE GCoordType CalcPointToSegmentPos(GCoordType x1, GCoordType y1,
                                             GCoordType x2, GCoordType y2,
                                             GCoordType x,  GCoordType y)
    {
        GCoordType dx = x2 - x1;
        GCoordType dy = y2 - y1;
        if(dx == 0 && dy == 0) return 0;
        return (dx*(x - x1) + dy*(y - y1)) / (dx*dx + dy*dy);
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GCoordType CalcPointToSegmentPos(const Vertex1& v1, const Vertex1& v2,
                                     const Vertex2& v)
    {
        return CalcPointToSegmentPos(v1.x, v1.y, v2.x, v2.y, v.x, v.y);
    }

    //-----------------------------------------------------------------------
    GINLINE bool SegmentIntersectionExists(GCoordType x1, GCoordType y1,
                                           GCoordType x2, GCoordType y2,
                                           GCoordType x3, GCoordType y3,
                                           GCoordType x4, GCoordType y4)
    {
        // It's less expensive but you can't control the 
        // boundary conditions: Less or LessEqual
        Double dx1 = x2 - x1;
        Double dy1 = y2 - y1;
        Double dx2 = x4 - x3;
        Double dy2 = y4 - y3;
        return ((x3 - x2) * dy1 - (y3 - y2) * dx1 < 0) != 
               ((x4 - x2) * dy1 - (y4 - y2) * dx1 < 0) &&
               ((x1 - x4) * dy2 - (y1 - y4) * dx2 < 0) !=
               ((x2 - x4) * dy2 - (y2 - y4) * dx2 < 0);
    }

    //-----------------------------------------------------------------------
    template<class Vertex1, class Vertex2>
    GINLINE bool SegmentIntersectionExists(const Vertex1& a, const Vertex1& b, 
                                           const Vertex2& c, const Vertex2& d)
    {
        return SegmentIntersectionExists(a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y);
    }

    //-----------------------------------------------------------------------
    template<class Vertex>
    bool ShrinkTriangle(Vertex* v1, Vertex* v2, Vertex* v3, GCoordType width,
                        GCoordType epsilon)
    {
        bool ret = true;

        GCoordType a = CalcDistance(*v1, *v2);
        GCoordType b = CalcDistance(*v2, *v3);
        GCoordType c = CalcDistance(*v3, *v1);
        GCoordType p = (a + b + c) / 2;
        GCoordType r = (GCoordType)sqrt((p - a) * (p - b) * (p - c) / p);
        GCoordType x[6], y[6];

        if(r < 0.01)
        {
            GCoordType u;
            u = CalcPointToSegmentPos(*v1, *v2, *v3);
            if(u > 0 && u < 1) 
            {
                *v1 = *v2 = *v3;
                return false;
            }
            u = CalcPointToSegmentPos(*v2, *v3, *v1);
            if(u > 0 && u < 1) 
            {
                *v2 = *v3 = *v1;
                return false;
            }
            *v3 = *v1 = *v2;
            return false;
        }


        if(width < -r)
        {
            width = -r;
            ret = false;
        }

        GCoordType dx1 = width * (v2->y - v1->y) / a;
        GCoordType dy1 = width * (v2->x - v1->x) / a; 
        GCoordType dx2 = width * (v3->y - v2->y) / b;
        GCoordType dy2 = width * (v3->x - v2->x) / b; 
        GCoordType dx3 = width * (v1->y - v3->y) / c;
        GCoordType dy3 = width * (v1->x - v3->x) / c; 
        x[0] = v1->x + dx1;  y[0] = v1->y - dy1;
        x[1] = v2->x + dx1;  y[1] = v2->y - dy1;
        x[2] = v2->x + dx2;  y[2] = v2->y - dy2;
        x[3] = v3->x + dx2;  y[3] = v3->y - dy2;
        x[4] = v3->x + dx3;  y[4] = v3->y - dy3;
        x[5] = v1->x + dx3;  y[5] = v1->y - dy3;
        CalcIntersection(x[4], y[4], x[5], y[5], x[0], y[0], x[1], y[1], &v1->x, &v1->y, epsilon);
        CalcIntersection(x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3], &v2->x, &v2->y, epsilon);
        CalcIntersection(x[2], y[2], x[3], y[3], x[4], y[4], x[5], y[5], &v3->x, &v3->y, epsilon);
        return ret;
    }

    //--------------------------------------------------------------------
    GINLINE void CalcBisectrix(GCoordType x1, GCoordType y1, 
                               GCoordType x2, GCoordType y2, 
                               GCoordType x3, GCoordType y3, 
                               GCoordType* x, GCoordType* y,
                               GCoordType epsilon)
    {
        GCoordType k = CalcDistance(x2, y2, x3, y3) / 
                       CalcDistance(x1, y1, x2, y2);
        *x = x3 - (x2 - x1) * k;
        *y = y3 - (y2 - y1) * k;


        if((*x - x2) * (*x - x2) + (*y - y2) * (*y - y2) < epsilon)
        {
            *x = (x2 + x2 + (y2 - y1) + (y3 - y3)) / 2;
            *y = (y2 + y2 - (x2 - x1) - (x3 - x3)) / 2;
        }
    }


    //--------------------------------------------------------------------
    GINLINE unsigned URound(GCoordType v) 
    { 
        return unsigned(v + 0.5f);
    }


    //--------------------------------------------------------------------
    inline void CalcPointOnQuadCurve(GCoordType x1, GCoordType y1,
                                     GCoordType x2, GCoordType y2,
                                     GCoordType x3, GCoordType y3,
                                     GCoordType t,
                                     GCoordType* x, GCoordType* y)
    {
        GCoordType x12  = x1  + t*(x2  - x1);
        GCoordType y12  = y1  + t*(y2  - y1);
        GCoordType x23  = x2  + t*(x3  - x2);
        GCoordType y23  = y2  + t*(y3  - y2);
                     *x = x12 + t*(x23 - x12);
                     *y = y12 + t*(y23 - y12);
    }


    //--------------------------------------------------------------------
    inline GCoordType CalcQuadCurveExtremum(GCoordType x1, GCoordType x2, GCoordType x3)
    {
        GCoordType dx = 2 * x2 - x1 - x3;
        return (dx == 0) ? -1 : (x2 - x1) / dx;
    }


} // GMath2D

#endif

