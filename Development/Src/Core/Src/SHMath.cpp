/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "CorePrivate.h"

//
//	Spherical harmonic globals.
//
FLOAT	NormalizationConstants[MAX_SH_BASIS];
INT		BasisL[MAX_SH_BASIS];
INT		BasisM[MAX_SH_BASIS];

/** Computes a factorial. */
FORCEINLINE INT Factorial(INT A)
{
	if(A == 0)
	{
		return 1;
	}
	else
	{
		return A * Factorial(A - 1);
	}
}

/** Initializes the tables used to calculate SH values. */
static INT InitSHTables()
{
	INT	L = 0,
		M = 0;

	for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
	{
		BasisL[BasisIndex] = L;
		BasisM[BasisIndex] = M;

		NormalizationConstants[BasisIndex] = appSqrt(
			(FLOAT(2 * L + 1) / FLOAT(4 * PI)) *
			(FLOAT(Factorial(L - Abs(M))) / FLOAT(Factorial(L + Abs(M))))
			);

		if(M != 0)
			NormalizationConstants[BasisIndex] *= appSqrt(2.f);

		M++;
		if(M > L)
		{
			L++;
			M = -L;
		}
	}

	return 0;
}
static INT InitDummy = InitSHTables();

/** Returns the basis index of the SH basis L,M. */
FORCEINLINE INT SHGetBasisIndex(INT L,INT M)
{
	return L * (L + 1) + M;
}

/** Evaluates the LegendrePolynomial for L,M at X */
FORCEINLINE FLOAT LegendrePolynomial(INT L,INT M,FLOAT X)
{
	switch(L)
	{
	case 0:
		return 1;
	case 1:
		if(M == 0)
			return X;
		else if(M == 1)
			return -appSqrt(1 - X * X);
		break;
	case 2:
		if(M == 0)
			return -0.5f + (3 * X * X) / 2;
		else if(M == 1)
			return -3 * X * appSqrt(1 - X * X);
		else if(M == 2)
			return -3 * (-1 + X * X);
		break;
	case 3:
		if(M == 0)
			return -(3 * X) / 2 + (5 * X * X * X) / 2;
		else if(M == 1)
			return -3 * appSqrt(1 - X * X) / 2 * (-1 + 5 * X * X);
		else if(M == 2)
			return -15 * (-X + X * X * X);
		else if(M == 3)
			return -15 * appPow(1 - X * X,1.5f);
		break;
	case 4:
		if(M == 0)
			return 0.125f * (3.0f - 30.0f * X * X + 35.0f * X * X * X * X);
		else if(M == 1)
			return -2.5f * X * appSqrt(1.0f - X * X) * (7.0f * X * X - 3.0f);
		else if(M == 2)
			return -7.5f * (1.0f - 8.0f * X * X + 7.0f * X * X * X * X);
		else if(M == 3)
			return -105.0f * X * appPow(1 - X * X,1.5f);
		else if(M == 4)
			return 105.0f * Square(X * X - 1.0f);
		break;
	case 5:
		if(M == 0)
			return 0.125f * X * (15.0f - 70.0f * X * X + 63.0f * X * X * X * X);
		else if(M == 1)
			return -1.875f * appSqrt(1.0f - X * X) * (1.0f - 14.0f * X * X + 21.0f * X * X * X * X);
		else if(M == 2)
			return -52.5f * (X - 4.0f * X * X * X + 3.0f * X * X * X * X * X);
		else if(M == 3)
			return -52.5f * appPow(1.0f - X * X,1.5f) * (9.0f * X * X - 1.0f);
		else if(M == 4)
			return 945.0f * X * Square(X * X - 1);
		else if(M == 5)
			return -945.0f * appPow(1.0f - X * X,2.5f);
		break;
	};

	return 0.0f;
}

FSHVector SHBasisFunction(const FVector& Vector)
{
	FSHVector	Result;

	// Initialize the result to the normalization constant.
	for(INT BasisIndex = 0;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		Result.V[BasisIndex] = NormalizationConstants[BasisIndex];

	// Multiply the result by the phi-dependent part of the SH bases.
	FLOAT	Phi = appAtan2(Vector.Y,Vector.X);
	for(INT BandIndex = 1;BandIndex < MAX_SH_ORDER;BandIndex++)
	{
		FLOAT	SinPhiM = GMath.SinFloat(BandIndex * Phi),
				CosPhiM = GMath.CosFloat(BandIndex * Phi);

		for(INT RecurrentBandIndex = BandIndex;RecurrentBandIndex < MAX_SH_ORDER;RecurrentBandIndex++)
		{
			Result.V[SHGetBasisIndex(RecurrentBandIndex,-BandIndex)] *= SinPhiM;
			Result.V[SHGetBasisIndex(RecurrentBandIndex,+BandIndex)] *= CosPhiM;
		}
	}

	// Multiply the result by the theta-dependent part of the SH bases.
	for(INT BasisIndex = 1;BasisIndex < MAX_SH_BASIS;BasisIndex++)
		Result.V[BasisIndex] *= LegendrePolynomial(BasisL[BasisIndex],Abs(BasisM[BasisIndex]),Vector.Z);

	return Result;
}
