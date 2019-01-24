/**
 * The StagePanel derives from the Panel class and provides a single inspectable interface to define the SWF's stage mode. It is expected to be attached to the root level container, and is not intended to be composed inside other Panels.
 */

/**********************************************************************
 Copyright (c) 2010 Scaleform Corporation. All Rights Reserved.
 Licensees may use this file in accordance with the valid Scaleform
 License Agreement provided with the software. This file is provided 
 AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, 
 MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.
**********************************************************************/

import flash.geom.Rectangle;
import gfx.core.UIComponent;
import gfx.layout.Panel;

[InspectableList("scaleMode", "bConstrainNonPanels", "enableInitCallback", "bTriggerSoundsOnEvents")]
class gfx.layout.StagePanel extends UIComponent {
		
// Public Properties:
	/** The scale mode to set on the SWF */
	[Inspectable(type="String", enumeration="noScale,showAll,noBorder", defaultValue="noScale")]	 
	public var scaleMode:String = "noScale";
	/** true of MovieClips that are not Panels within the Stage panel should be constrained, false if they should be left as is. */
	[Inspectable(defaultValue="false")]
	public var bConstrainNonPanels:Boolean = false;
	
// Private Properties:	
	private var _offsets:Object;
	private var _orect:flash.geom.Rectangle;
	
	private var _elements:Array;
	
	private static var LEFT:Number = 1;	
	private static var TOP:Number = 1;	
	private static var CENTER:Number = 2;
	private static var RIGHT:Number = 4;
	private static var BOTTOM:Number = 4;
	private static var ALL:Number = LEFT | RIGHT;
	
// Initialization:
	public function StagePanel() {		
		super();
	}
	
	/** @exclude */
	public function toString():String {
		return "[Scaleform StagePanel " + _name + "]";
	}

// Private Methods:
	private function onResize():Void {
		validateNow();
	}
	
	private function configUI():Void {
		super.configUI();

		// *** Setup Stage
		_orect = Stage.originalRect;
		Stage.addListener(this);
		Stage.align 	= "C";
		Stage.scaleMode = scaleMode;
		// Compute stage offsets
		var orect:flash.geom.Rectangle = Stage.originalRect;
		_offsets = {left:this._x, right:orect.width - this._width - this._x, 
				    top:this._y, bottom:orect.height - this._height - this._y};

		// *** Introspect content
		_elements = [];
		var vconstrain:Number = 0;
		var hconstrain:Number = 0;
		// Find sub elements
		for (var i in this) {
			var obj:Object = this[i];
			
			// Child panel?
			if (obj instanceof Panel) {
				var panel:Panel = Panel(obj);
				switch (panel.hconstrain) {
					case "left": { hconstrain = StagePanel.LEFT; break; }
					case "center": { hconstrain = StagePanel.CENTER; break; }
					case "right": { hconstrain = StagePanel.RIGHT; break; }
					case "all": { hconstrain = StagePanel.ALL; break; }
				}
				switch (panel.vconstrain) {
					case "top": { vconstrain = StagePanel.TOP; break; }
					case "center": { vconstrain = StagePanel.CENTER; break; }
					case "bottom": { vconstrain = StagePanel.BOTTOM; break; }
					case "all": { vconstrain = StagePanel.ALL; break; }
				}
				addElement(obj, hconstrain, vconstrain);
			}
			
			// Other element?
			else if (obj instanceof MovieClip) {
				// Constrain mode for non-panel elements set via inspectable.
				if (bConstrainNonPanels)
				{
					addElement(obj, StagePanel.ALL, StagePanel.ALL);
				}
			}
		}
	}

	public function addElement(clip:Object, hconstrain:Number, vconstrain:Number):Void {
		if (clip == null) { return; }
		
		var w:Number = Stage.originalRect.width;
		var h:Number = Stage.originalRect.height;
		
		var element:Object = {
			clip:clip,
			hconstrain:hconstrain,
			vconstrain:vconstrain,
			metrics:{
				left:clip._x, 
				top:clip._y, 
				right:w - (clip._x + clip._width), 
				bottom:h - (clip._y + clip._height),
				xscale:clip._xscale,
				yscale:clip._yscale
			}
		}
		_elements.push(element);
	}

	private function draw():Void {
		var vr:flash.geom.Rectangle = Stage.visibleRect;
		var cw:Number = vr.width - _offsets.right - _offsets.left;
		var ch:Number = vr.height - _offsets.bottom - _offsets.top;
		
		super.draw();
		
		processAlignment();
	}
	
	private function processAlignment():Void {
		// Stage center alignment is in effect
		var origRect:Rectangle = Stage.originalRect;
		var visRect:Rectangle = Stage.visibleRect;
		var vwidth = visRect.width;
		var vheight = visRect.height;
		var hoffset:Number = (origRect.width - vwidth) / 2;
		var voffset:Number = (origRect.height - vheight) / 2;
		for (var i:Number = 0; i < _elements.length; i++) {
			var element:Object = _elements[i];
			var clip:MovieClip = MovieClip(element.clip);
			var metrics:Object = element.metrics;
			var w:String = clip.width ? "width" : "_width";
			var h:String = clip.height ? "height" : "_height";
			
			var leftPercShift:Number = 0;
			var rightPercShift:Number = 0;
			var topPercShift:Number = 0;
			var bottomPercShift:Number = 0;
			var bUsePercentWidth:Boolean = clip.bUsePercentWidth;			
			if ( bUsePercentWidth )
			{
				var wperc:Number = (vwidth / origRect.width);
				if ( wperc < 1 ) // If the visible rect is smaller than the original rect.
				{
					var t_wperc:Number = clip["wPercentage"];	// Use the user defined value in the inspectable for the panel if it is valid.
					if ( t_wperc != undefined && t_wperc != 0 )	
					{
						wperc = (t_wperc / 100);
					}
				}
				
				leftPercShift = ( wperc - 1 ) * metrics.left;
				rightPercShift = ( wperc - 1 ) * metrics.right;		
			}
			
			var bUsePercentHeight:Boolean = clip.bUsePercentHeight;
			if ( bUsePercentHeight )
			{
				var hperc:Number =  vheight / origRect.height;
				if ( hperc < 1 )
				{
					var t_hperc:Number = clip["hPercentage"];
					if ( t_hperc != undefined || t_hperc != 0 )
					{
						hperc = (t_hperc / 100);
					}
				}
				
				topPercShift = ( hperc - 1 ) * metrics.top;
				bottomPercShift = ( hperc - 1  ) * metrics.bottom;
			}
			
			// Horizontal alignment
			switch (element.hconstrain) {
				case StagePanel.LEFT: { 	clip._x = (metrics.left) + hoffset + leftPercShift; 	break; }
				case StagePanel.RIGHT: {	clip._x = (metrics.left) - hoffset - rightPercShift; 	break; }
				case StagePanel.ALL: {		
					clip._x = metrics.left + hoffset;
					clip[w] = vwidth - metrics.left - metrics.right;
					break;
				}
				// Stage center alignent takes care of CENTER case automatically
			}
			// Vertical alignment
			switch (element.vconstrain) {
				case StagePanel.TOP: {		clip._y = (metrics.top) + voffset + topPercShift;		break; }
				case StagePanel.BOTTOM: {	clip._y = (metrics.top) - voffset - bottomPercShift;	break; }
				case StagePanel.ALL: {
					clip._y = metrics.top + voffset;
					clip[h] = vheight - metrics.top - metrics.bottom;
					break;
				}
				// Stage center alignent takes care of CENTER case automatically
			}
		}
	}
}
