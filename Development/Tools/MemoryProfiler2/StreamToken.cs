/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.IO;

namespace MemoryProfiler2
{
    /**
     * The lower 2 bits of a pointer are piggy-bagged to store what kind of data follows it. This enum lists
     * the possible types.
     */
    public enum EProfilingPayloadType
    {
        TYPE_Malloc		= 0,
        TYPE_Free		= 1,
        TYPE_Realloc	= 2,
        TYPE_Other		= 3,
        // Don't add more than 4 values - we only have 2 bits to store this.
    }

    /**
     *  The the case of TYPE_Other, this enum determines the subtype of the token.
     */
    public enum EProfilingPayloadSubType
    {
		SUBTYPE_EndOfStreamMarker = 0,
		SUBTYPE_EndOfFileMarker = 1,
		SUBTYPE_SnapshotMarker = 2,
		SUBTYPE_FrameTimeMarker = 3,
		SUBTYPE_TextMarker = 4,

		SUBTYPE_TotalUsed = 5,
		SUBTYPE_TotalAllocated = 6,

		SUBTYPE_CPUUsed = 7,
		SUBTYPE_CPUSlack = 8,
		SUBTYPE_CPUWaste = 9,

		SUBTYPE_GPUUsed = 10,
		SUBTYPE_GPUSlack = 11,
		SUBTYPE_GPUWaste = 12,

		SUBTYPE_OSOverhead = 13,
		SUBTYPE_ImageSizeMarker = 14,
		SUBTYPE_Unknown,
    }

    /**
     * Variable sized token emitted by capture code. The parsing code ReadNextToken deals with this and updates
     * internal data. The calling code is responsible for only accessing member variables associated with the type.
     */
    public struct FStreamToken
    {
        /** Type of token */
        public EProfilingPayloadType Type;
        /** Subtype of token if it's of TYPE_Other */
        public EProfilingPayloadSubType SubType;
        /** Pointer in the caes of alloc/ free */
        public UInt64 Pointer;
        /** Old pointer in the case of realloc */
        public UInt64 OldPointer;
        /** New pointer in the case of realloc */
        public UInt64 NewPointer;
        /** Index into callstack array */
        public Int32 CallStackIndex;
        /** Size of allocation in alloc/ realloc case */
        public Int32 Size;
        /** Payload if type is TYPE_Other. */
        public UInt32 Payload;
        /** Payload data if type is TYPE_Other and subtype is SUBTYPE_SnapshotMarker or SUBTYPE_TextMarker */
        public int TextIndex;
        /** Payload data if type is TYPE_Other and subtype is SUBTYPE_FrameTimeMarker */
        public float DeltaTime;

        /** 
         * Updates the token with data read from passed in stream and returns whether we've reached the end.
         */
        public bool ReadNextToken(BinaryReader BinaryStream)
        {
            bool bReachedEndOfStream = false;

			// Initialize to defaults.
			SubType = EProfilingPayloadSubType.SUBTYPE_Unknown;
			TextIndex = -1;

            // Read the pointer and convert to token type by looking at lowest 2 bits. Pointers are always
            // 4 byte aligned so need to clear them again after the conversion.
            Pointer = BinaryStream.ReadUInt64();
			Type = ( EProfilingPayloadType )( Pointer & 3UL );
            Pointer = Pointer & ~3UL;

            // Serialize based on toke type.
            switch (Type)
            {
                // Malloc
                case EProfilingPayloadType.TYPE_Malloc:
                    CallStackIndex = BinaryStream.ReadInt32();
                    Size = BinaryStream.ReadInt32();
                    break;
                // Free
                case EProfilingPayloadType.TYPE_Free:
                    break;
                // Realloc
                case EProfilingPayloadType.TYPE_Realloc:
                    OldPointer = Pointer;
                    NewPointer = BinaryStream.ReadUInt64();
                    CallStackIndex = BinaryStream.ReadInt32();
                    Size = BinaryStream.ReadInt32();
                    break;
                // Other
                case EProfilingPayloadType.TYPE_Other:
					SubType = ( EProfilingPayloadSubType )BinaryStream.ReadInt32();
                    Payload = BinaryStream.ReadUInt32();

                    // Read subtype.
					switch( SubType )
                    {
                        // End of stream!
                        case EProfilingPayloadSubType.SUBTYPE_EndOfStreamMarker:
                            bReachedEndOfStream = true;
                            break;
                        case EProfilingPayloadSubType.SUBTYPE_EndOfFileMarker:
                            break;
                        case EProfilingPayloadSubType.SUBTYPE_SnapshotMarker:
							TextIndex = (int) Payload;
							break;
                        case EProfilingPayloadSubType.SUBTYPE_FrameTimeMarker:
                            DeltaTime = BitConverter.ToSingle(System.BitConverter.GetBytes(Payload), 0);
                            break;
						case EProfilingPayloadSubType.SUBTYPE_TextMarker:
							TextIndex = (int) Payload;
							break;
						case EProfilingPayloadSubType.SUBTYPE_TotalUsed:
						case EProfilingPayloadSubType.SUBTYPE_TotalAllocated:
						case EProfilingPayloadSubType.SUBTYPE_CPUUsed:
						case EProfilingPayloadSubType.SUBTYPE_CPUSlack:
						case EProfilingPayloadSubType.SUBTYPE_CPUWaste:
						case EProfilingPayloadSubType.SUBTYPE_GPUUsed:
						case EProfilingPayloadSubType.SUBTYPE_GPUSlack:
						case EProfilingPayloadSubType.SUBTYPE_GPUWaste:
						case EProfilingPayloadSubType.SUBTYPE_OSOverhead:
						case EProfilingPayloadSubType.SUBTYPE_ImageSizeMarker:
							break;					  
						default:					  
                            throw new InvalidDataException();
                    }
                    break;
            }

            return !bReachedEndOfStream;
        }
    }
}
