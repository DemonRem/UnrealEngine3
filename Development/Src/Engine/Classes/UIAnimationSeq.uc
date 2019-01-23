/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UIAnimationSeq extends UIAnimation
	native(UIPrivate);

/** The name of this sequence */
var name SeqName;

/** The default duration.  PlayUIAnimation can be used to override this */
var float SeqDuration;

/** Holds a list of Animation Tracks in this sequence */
var array<UIAnimTrack> Tracks;

/** If true, any positional changes made using this sequence will be considered as
    absolute position changes (ie: Move the Archor point to absolute pixel 0.1,0.1
	instead of moving it 0.1,0.1 pixels. */

var bool bAbsolutePositioning;

/**
 * Apply this animation.  Note, if LFI = NFI this will simply apply the
 * frame without any interpolation
 *
 * @Param TargetWidget		The Widget to apply the animation to
 * @Param Position			Where in the animation are we
 * @Param LFI				The Last Frame Index
 * @Param NFI				The Next Frame Index
 **/

native function ApplyAnimation(UIObject TargetWidget, INT TrackIndex, FLOAT Position, INT LFI, INT NFI, UIAnimSeqRef AnimRefInst );


defaultproperties
{
}
