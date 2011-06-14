/****************************************************
 * NOTE: this replayer was the test prototype intended
 * to work as a library. The newer prototype, which is
 * in ReplayDOM.js, and works as a firefox extension,
 * subsumes this code.
 ****************************************************/

// XXX:
// Replayer works as follows:
//
// 1. It cancels all the events, so no external events are sent to DOM
// elements
// 2. Only the synthetic events created by the replayer are sent -- the
// canceling function makes sure that events that are created by
// replayer are not canceled.
// 3. The replayer dispatches an event after the previous event has
// finished, and it does this by using the bubbling phase of the event.
// 4. If we want to have replayer specific buttons, we can ignore
// cancelling events to those buttons (by looking at the targets)

if (typeof pdos == "undefined")
    pdos = {};

if (window.location.toString().indexOf("action=replay") < 0) 
    pdos.replayer = {};

var log = JSON.parse(unescape(logstr));

var mylog = null;
var clearlog = function() {};
function defineMyLog() {
    if (mylog !=null)
	return;
    
    var logdiv = document.getElementById("logdiv");

    //alert(logdiv);
    if (logdiv != null) {
	mylog = function(msg) {
	    logdiv.innerHTML += msg + "<br>";
	};
	mylog("my log created");

	clearlog = function() {
	    logdiv.innerHTML = "";
	};
    }
}

defineMyLog();

function debug(msg) {
    if (typeof console != "undefined" && typeof console.log != "undefined")
	console.log(msg);
    else if (mylog != null)
	mylog(msg);
}

function trace() {
    if (typeof console != "undefined" && typeof console.trace != "undefined")
	console.trace();
    else if (mylog != null)
	mylog("trace");
}

pdos.replayer || function() {
    var orig = { random        : Math.random,
                 date          : Date,
                 setTimeout    : setTimeout,
                 clearTimeout  : clearTimeout,
                 setInterval   : setInterval,
                 clearInterval : clearInterval,
                 xhr           : XMLHttpRequest,
                 getSelection  : window.getSelection
    };
    
    var logIdx = 0;

    function checkEvent(actual, expected) {
        if (actual != expected) {
            throw new Error("replayer: error: event " + logIdx + " expected " + expected + " got " + actual);
        }
    }

    var random = [];
    var randomIdx = 0;
    var dates = [];
    var dateIdx = 0;
    var timeouts = [];
    var timeoutIdx = 0;
    for (var i in log) {
	if (log[i].evType == "setTimeout" || log[i].evType == "setInterval") 
	    timeouts.push(log[i]);
	if (log[i].evType == "random")
	    random.push(log[i]);
	if (log[i].evType == "date")
	    dates.push(log[i]);
    }
    
    Math.random = function() {
        var item = random[randomIdx++];
        checkEvent(item.evType, "random");
        return item.value;
    };

    Date = function(t) {
        if (typeof t === "undefined") {
            var item = dates[dateIdx++];
            checkEvent(item.evType, "date");
            return new orig.date(item.value);
        } else {
            return new orig.date(t);
        }
    };

    var callbacks = {};
    var callbackIds = {};
    function computeMD5(cb) {
        if (typeof cb != "undefined" && typeof cb.toString != "undefined")
            return MD5(cb.toString());
        else
            return "undefined";
    }

    setTimeout = function(cb, timeout) {
        var cbMD5 = computeMD5(cb);
	var item = timeouts[timeoutIdx++];
        checkEvent(item.evType, "setTimeout");
        if (item.cb != cbMD5 || item.timeout != timeout) {
            debug("setTimeout: error: expected (" + item.cb + ", " +
		  item.timeout + ") got (" + cbMD5 + ", " +
		  timeout + ")");
        }

        callbackIds[item.ret] = item.cb;
        callbacks[item.cb] = cb;
        return item.ret;
    };

    clearTimeout = function(id) {
//         var item = log[logIdx++];
//         checkEvent(item.evType, "clearTimeout");
//         if (item.id != id) {
//             debug("clearTimeout: error: expected " + item.id + " got " + id);
//         }

        var cb = callbackIds[id];
        delete callbacks[cb];
        delete callbackIds[id];
    };

    setInterval = function(cb, interval) {
        var cbMD5 = computeMD5(cb);
        var item = timeouts[timeoutIdx++];
        checkEvent(item.evType, "setInterval");
        if (item.cb != cbMD5 || item.interval != interval) {
            debug("setInterval: error: expected (" + item.cb + ", " +
		  item.interval + ") got (" + cbMD5 + ", " +
		  interval + ")");
        }

        callbackIds[item.ret] = item.cb;
        callbacks[item.cb] = cb;
        return item.ret;
    };

    clearInterval = function(id) {
//         var item = log[logIdx++];
//         checkEvent(item.evType, "clearInterval");
//         if (item.id != id) {
//             debug("clearInterval: error: expected " + item.id + " got " + id);
//         }

        var cb = callbackIds[id];
        delete callbacks[cb];
        delete callbackIds[id];
    };

    // From: http://help.dottoro.com/ljfvvdnm.php and
    // http://help.dottoro.com/ljojnwud.php
    // Only events supported by Firefox's init*Event methods
    var events = {
        // Drag events: initDragEvent
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
    };
    
    function nodeFromPathInternal(root, path_arr) {
        if (path_arr.length == 0)
            return root;

        var idx = path_arr.shift();
        if (idx >= root.childNodes.length)
            throw new Error("nodeFromPath: error at node " + root + " : idx = " + idx);

        return nodeFromPathInternal(root.childNodes[idx], path_arr);
    }

    function nodeFromPath(path) {
        if (path === null) {
            debug("path is null");
            trace();
        }

        debug("nodeFromPath: "+path);
        
        if (path === "null")
            return null;
        
        if (path === "window")
            return window;
        
        var path_arr = path.split(":");
        return nodeFromPathInternal(document, path_arr);
    }

    var currentEvent = null;
    var len = log.length;
    function dispatchNextEvent() {
	defineMyLog();
	clearlog();
	
        // Dispatch timeout/interval calls and non Mutation events
        while (true && logIdx < log.length) {
            if (log[logIdx].evType == "event" &&
                events[log[logIdx].type] != "MutationEvent")
                break;

            if (log[logIdx].evType == "setTimeoutCall" ||
                log[logIdx].evType == "setIntervalCall")
                break;

            //debug("dispatchNextEvent: ignoring log item: " + JSON.stringify(log[logIdx]));
            logIdx++;       
        }

	if (logIdx >= log.length)
	    return false;
	
        var item = log[logIdx++];
        debug("Processing event (" + (logIdx-1) + "/" + len + "): " + JSON.stringify(item));

        if (item.evType == "setTimeoutCall" || item.evType == "setIntervalCall") {
            var cb = callbacks[item.cb];
            var wrappedCb = function() {
                if (typeof cb == "function") {
                    cb();
                } else {
                    eval(cb);
                }

		debug("setTimeout/setInterval finished");
                //dispatchNextEvent();
            };

            orig.setTimeout.call(window, wrappedCb, 1);
	    orig.setTimeout.call(window, dispatchNextEvent, 10);
            return true;
        }

        // Log item is an event. Dispatch it based on type of event to
        // the right DOM element.
        // While dispatching it, make sure to set currentEvent to the
        // created event
        if (! (item.type in events)) 
            throw new Error("dispatchNextEvent: error: unknown event type: " + item.type);

        var evClass = events[item.type];
        var e = document.createEvent(evClass);

        // Properties to log and their default values
        var propsToLog = {
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
        };

        for (var p in propsToLog) {
            if (! (p in item)) {
                item[p] = propsToLog[p];
            }
        }
        
        var relatedTarget = null;
        if (item["relatedTarget"] != null && typeof item["relatedTarget"] != "undefined")
            relatedTarget = nodeFromPath(item.relatedTarget);

        if (evClass === "DragEvent") {
            e.initDragEvent(item.type, item.bubbles, item.cancelable, window, item.detail,
                            item.screenX, item.screenY, item.clientX, item.clientY, item.ctrlKey,
                            item.altKey, item.shiftKey, item.metaKey, item.button, relatedTarget,
                            item.dataTransfer);
        } else if (evClass === "Event") {
            e.initEvent(item.type, item.bubbles, item.cancelable);
        } else if (evClass === "KeyboardEvent") {
            e.initKeyEvent(item.type, item.bubbles, item.cancelable, window, item.ctrlKey,
                           item.altKey, item.shiftKey, item.metaKey, item.keyCode, item.charCode);
        } else if (evClass === "MessageEvent") {
            e.initMessageEvent(item.type, item.bubbles, item.cancelable, item.data,
                               item.origin, item.lastEventId, item.source);
        } else if (evClass === "MouseEvent") {
            e.initMouseEvent(item.type, item.bubbles, item.cancelable, window, item.detail,
                             item.screenX, item.screenY, item.clientX, item.clientY, item.ctrlKey,
                             item.altKey, item.shiftKey, item.metaKey, item.button, relatedTarget);
        } else if (evClass === "MutationEvent") {
            e.initMutationEvent(item.type, item.bubbles, item.cancelable, item.relatedNode,
                                item.prevValue, item.newValue, item.attrName, item.attrChange);
        } else if (evClass === "UIEvent") {
            e.initUIEvent(item.type, item.bubbles, item.cancelable, window, item.detail);
        } 

        currentEvent = e;
	var node = nodeFromPath(item.target);
	debug("dispatching event: " + e.type + " to...");
	debug(node);
        node.dispatchEvent(e);

	orig.setTimeout.call(window, dispatchNextEvent, 10);	
	return true;
    }

    function registerEventHandlers() {
        function isEventLogged(type) {
            for (var i in events) {
                if (type == i && events[i] != "MutationEvent")
                    return true;
            }

            return false;
        }
        
        function captureHandler(e) {
            // Allow event with types that we don't log to pass
            // through.
            // XXX: we don't replay all events we log, so events array
            // should be updated with only the events that we actually
            // replay
	    // debug("captureHandler: " + e.type);
            if (!isEventLogged(e.type))
                return;

            // For the event types we log, only allow events
            // dispatched by replayer
            if (e !== currentEvent) {
		// For the replay button
		if (typeof e.target.id == "undefined" || e.target.id != "replay")
		    //debug("captureHandler: stopping propagation of " + e.type);
		    e.stopPropagation();
            }
// 	    else {
//                 currentEvent = null;
//             }
        }

	var started = false;
        function bubbleHandler(e) {
	    //debug("bubbleHandler: " + e.type);
            if (!isEventLogged(e.type))
                return;

	    debug("event finished");
	    if (e == currentEvent) {
		debug("currentEventdone");
		currentEvent = null;
		//dispatchNextEvent();
	    }
	    
// 	    if (!started) { 
// 		started = true;
// 		orig.setTimeout(dispatchNextEvent, 1000);
// 	    } else {
// 		dispatchNextEvent();
// 	    }
        }

        debug("registering event handlers");
        for(var i in events) {
            window.addEventListener(i, captureHandler, true);
            window.addEventListener(i, bubbleHandler, false);
        }
    }

    registerEventHandlers();
    //alert("registered event handlers");
    dispatchNextEvent();

    pdos.replayer = {
	"next": function() {
	    clearlog();
	    return dispatchNextEvent();
	}
    };

}();