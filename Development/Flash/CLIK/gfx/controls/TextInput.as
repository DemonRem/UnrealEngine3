﻿/**
 * TextInput is an editable text field component used to capture textual user input. Similar to the Label, this component is merely a wrapper for a standard textField, and therefore supports the same capabilities of the textField such as password mode, maximum number of characters and HTML text. Only a handful of these properties are exposed by the component itself, while the rest can be modified by directly accessing the TextInput’s textField instance.
    
   The TextInput component should be used for input, since noneditable text can be displayed using the Label. Similar to the Label, developers may substitute standard textFields for TextInput components based on their requirements. However, when developing sophisticated UIs, especially for PC applications, the TextInput component provides valuable extended capabilities over the standard textField.
   
   For starters, TextInput supports the focused and disabled state, which are not easily achieved with the standard textField. Due to the separated focus state, TextInput can support custom focus indicators, which are not included with the standard textField. Complex AS2 code is required to change the visual style of a standard textField, while the TextInput visual style can be configured easily on the timeline. The TextInput inspectable properties provide an easy workflow for designers and programmers who are not familiar with Flash Studio. Developers can also easily listen for events fired by the TextInput to create custom behaviors. 

   The TextInput also supports the standard selection and cut, copy, and paste functionality provided by the textField, including multi paragraph HTML formatted text. By default, the keyboard commands are select (Shift+Arrows), cut (Shift+Delete), copy (Ctrl+Insert), and paste (Shift+Insert).


 	<b>Inspectable Properties</b>
	The inspectable properties of the TextInput component are:<ul>
	<li><i>text</i>: Sets the text of the textField.</li>
	<li><i>visible</i>: Hides the component if set to false.</li>
	<li><i>disabled</i>: Disables the component if set to true.</li>
	<li><i>editable</i>: Makes the TextInput non-editable if set to false.</li>
	<li><i>maxChars</i>: A number greater than zero limits the number of characters that can be entered in the textField.</li>
	<li><i>password</i>: If true, sets the textField to display '*' characters instead of the real characters. The value of the textField will be the real characters entered by the user – returned by the text property.</li>
	<li><i>defaultText</i>: Text to display when the textField is empty. This text is formatted by the defaultTextFormat object, which is by default set to light gray and italics.</li>
	<li><i>actAsButton</i>: If true, then the TextInput will behave similar to a Button when not focused and support rollOver and rollOut states. Once focused via mouse press or tab, the TextInput reverts to its normal mode until focus is lost.</li>
    <li><i>enableInitCallback</i>: If set to true, _global.CLIK_loadCallback() will be fired when a component is loaded and _global.CLIK_unloadCallback will be called when the component is unloaded. These methods receive the instance name, target path, and a reference the component as parameters.  _global.CLIK_loadCallback and _global.CLIK_unloadCallback should be overriden from the game engine using GFx FunctionObjects.</li>
	<li><i>soundMap</i>: Mapping between events and sound process. When an event is fired, the associated sound process will be fired via _global.gfxProcessSound, which should be overriden from the game engine using GFx FunctionObjects.</li></ul>
		
	<b>States</b>
	The CLIK TextInput component supports three states based on its focused and disabled properties. <ul>
	<li>default or enabled state.</li>
	<li>focused state, typically a represented by a highlighted border around the textField.</li>
	<li>disabled state.</li></ul>
	
	<b>Events</b>
	All event callbacks receive a single Object parameter that contains relevant information about the event. The following properties are common to all events. <ul>
	<li><i>type</i>: The event type.</li>
	<li><i>target</i>: The target that generated the event.</li></ul>
		
	The events generated by the TextInput component are listed below. The properties listed next to the event are provided in addition to the common properties.<ul>
	<li><i>show</i>: The component’s visible property has been set to true at runtime.</li>
	<li><i>hide</i>: The component’s visible property has been set to false at runtime.</li>
	<li><i>focusIn</i>: The component has received focus.</li>
	<li><i>focusOut</i>: The component has lost focus.</li>
	<li><i>textChange</i>: The text field contents have changed.</li>
	<li><i>rollOver</i>: The mouse cursor has rolled over the component when not focused. Only fired when the actAsButton property is set.<ul>
	<li><i>controllerIdx</i>: The index of the mouse cursor used to generate the event (applicable only for multi-mouse-cursor environments). Number type. Values 0 to 3.</li></ul></li>
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

/*
*/

import flash.external.ExternalInterface; 
import gfx.core.UIComponent;
import gfx.ui.InputDetails;
import gfx.ui.NavigationCode;
import gfx.utils.Constraints;
import gfx.utils.Locale;
import System.capabilities;

[InspectableList("disabled", "visible", "textID", "password", "maxChars", /*"restrict",*/ "editable", "actAsButton", "defaultText", "enableInitCallback", "soundMap", "bTriggerSoundsOnEvents")]
class gfx.controls.TextInput extends UIComponent {
	
// Constants:

// Public Properties:
	/** The default text to be shown when no text has been assigned or entered into this component. */
	[Inspectable(verbose=1)]
	public var defaultText:String = "";
	/** The text format used to display the default text. By default it is set to color:0xAAAAAA and italic:true. */
	public var defaultTextFormat:TextFormat;
    /** Mapping between events and sound process */
    [Inspectable(type="Object", defaultValue="theme:default,focusIn:focusIn,focusOut:focusOut,textChange:textChange")]
    public var soundMap:Object = { theme:"default", focusIn:"focusIn", focusOut:"focusOut", textChange:"textChange" };

// Private Properties:
	private var _text:String = "";
	private var _password:Boolean;
	private var _maxChars:Number = 0
	//private var _restrict:String;
	private var _editable:Boolean = true;
	private var _selectable:Boolean;	
	private var isHtml:Boolean;
	private var constraints:Constraints;
	[Inspectable(defaultValue="false", verbose=1)]
	private var actAsButton:Boolean = false;
	private var hscroll:Number = 0;
	private var changeLock:Boolean = false;
	
// UI Elements
	/** A reference the on-stage TextField instance. Note that when state changes are made, the textField instance may change, so changes made to it externally may be lost. */
	public var textField:TextField;
	

// Initialization:
	/**
	 * The constructor is called when a TextInput or a sub-class of TextInput is instantiated on stage or by using {@code attachMovie()} in ActionScript. This component can <b>not</b> be instantiated using {@code new} syntax. When creating new components that extend TextInput, ensure that a {@code super()} call is made first in the constructor.
	 */
	public function TextInput() { 
		super(); 
		tabEnabled = !_disabled;
		focusEnabled = !_disabled;
		// Create a custom text format for the empty state (defaultTextFormat), which can be overridden by the user.
		defaultTextFormat = textField.getNewTextFormat();
		defaultTextFormat.italic = true;
		defaultTextFormat.color = 0xAAAAAA;
	}

// Public Methods:
	/** 
	 * Set the {@code text} parameter of the component using the Locale class to look up a localized version of the text from the Game Engine. This property can be set with ActionScript, and is used when the text is set using the Component Inspector.
	 */
	[Inspectable(name="text")]
	public function get textID():String { return null; }
	public function set textID(value:String):Void {
		if (value != "") {
			text = Locale.getTranslatedString(value);
		}
	}
	/**
	 * Get and set the text of the component using ActionScript. The {@code text} property can only set plain text. For formatted text, use the {@code htmlText} property, or set {@code html=true}, and use {@code TextFormat} instead.
	 */
	public function get text():String { return _text; }
	public function set text(value:String) {
		_text = value;
		isHtml = false;
		updateText();
	}
	
	/**
	 * Get and set the html text of the component. Html text can be formatted using the tags supported by ActionScript.
	 */
	public function get htmlText():String { return _text; }
	public function set htmlText(value:String) {
		_text = value;
		isHtml = true;
		updateText();
	}
	
	/**
	 * Determines if text can be entered into the TextArea, or if it is display-only. Text in a non-editable TextInput components can not be selected.
	 */
	[Inspectable(defaultValue="true")]
	public function get editable():Boolean { return _editable; }
	public function set editable(value:Boolean):Void {
		_editable = value;	
		tabEnabled = !_disabled && !_editable;
		updateTextField();
	}
	
	/**
	 * The "password" mode of the text field. When {@code true}, the component will show asterisks instead of the typed letters.
	 */
	[Inspectable(defaultValue="false")]
	public function get password():Boolean { return textField.password; }
	public function set password(value:Boolean):Void {
		_password = textField.password = value;
	}
	
	/**
	 * The maximum number of characters that the field can contain.
	 */
	[Inspectable(defaultValue="0")]
	public function get maxChars():Number { return _maxChars; }
	public function set maxChars(value:Number):Void {
		_maxChars = textField.maxChars = value;
	}
	
	/*
	//
	// A string of the characters the the textField can contain. Characters not included can not be entered into the TextField using the keyboard, only ActionScript.  Set to an empty string or null to reset to all characters.
	//
	[Inspectable(defaultValue="")]
	public function get restrict():String { return _restrict; }
	public function set restrict(value:String):Void {
		_restrict = textField.restrict = value;
	}
	*/
	
	/**
	 * Disable this component. Focus (along with keyboard events) and mouse events will be suppressed if disabled.
	 */	
	[Inspectable(defaultValue="false", verbose="1")]
	public function get disabled():Boolean { return _disabled; }
	public function set disabled(value:Boolean):Void { 
		super.disabled = value;
		tabEnabled = !_disabled;
		focusEnabled = !_disabled;
		if (initialized) {
			setMouseHandlers();
			setState();
			updateTextField();
		}
	}	
	
	/**
	 * Append a new string to the existing text. The textField will be set to non-html rendering when this method is invoked.
	 */	
	public function appendText(text:String):Void {
		_text += text;
		if (isHtml) { textField.html = false; }
		isHtml = false;		
		textField.appendText(text);
	}
		
	/**
	 * Append a new html string to the existing text. The textField will be set to html rendering when this method is invoked.
	 */	
	public function appendHtml(text:String):Void {
		_text += text;
		if (!isHtml) { textField.html = true; }
		isHtml = true;
		textField.appendHtml(text);
	}
	
	/**
	 * The length of the text in the textField.
	 */
	public function get length():Number { return textField.length; }
		
	public function handleInput(details:InputDetails, pathToFocus:Array):Boolean {
		if (details.value != "keyDown" && details.value != "keyHold") { return false; }
		var controllerIdx:Number = details.controllerIdx;
		if (Selection.getFocus(controllerIdx) != null) { return false; }
		Selection.setFocus(textField, controllerIdx);
		return true;
	}			
			
	/** @exclude */
	public function toString():String {
		return "[Scaleform TextInput " + _name + "]";
	}
	
	
// Private Methods:
	private function configUI():Void {
		super.configUI();
		
		constraints = new Constraints(this, true);
		constraints.addElement(textField, Constraints.ALL);
		
		setState();
		updateTextField();
		
		setMouseHandlers();
	}
	
	private function setState():Void {
		gotoAndPlay(_disabled ? "disabled" : (_focused ? "focused" : "default"));
	}
	
	// Switch on/off the mouse handlers to support rollover/rollout behavior in addition to text input
	private function setMouseHandlers():Void {
		if (actAsButton == false) { return; }
		if (_disabled || _focused) {
			delete onRollOver;
			delete onRollOut;
			delete onPress;
		} else if (_editable) {
			onRollOver = handleMouseRollOver;
			onRollOut = handleMouseRollOut;
			onPress = handleMousePress;	
		}
	}
	
	// Give the component focus on mouse press
	private function handleMousePress(controllerIdx:Number, keyboardOrMouse:Number, button:Number):Void {
		dispatchEvent( { type:"press", controllerIdx:controllerIdx, button:button } );
		Selection.setFocus(this.textField, controllerIdx);
	}
	
	// Play the rollover animation if one exists (the "over" keyframe)
	private function handleMouseRollOver(controllerIdx:Number):Void {
		gotoAndPlay("default");
		gotoAndPlay("over");
		if (constraints) {	constraints.update(__width, __height); }		
		updateTextField();
		dispatchEvent({type:"rollOver", controllerIdx:controllerIdx});
	}
	
	// Play the rollout animation if one exists (the "out" keyframe)
	private function handleMouseRollOut(controllerIdx:Number):Void {
		gotoAndPlay("default");
		gotoAndPlay("out");
		if (constraints) {	constraints.update(__width, __height); }		
		updateTextField();		
		dispatchEvent({type:"rollOut", controllerIdx:controllerIdx});
	}
	
	private function draw():Void {
		if (sizeIsInvalid) { 
			_width = __width;
			_height = __height;
		}
		super.draw();
		constraints.update(__width, __height);
	}
	
	private function changeFocus():Void {
		tabEnabled = !_disabled;
		if (!_focused) { hscroll = textField.hscroll; }
		setState();
		if (constraints) {	constraints.update(__width, __height); }
		updateTextField();
		// Support Selection.setFocus on this component
		// (handoff to the textField if not focused)
		if (_focused && textField.type == "input") {
			tabEnabled = false;
			var controllerMask:Number = Selection.getFocusBitmask(this);
			for (var i:Number = 0; i < capabilities["numControllers"]; i++) {
				if ( ((controllerMask >> i) & 0x1) != 0 ) {
					Selection.setFocus( textField, i);
					// Select all text (to mimic single line textField behavior)
					Selection.setSelection(0, textField.htmlText.length, i);
				}
			}
		}		
		
		setMouseHandlers();
		// hscroll needs to be reset after losing focus
		textField.hscroll = hscroll;		
	}
	
	// Update the textField content
	private function updateText():Void {
		if (_text != "") {
			if (isHtml) { 
				textField.html = true;
				textField.htmlText = _text;
			} else {
				textField.html = false;
				textField.text = _text;
			}
		} else {
			textField.text = "";
			if (!_focused && defaultText != "") {
				textField.text = defaultText;
				textField.setTextFormat(defaultTextFormat);
			}
		}
	}
	
	// Update the textField properties. Usually this just happens on a frame change.
	private function updateTextField():Void {
		if (textField != null) {
			if (!_selectable) { _selectable = textField.selectable; }
			updateText();
			//textField.restrict = _restrict;
			textField.maxChars = _maxChars;
			textField.password = _password;
			textField.selectable = _disabled ? false : (_selectable || _editable);
			textField.type = (_editable && !_disabled) ? "input" : "dynamic";
			textField["focusTarget"] = this;
			textField.hscroll = hscroll;
			textField.addListener(this);
		}		
	}
	
	// The text in the textField has changed. Store the new value in case the frame changes, and dispatch an event.	
	private function onChanged(target:Object):Void {
		if (!changeLock) {
			_text = isHtml ? textField.htmlText : textField.text;
			dispatchEventAndSound({type:"textChange"});			
		}
	}

}