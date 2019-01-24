/**********************************************************************

Filename    :   GAlgorithm.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GAlgorithm_H
#define INC_GAlgorithm_H

#include "GTypes.h"


// ***** GOperatorLess
//
//------------------------------------------------------------------------
template<class T> struct GOperatorLess
{
    static bool Compare(const T& a, const T& b)
    {
        return a < b;
    }
};


// ***** G_QuickSortSliced
//
// Sort any part of any array: plain, GArray, GArrayPaged, GArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The comparison predicate must be specified.
//
// The code of QuickSort, was taken from the 
// Anti-Grain Geometry Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corp. by the author of Anti-Grain Geometry Project.
//------------------------------------------------------------------------
template<class Array, class Less> 
void G_QuickSortSliced(Array& arr, UPInt start, UPInt end, Less less)
{
    enum 
    {
        Threshold = 9
    };

    if(end - start <  2) return;

    SPInt  stack[80];
    SPInt* top   = stack; 
    SPInt  base  = (SPInt)start;
    SPInt  limit = (SPInt)end;

    for(;;)
    {
        SPInt len = limit - base;
        SPInt i, j, pivot;

        if(len > Threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            G_Swap(arr[base], arr[pivot]);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j 
            if(less(arr[j],    arr[i])) G_Swap(arr[j],    arr[i]);
            if(less(arr[base], arr[i])) G_Swap(arr[base], arr[i]);
            if(less(arr[j], arr[base])) G_Swap(arr[j], arr[base]);

            for(;;)
            {
                do i++; while( less(arr[i], arr[base]) );
                do j--; while( less(arr[base], arr[j]) );

                if( i > j )
                {
                    break;
                }

                G_Swap(arr[i], arr[j]);
            }

            G_Swap(arr[base], arr[j]);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less(arr[j + 1], arr[j]); j--)
                {
                    G_Swap(arr[j + 1], arr[j]);
                    if(j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
}


// ***** G_QuickSortSliced
//
// Sort any part of any array: plain, GArray, GArrayPaged, GArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The data type must have a defined "<" operator.
//------------------------------------------------------------------------
template<class Array> 
void G_QuickSortSliced(Array& arr, UPInt start, UPInt end)
{
    typedef typename Array::ValueType ValueType;
    G_QuickSortSliced(arr, start, end, GOperatorLess<ValueType>::Compare);
}

// Same as corresponding G_QuickSortSliced but with checking array limits to avoid
// crash in the case of wrong comparator functor.
template<class Array, class Less> 
bool G_QuickSortSlicedSafe(Array& arr, UPInt start, UPInt end, Less less)
{
    enum 
    {
        Threshold = 9
    };

    if(end - start <  2) return true;

    SPInt  stack[80];
    SPInt* top   = stack; 
    SPInt  base  = (SPInt)start;
    SPInt  limit = (SPInt)end;

    for(;;)
    {
        SPInt len = limit - base;
        SPInt i, j, pivot;

        if(len > Threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            G_Swap(arr[base], arr[pivot]);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j 
            if(less(arr[j],    arr[i])) G_Swap(arr[j],    arr[i]);
            if(less(arr[base], arr[i])) G_Swap(arr[base], arr[i]);
            if(less(arr[j], arr[base])) G_Swap(arr[j], arr[base]);

            for(;;)
            {
                do 
                {   
                    i++; 
                    if (i >= limit)
                        return false;
                } while( less(arr[i], arr[base]) );
                do 
                {
                    j--; 
                    if (j < 0)
                        return false;
                } while( less(arr[base], arr[j]) );

                if( i > j )
                {
                    break;
                }

                G_Swap(arr[i], arr[j]);
            }

            G_Swap(arr[base], arr[j]);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less(arr[j + 1], arr[j]); j--)
                {
                    G_Swap(arr[j + 1], arr[j]);
                    if(j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
    return true;
}

template<class Array> 
bool G_QuickSortSlicedSafe(Array& arr, UPInt start, UPInt end)
{
    typedef typename Array::ValueType ValueType;
    return G_QuickSortSlicedSafe(arr, start, end, GOperatorLess<ValueType>::Compare);
}

//------------------------------------------------------------------------
// ***** G_QuickSort
//
// Sort an array GArray, GArrayPaged, GArrayUnsafe.
// The array must have GetSize() function.
// The comparison predicate must be specified.
//------------------------------------------------------------------------
template<class Array, class Less> 
void G_QuickSort(Array& arr, Less less)
{
    G_QuickSortSliced(arr, 0, arr.GetSize(), less);
}

// checks for boundaries
template<class Array, class Less> 
bool G_QuickSortSafe(Array& arr, Less less)
{
    return G_QuickSortSlicedSafe(arr, 0, arr.GetSize(), less);
}

//------------------------------------------------------------------------
// ***** G_QuickSort
//
// Sort an array GArray, GArrayPaged, GArrayUnsafe.
// The array must have GetSize() function.
// The data type must have a defined "<" operator.
template<class Array> 
void G_QuickSort(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    G_QuickSortSliced(arr, 0, arr.GetSize(), GOperatorLess<ValueType>::Compare);
}

// checks for boundaries
template<class Array> 
bool G_QuickSortSafe(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    return G_QuickSortSlicedSafe(arr, 0, arr.GetSize(), GOperatorLess<ValueType>::Compare);
}



// ***** G_InsertionSortSliced
//
// Sort any part of any array: plain, GArray, GArrayPaged, GArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The comparison predicate must be specified.
// Unlike Quick Sort, the Insertion Sort works much slower in average, 
// but may be much faster on almost sorted arrays. Besides, it guarantees
// that the elements will not be swapped if not necessary. For example, 
// an array with all equal elements will remain "untouched", while 
// Quick Sort will considerably shuffle the elements in this case.
//------------------------------------------------------------------------
template<class Array, class Less> 
void G_InsertionSortSliced(Array& arr, UPInt start, UPInt end, Less less)
{
    SPInt j = (UPInt)start;
    SPInt i = j + 1;
    SPInt limit = (UPInt)end;

    for(; i < limit; j = i, i++)
    {
        for(; less(arr[j + 1], arr[j]); j--)
        {
            G_Swap(arr[j + 1], arr[j]);
            if(j <= 0)
            {
                break;
            }
        }
    }
}


// ***** G_InsertionSortSliced
//
// Sort any part of any array: plain, GArray, GArrayPaged, GArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The data type must have a defined "<" operator.
//------------------------------------------------------------------------
template<class Array> 
void G_InsertionSortSliced(Array& arr, UPInt start, UPInt end)
{
    typedef typename Array::ValueType ValueType;
    G_InsertionSortSliced(arr, start, end, GOperatorLess<ValueType>::Compare);
}


// ***** G_InsertionSort
//
// Sort an array GArray, GArrayPaged, GArrayUnsafe.
// The array must have GetSize() function.
// The comparison predicate must be specified.
//------------------------------------------------------------------------
template<class Array, class Less> 
void G_InsertionSort(Array& arr, Less less)
{
    G_InsertionSortSliced(arr, 0, arr.GetSize(), less);
}


// ***** G_InsertionSort
//
// Sort an array GArray, GArrayPaged, GArrayUnsafe.
// The array must have GetSize() function.
// The data type must have a defined "<" operator.
//------------------------------------------------------------------------
template<class Array> 
void G_InsertionSort(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    G_InsertionSortSliced(arr, 0, arr.GetSize(), GOperatorLess<ValueType>::Compare);
}





// ***** G_LowerBoundSliced
//
//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
UPInt G_LowerBoundSliced(const Array& arr, UPInt start, UPInt end, const Value& val, Less less)
{
    SPInt first = (SPInt)start;
    SPInt len   = (SPInt)(end - start);
    SPInt half;
    SPInt middle;
    
    while(len > 0) 
    {
        half = len >> 1;
        middle = first + half;
        if(less(arr[middle], val)) 
        {
            first = middle + 1;
            len   = len - half - 1;
        }
        else
        {
            len = half;
        }
    }
    return (UPInt)first;
}


// ***** G_LowerBoundSliced
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_LowerBoundSliced(const Array& arr, UPInt start, UPInt end, const Value& val)
{
    return G_LowerBoundSliced(arr, start, end, val, GOperatorLess<Value>::Compare);
}


// ***** G_LowerBoundSized
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_LowerBoundSized(const Array& arr, UPInt size, const Value& val)
{
    return G_LowerBoundSliced(arr, 0, size, val, GOperatorLess<Value>::Compare);
}


// ***** G_LowerBound
//
//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
UPInt G_LowerBound(const Array& arr, const Value& val, Less less)
{
    return G_LowerBoundSliced(arr, 0, arr.GetSize(), val, less);
}


// ***** G_LowerBound
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_LowerBound(const Array& arr, const Value& val)
{
    return G_LowerBoundSliced(arr, 0, arr.GetSize(), val, GOperatorLess<Value>::Compare);
}




// ***** G_UpperBoundSliced
//
//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
UPInt G_UpperBoundSliced(const Array& arr, UPInt start, UPInt end, const Value& val, Less less)
{
    SPInt first = (SPInt)start;
    SPInt len   = (SPInt)(end - start);
    SPInt half;
    SPInt middle;
    
    while(len > 0) 
    {
        half = len >> 1;
        middle = first + half;
        if(less(val, arr[middle]))
        {
            len = half;
        }
        else 
        {
            first = middle + 1;
            len   = len - half - 1;
        }
    }
    return (UPInt)first;
}


// ***** G_UpperBoundSliced
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_UpperBoundSliced(const Array& arr, UPInt start, UPInt end, const Value& val)
{
    return G_UpperBoundSliced(arr, start, end, val, GOperatorLess<Value>::Compare);
}


// ***** G_UpperBoundSized
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_UpperBoundSized(const Array& arr, UPInt size, const Value& val)
{
    return G_UpperBoundSliced(arr, 0, size, val, GOperatorLess<Value>::Compare);
}


// ***** G_UpperBound
//
//-----------------------------------------------------------------------
template<class Array, class Value, class Less>
UPInt G_UpperBound(const Array& arr, const Value& val, Less less)
{
    return G_UpperBoundSliced(arr, 0, arr.GetSize(), val, less);
}


// ***** G_UpperBound
//
//-----------------------------------------------------------------------
template<class Array, class Value>
UPInt G_UpperBound(const Array& arr, const Value& val)
{
    return G_UpperBoundSliced(arr, 0, arr.GetSize(), val, GOperatorLess<Value>::Compare);
}


// ***** G_ReverseArray
//
//-----------------------------------------------------------------------
template<class Array> void G_ReverseArray(Array& arr)
{
    SPInt from = 0;
    SPInt to   = arr.GetSize() - 1;
    while(from < to)
    {
        G_Swap(arr[from], arr[to]);
        ++from;
        --to;
    }
}



// ***** G_AppendArray
//
template<class CDst, class CSrc> 
void G_AppendArray(CDst& dst, const CSrc& src)
{
    UPInt i;
    for(i = 0; i < src.GetSize(); i++) 
        dst.PushBack(src[i]);
}


// ***** GArrayAdaptor
//
// A simple adapter that provides the GetSize() method and overloads 
// operator []. Used to wrap plain arrays in QuickSort and such.
//------------------------------------------------------------------------
template<class T> class GArrayAdaptor
{
public:
    typedef T ValueType;
    GArrayAdaptor() : Data(0), Size(0) {}
    GArrayAdaptor(T* ptr, UPInt size) : Data(ptr), Size(size) {}
    UPInt GetSize() const { return Size; }
    const T& operator [] (UPInt i) const { return Data[i]; }
          T& operator [] (UPInt i)       { return Data[i]; }
private:
    T*      Data;
    UPInt   Size;
};


// ***** GConstArrayAdaptor
//
// A simple const adapter that provides the GetSize() method and overloads 
// operator []. Used to wrap plain arrays in LowerBound and such.
//------------------------------------------------------------------------
template<class T> class GConstArrayAdaptor
{
public:
    typedef T ValueType;
    GConstArrayAdaptor() : Data(0), Size(0) {}
    GConstArrayAdaptor(const T* ptr, UPInt size) : Data(ptr), Size(size) {}
    UPInt GetSize() const { return Size; }
    const T& operator [] (UPInt i) const { return Data[i]; }
private:
    const T* Data;
    UPInt    Size;
};



#endif
