
var ev = {
    // From: http://help.dottoro.com/ljfvvdnm.php and
    // http://help.dottoro.com/ljojnwud.php
    // Only events supported by Firefox's init*Event methods
    events : {
        "drag"       : "DragEvent",
        "dragend"    : "DragEvent",
        "dragenter"  : "DragEvent",
        "dragexit"   : "DragEvent",
        "draggesture": "DragEvent",
        "dragleave"  : "DragEvent",
        "dragover"   : "DragEvent",
        "dragstart"  : "DragEvent",
        "drop"       : "DragEvent",

        // Basic events: initEvent
        "beforeunload" : "Event",
        "blur"         : "Event",
        "bounce"       : "Event",      
        "change"       : "Event",      
        "CheckboxStateChange": "Event",
        "copy"         : "Event",
        "cut"          : "Event",
        "error"        : "Event",
        "finish"       : "Event",
        "focus"        : "Event",
        "hashchange"   : "Event",
        "input"        : "Event",
        "load"         : "Event",
        "RadioStateChange": "Event",
        "reset"        : "Event",
        "resize"       : "Event",
        "scroll"       : "Event",
        "select"       : "Event",
        "start"        : "Event",
        "submit"       : "Event",
        "unload"       : "Event",

        // Keyboard input events: initKeyEvent
        "keydown"  : "KeyboardEvent",
        "keypress" : "KeyboardEvent",
        "keyup"    : "KeyboardEvent",

        // Message events: initMessageEvent
        "message"  : "MessageEvent",

        // Mouse events: initMouseEvent
        "click"         : "MouseEvent",
        "contextmenu"   : "MouseEvent",
        "dblclick"      : "MouseEvent",
        "DOMMouseScroll": "MouseEvent",
        "drag"          : "MouseEvent",
        "dragdrop"      : "MouseEvent",
        "dragend"       : "MouseEvent",
        "dragenter"     : "MouseEvent",
        "dragexit"      : "MouseEvent",
        "draggesture"   : "MouseEvent",
        "dragover"      : "MouseEvent",
        "mousedown"     : "MouseEvent",
        "mousemove"     : "MouseEvent",
        "mouseout"      : "MouseEvent",
        "mouseover"     : "MouseEvent",
        "mouseup"       : "MouseEvent",

        // Mutation events: initMutationEvent
        "DOMAttrModified"         : "MutationEvent",
        "DOMCharacterDataModified": "MutationEvent",
        "DOMNodeInserted"         : "MutationEvent",
        "DOMNodeRemoved"          : "MutationEvent",
        "DOMSubtreeModified"      : "MutationEvent",

        // UI events: initUIEvent
        "DOMActivate" : "UIEvent",
        "overflow"    : "UIEvent",
        "underflow"   : "UIEvent"
    },

    /**
     * Event properties to record in the replay log, and their default values
     **/
    eventPropertiesToLog: {
        // Fields common to most events including DragEvents,
        // MouseEvents, and UIEvents
        "bubbles"       : true,
        "cancelable"    : true,
        "detail"        : 0,
        "screenX"       : 0,
        "screenY"       : 0,
        "clientX"       : 0,
        "clientY"       : 0,
        "ctrlKey"       : false,
        "altKey"        : false,
        "shiftKey"      : false,
        "metaKey"       : false,
        "button"        : 0,
        "dataTransfer"  : null,

        "timeStamp"     : 0,
        "type"          : "",

        // Unique to KeyboardEvents
        "keyCode"       : 0,
        "charCode"      : 0,

        // Unique to MessageEvents
        "data"          : null,
        "origin"        : "",
        "lastEventId"   : null,
        "source"        : null,

        // Unique to MutationEvents
        "relatedNode"   : {},
        "prevValue"     : "",
        "newValue"      : "",
        "attrName"      : "",
        "attrChange"    : 1
    },

    /**
     * Use preventDefault and stopPropagation to stop any other
     * event handlers from being called.
     *
     * Tried using preventBubble and preventCapture, but they
     * are deprecated functions and don't seem to work anyway.
     **/
    cancelEvent: function(e) {
        pdos.u.debug(VVVERBOSE, "preventing propagation of event: " + e.type);
        e.preventDefault();
        e.stopPropagation();
    }
};
