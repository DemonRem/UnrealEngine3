/*=============================================================================
	AnimationCompression.h: Skeletal mesh animation compression.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#ifndef __ANIMATIONCOMPRESSION_H__
#define __ANIMATIONCOMPRESSION_H__

#include "FloatPacker.h"

#define Quant16BitDiv     (32767.f)
#define Quant16BitFactor  (32767.f)
#define Quant16BitOffs    (32767)

#define Quant10BitDiv     (511.f)
#define Quant10BitFactor  (511.f)
#define Quant10BitOffs    (511)

#define Quant11BitDiv     (1023.f)
#define Quant11BitFactor  (1023.f)
#define Quant11BitOffs    (1023)

class FQuatFixed48NoW
{
public:
	WORD X;
	WORD Y;
	WORD Z;

	FQuatFixed48NoW()
	{}

	explicit FQuatFixed48NoW(const FQuat& Quat)
	{
		FromQuat( Quat );
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp( Quat );
		if ( Temp.W < 0.f )
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		X = (INT)(Temp.X * Quant16BitFactor) + Quant16BitOffs;
		Y = (INT)(Temp.Y * Quant16BitFactor) + Quant16BitOffs;
		Z = (INT)(Temp.Z * Quant16BitFactor) + Quant16BitOffs;
	}

	void ToQuat(FQuat& Out) const
	{
		const FLOAT FX = ((INT)X - (INT)Quant16BitOffs) / Quant16BitDiv;
		const FLOAT FY = ((INT)Y - (INT)Quant16BitOffs) / Quant16BitDiv;
		const FLOAT FZ = ((INT)Z - (INT)Quant16BitOffs) / Quant16BitDiv;
		const FLOAT WSquared = 1.f - FX*FX - FY*FY - FZ*FZ;

		Out.X = FX;
		Out.Y = FY;
		Out.Z = FZ;
		Out.W = WSquared > 0.f ? appSqrt( WSquared ) : 0.f;
	}

	friend FArchive& operator<<(FArchive& Ar, FQuatFixed48NoW& Quat)
	{
		Ar << Quat.X;
		Ar << Quat.Y;
		Ar << Quat.Z;
		return Ar;
	}
};

class FQuatFixed32NoW
{
public:
	DWORD Packed;

	FQuatFixed32NoW()
	{}

	explicit FQuatFixed32NoW(const FQuat& Quat)
	{
		FromQuat( Quat );
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp( Quat );
		if ( Temp.W < 0.f )
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		const DWORD PackedX = (INT)(Temp.X * Quant11BitFactor) + Quant11BitOffs;
		const DWORD PackedY = (INT)(Temp.Y * Quant11BitFactor) + Quant11BitOffs;
		const DWORD PackedZ = (INT)(Temp.Z * Quant10BitFactor) + Quant10BitOffs;

		// 21-31 X, 10-20 Y, 0-9 Z.
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out) const
	{
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		const DWORD ZMask = 0x000003ff;
		const DWORD YMask = 0x001ffc00;
		const DWORD XMask = 0xffe00000;

		const DWORD UnpackedX = Packed >> XShift;
		const DWORD UnpackedY = (Packed & YMask) >> YShift;
		const DWORD UnpackedZ = (Packed & ZMask);

		const FLOAT X = ((INT)UnpackedX - (INT)Quant11BitOffs) / Quant11BitDiv;
		const FLOAT Y = ((INT)UnpackedY - (INT)Quant11BitOffs) / Quant11BitDiv;
		const FLOAT Z = ((INT)UnpackedZ - (INT)Quant10BitOffs) / Quant10BitDiv;
		const FLOAT WSquared = 1.f - X*X - Y*Y - Z*Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? appSqrt( WSquared ) : 0.f;
	}

	friend FArchive& operator<<(FArchive& Ar, FQuatFixed32NoW& Quat)
	{
		Ar << Quat.Packed;
		return Ar;
	}
};

class FQuatFloat96NoW
{
public:
	FLOAT X;
	FLOAT Y;
	FLOAT Z;

	FQuatFloat96NoW()
	{}

	explicit FQuatFloat96NoW(const FQuat& Quat)
	{
		FromQuat( Quat );
	}

	FQuatFloat96NoW(FLOAT InX, FLOAT InY, FLOAT InZ)
		:	X( InX )
		,	Y( InY )
		,	Z( InZ )
	{}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp( Quat );
		if ( Temp.W < 0.f )
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();
		X = Temp.X;
		Y = Temp.Y;
		Z = Temp.Z;
	}

	void ToQuat(FQuat& Out) const
	{
		const FLOAT WSquared = 1.f - X*X - Y*Y - Z*Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? appSqrt( WSquared ) : 0.f;
	}

	friend FArchive& operator<<(FArchive& Ar, FQuatFloat96NoW& Quat)
	{
		Ar << Quat.X;
		Ar << Quat.Y;
		Ar << Quat.Z;
		return Ar;
	}
};

class FQuatIntervalFixed32NoW
{
public:
	DWORD Packed;

	FQuatIntervalFixed32NoW()
	{}

	explicit FQuatIntervalFixed32NoW(const FQuat& Quat, const FLOAT* Mins, const FLOAT *Ranges)
	{
		FromQuat( Quat, Mins, Ranges );
	}

	void FromQuat(const FQuat& Quat, const FLOAT* Mins, const FLOAT *Ranges)
	{
		FQuat Temp( Quat );
		if ( Temp.W < 0.f )
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		Temp.X -= Mins[0];
		Temp.Y -= Mins[1];
		Temp.Z -= Mins[2];

		const DWORD PackedX = (INT)((Temp.X / Ranges[0]) * Quant11BitFactor ) + Quant11BitOffs;
		const DWORD PackedY = (INT)((Temp.Y / Ranges[1]) * Quant11BitFactor ) + Quant11BitOffs;
		const DWORD PackedZ = (INT)((Temp.Z / Ranges[2]) * Quant10BitFactor ) + Quant10BitOffs;

		// 21-31 X, 10-20 Y, 0-9 Z.
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out, const FLOAT* Mins, const FLOAT *Ranges) const
	{
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		const DWORD ZMask = 0x000003ff;
		const DWORD YMask = 0x001ffc00;
		const DWORD XMask = 0xffe00000;

		const DWORD UnpackedX = Packed >> XShift;
		const DWORD UnpackedY = (Packed & YMask) >> YShift;
		const DWORD UnpackedZ = (Packed & ZMask);

		const FLOAT X = ( (((INT)UnpackedX - (INT)Quant11BitOffs) / Quant11BitDiv) * Ranges[0] + Mins[0] );
		const FLOAT Y = ( (((INT)UnpackedY - (INT)Quant11BitOffs) / Quant11BitDiv) * Ranges[1] + Mins[1] );
		const FLOAT Z = ( (((INT)UnpackedZ - (INT)Quant10BitOffs) / Quant10BitDiv) * Ranges[2] + Mins[2] );
		const FLOAT WSquared = 1.f - X*X - Y*Y - Z*Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? appSqrt( WSquared ) : 0.f;
	}

	friend FArchive& operator<<(FArchive& Ar, FQuatIntervalFixed32NoW& Quat)
	{
		Ar << Quat.Packed;
		return Ar;
	}
};

class FQuatFloat32NoW
{
public:
	DWORD Packed;

	FQuatFloat32NoW()
	{}

	explicit FQuatFloat32NoW(const FQuat& Quat)
	{
		FromQuat( Quat );
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp( Quat );
		if ( Temp.W < 0.f )
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		TFloatPacker<3, 7, TRUE> Packer7e3;
		TFloatPacker<3, 6, TRUE> Packer6e3;

		const DWORD PackedX = Packer7e3.Encode( Temp.X );
		const DWORD PackedY = Packer7e3.Encode( Temp.Y );
		const DWORD PackedZ = Packer6e3.Encode( Temp.Z );

		// 21-31 X, 10-20 Y, 0-9 Z.
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out) const
	{
		const DWORD XShift = 21;
		const DWORD YShift = 10;
		const DWORD ZMask = 0x000003ff;
		const DWORD YMask = 0x001ffc00;
		const DWORD XMask = 0xffe00000;

		const DWORD UnpackedX = Packed >> XShift;
		const DWORD UnpackedY = (Packed & YMask) >> YShift;
		const DWORD UnpackedZ = (Packed & ZMask);

		TFloatPacker<3, 7, TRUE> Packer7e3;
		TFloatPacker<3, 6, TRUE> Packer6e3;

		const FLOAT X = Packer7e3.Decode( UnpackedX );
		const FLOAT Y = Packer7e3.Decode( UnpackedY );
		const FLOAT Z = Packer6e3.Decode( UnpackedZ );
		const FLOAT WSquared = 1.f - X*X - Y*Y - Z*Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? appSqrt( WSquared ) : 0.f;
	}

	friend FArchive& operator<<(FArchive& Ar, FQuatFloat32NoW& Quat)
	{
		Ar << Quat.Packed;
		return Ar;
	}
};

#endif // __ANIMATIONCOMPRESSION_H__
