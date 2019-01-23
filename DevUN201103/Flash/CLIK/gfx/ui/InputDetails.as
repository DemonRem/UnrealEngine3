﻿/**
 * An InputDetails object is generated by the InputDelegate, and contains relevant information about user input, such as controller buttons, keyboard keys, etc.
 */

/**********************************************************************
 Copyright (c) 2009 Scaleform Corporation. All Rights Reserved.

 Portions of the integration code is from Epic Games as identified by Perforce annotations.
 Copyright © 2010 Epic Games, Inc. All rights reserved.
 
 Licensees may use this file in accordance with the valid Scaleform
 License Agreement provided with the software. This file is provided 
 AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, 
 MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.
**********************************************************************/

class gfx.ui.InputDetails {
	
// Constants:

// Public Properties:
	/** The type of input */
	public var type:String;
	/** The code (if any) of the input, such as a keyboard key */
	public var code:Number;
	/** The value of the input, such as an analog button delta */
	public var value;
	/** The navigation equivalent of the input, such as "up", "down", "tab", etc. */
	public var navEquivalent:String;
	/** The index of the controller that generated the input event. */
	public var controllerIdx:Number;
	
// Private Properties:


// Initialization:
	/**
	 * Create a new InputDetails instance.
	 * @param type The type of input.
	 * @param code The code of the input, such as "key".
	 * @param navEquivalent The navigation equivalent.
	 */
	public function InputDetails(type:String, code:Number, value, navEquivalent:String, controllerIdx:Number) {
		this.type = type;
		this.code = code;
		this.value = value;
		this.navEquivalent = navEquivalent;
		this.controllerIdx = controllerIdx;
	}

// Public Methods:
	/** @exclude */
	public function toString():String {
		return ["[InputDelegate", "code="+code, "type="+type, "value="+value, "navEquivalent="+navEquivalent, "controllerIdx="+controllerIdx +"]"].toString();
	}
	
// Private Methods:

}