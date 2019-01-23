/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

//	Constants.

#define MAX_SH_ORDER	2
#define MAX_SH_BASIS	(MAX_SH_ORDER*MAX_SH_ORDER)

/** A vector of spherical harmonic coefficients. */
class FSHVector
{
public:

	FLOAT V[MAX_SH_BASIS];

	/** Default constructor. */
	FSHVector()
	{
		for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		{
			V[BasisIndex] = 0.0f;
		}
	}

	/** Scalar multiplication operator. */
	friend FSHVector operator*(const FSHVector& A,FLOAT B)
	{
		FSHVector Result;
		for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		{
			Result.V[BasisIndex] = A.V[BasisIndex] * B;
		}
		return Result;
	}

	/** Addition operator. */
	friend FSHVector operator+(const FSHVector& A,const FSHVector& B)
	{
		FSHVector Result;
		for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		{
			Result.V[BasisIndex] = A.V[BasisIndex] + B.V[BasisIndex];
		}
		return Result;
	}
	
	/** Subtraction operator. */
	friend FSHVector operator-(const FSHVector& A,const FSHVector& B)
	{
		FSHVector Result;
		for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		{
			Result.V[BasisIndex] = A.V[BasisIndex] - B.V[BasisIndex];
		}
		return Result;
	}

	/** Dot product operator. */
	friend FLOAT Dot(const FSHVector& A,const FSHVector& B)
	{
		FLOAT Result = 0.0f;
		for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		{
			Result += A.V[BasisIndex] * B.V[BasisIndex];
		}
		return Result;
	}

	/** In-place addition operator. */
	FSHVector& operator+=(const FSHVector& B)
	{
		*this = *this + B;
		return *this;
	}
	
	/** In-place subtraction operator. */
	FSHVector& operator-=(const FSHVector& B)
	{
		*this = *this - B;
		return *this;
	}

	/** In-place scalar division operator. */
	FSHVector& operator/=(FLOAT B)
	{
		*this = *this * (1.0f / B);
		return *this;
	}

	/** In-place scalar multiplication operator. */
	FSHVector& operator*=(FLOAT B)
	{
		*this = *this * B;
		return *this;
	}
};

/** A vector of colored spherical harmonic coefficients. */
class FSHVectorRGB
{
public:

	FSHVector R;
	FSHVector G;
	FSHVector B;

	/** Calculates greyscale spherical harmonic coefficients. */
	FSHVector GetLuminance() const
	{
		return R * 0.3f + G * 0.59f + B * 0.11f;
	}

	/** Scalar multiplication operator. */
	friend FSHVectorRGB operator*(const FSHVectorRGB& A,FLOAT B)
	{
		FSHVectorRGB Result;
		Result.R = A.R * B;
		Result.G = A.G * B;
		Result.B = A.B * B;
		return Result;
	}

	/** Addition operator. */
	friend FSHVectorRGB operator+(const FSHVectorRGB& A,const FSHVectorRGB& B)
	{
		FSHVectorRGB Result;
		Result.R = A.R + B.R;
		Result.G = A.G + B.G;
		Result.B = A.B + B.B;
		return Result;
	}
	
	/** Subtraction operator. */
	friend FSHVectorRGB operator-(const FSHVectorRGB& A,const FSHVectorRGB& B)
	{
		FSHVectorRGB Result;
		Result.R = A.R - B.R;
		Result.G = A.G - B.G;
		Result.B = A.B - B.B;
		return Result;
	}

	/** Dot product operator. */
	friend FLinearColor Dot(const FSHVectorRGB& A,const FSHVector& B)
	{
		FLinearColor Result(FLinearColor::Black);
		Result.R = Dot(A.R,B);
		Result.G = Dot(A.G,B);
		Result.B = Dot(A.B,B);
		return Result;
	}

	/** In-place addition operator. */
	FSHVectorRGB& operator+=(const FSHVectorRGB& InB)
	{
		*this = *this + InB;
		return *this;
	}
	
	/** In-place subtraction operator. */
	FSHVectorRGB& operator-=(const FSHVectorRGB& InB)
	{
		*this = *this - InB;
		return *this;
	}
};

/** Color multiplication operator. */
FORCEINLINE FSHVectorRGB operator*(const FSHVector& A,const FLinearColor& B)
{
	FSHVectorRGB Result;
	Result.R = A * B.R;
	Result.G = A * B.G;
	Result.B = A * B.B;
	return Result;
}

/** Returns the value of the SH basis L,M at the point on the sphere defined by the unit vector Vector. */
extern FSHVector SHBasisFunction(const FVector& Vector);
