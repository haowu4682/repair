
/****************************************************
 * NOTE: this recorder was the test prototype intended
 * to work as a library. The newer prototype, which is
 * in RecordDOM.js, and works as a firefox extension,
 * subsumes this code.
 ****************************************************/

/* TODO:
   
 * 0. Name: revive, renovate, rejuvenate, total recall, flashback,
 *    refresh, retrospect, dejavu, webspy, witness,
 * 1. Compress log entries (e.g., by using binary format)
 * 2. How do you name objects, DOM elements, callbacks in setTimeout?
 * 3. on setTimeout etc., we need to wrap in our own cb that has a
 *    hidden id that is logged when cb is called
 * 4. How do you make it invisible to scripts that they are not
 *    running in a sandbox?
 *    * Don't have any visible names (e.g. pdos.recorder)
 *    * But maybe JS code in sandbox may still be able to do
 *      MD5 sum of functions to see whether they are being
 *      interrupted.
 * 5. During undo replay, how do you deal with missing DOM elements,
 *    missing callbacks etc.,?
 * 6. Right now log is only a sequence of events without times. This
 *    is fine since JS is single threaded and non-preemptible. Maybe
 *    recording times will also be helpful.
 * 7. Override toString of the wrapped Math.random etc., so it returns
 *    native code.
 * 8. Deal with message events. Also, look at http://help.dottoro.com/ljogqtqm.php
 *    for all events that you still need to deal with. That page also helps
 *    with replaying the events
 * 9. Deal with DOM nodes with no ids
 *10. Implement recording of XHR events
 *11. Move events, propsToLog, debug to a common file
 *12. Deal with text selection
 *
 * Ways in which attacker can subvert:
 * * Detect that his code is running under our recorder and do
     something else
 *   * But recording env will be the same as the replay env, so the code
       will do the same stuff during replay as during recording
 *   * Can override toString for the wrapped functions
 * * Redefine toString for callback functions, so it returns the same value
 */

if (typeof pdos == "undefined")
    pdos = {};

if (window.location.toString().indexOf("action=record") < 0) 
    pdos.recorder = {};

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

pdos.recorder || function() {
    // Check for requirements
    if (typeof MD5 == "undefined") 
        throw new Error("recorder requires MD5. Include md5.js first in the html file");
    
    var orig = { random        : Math.random,
                 date          : Date,
                 setTimeout    : setTimeout,
                 clearTimeout  : clearTimeout,
                 setInterval   : setInterval,
                 clearInterval : clearInterval,
                 xhr           : XMLHttpRequest,
                 getSelection  : window.getSelection
    };

    // Each log entry is an object that has a mandatory type
    // field. The rest of the fields are dependent on the type of
    // log entry
    var log = [];

    var maxlen = 20000;
    var maxlenreached = false;
    function log_push(evt) {
	if (log.length < maxlen) {
	    log.push(evt);
	    return;
	}

	if (!maxlenreached) {
	    alert("max length of log exceeded");
	    maxlenreached = true;
	}
    }
    
    Math.random = function() {
        var ret = orig.random.call(Math);
        log_push({evType: "random", value: ret});
        return ret;
    };

    Date = function(t) {
        var ret;

        if (typeof t === "undefined") {
            ret = new orig.date();
            log_push({evType: "date", value: ret.getTime()});
        } else {
            ret = new orig.date(t);
        }

        return ret;
    };

    function computeMD5(cb) {
        if (typeof cb != "undefined" && typeof cb.toString != "undefined")
            return MD5(cb.toString());
        else
            return "undefined";
    }

    function wrap(cb, type, cbMD5) {
        return function() {
            log_push({evType: type, cb: cbMD5});
            if (typeof cb == "function") {
                cb();
            } else {
                eval(cb);
            }
        };
    }
    
    setTimeout = function(cb, timeout) {
        var cbMD5 = computeMD5(cb); 
        var mycb =  wrap(cb, "setTimeoutCall", cbMD5);
        var ret = orig.setTimeout.call(window, mycb, timeout);
        log_push({evType: "setTimeout", cb: cbMD5, timeout: timeout, ret: ret});
        
        return ret;
    };

    clearTimeout = function(id) {
        log_push({evType: "clearTimeout", id: id});
        orig.clearTimeout.call(window, id);
    };
    
    setInterval = function(cb, interval) {
        var cbMD5 = computeMD5(cb); 
        var mycb =  wrap(cb, "setIntervalCall", cbMD5);
        var ret = orig.setInterval.call(window, mycb, interval);
        log_push({evType: "setInterval", cb: cbMD5, interval: interval, ret: ret});
        
        return ret;
    };

    clearInterval = function(id) {
        log_push({evType: "clearInterval", id: id});
        orig.clearInterval.call(window, id);
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
    
    function pathFromRoot(node) {
	if (node === null)
	    return "null";
	
	if (node === window)
	    return "window";
	
        if (node === document)
            return "";

	if (typeof node.parentNode === "undefined") {
	    debug("no parentNode for: " + node);
	}
	
        parent = node.parentNode;
        for (var i in parent.childNodes) {
            if (parent.childNodes[i] === node) {
                var parentPath = pathFromRoot(parent);
                if (parentPath != "")
                    parentPath += ":";
                return parentPath + i;
            }
        }

        throw new Error("Shouldn't come here: " + node);
    }

    // XXX:
    //
    // 1. Testing seems to suggest that there is no unwanted
    // interaction between DOM0 and DOM2 events as suggested by Mugshot
    // paper (section 3.1.4).
    //
    // 2. Since a lot of events can be generated (especially for mouse
    // events), we need to figure out a way to compress the events either
    // by (i) a good log format or (ii) recording events only if there are
    // other handlers for it
    //
    // 3. How do we distinguish user generated events from those generated
    // by dispatchEvent()?
    // 
    // XXX: synthetic events seem to be generated by createEvent. So you
    // can interpose on createEvent or maybe on dispatchEvent and figure
    // out which events are generated by JS vs by the user
    //
    // XXX: See: https://developer.mozilla.org/en/DOM/document.createEvent
    // for specification on events, especially the W3C links at the bottom
    // of the page.
    function registerEventHandlers() {
        function handler(e) {
	    defineMyLog();
	    
            // Record all user-generated events. Don't need to record
            // app-generated events, since those are not
            // non-deterministic. The isTrusted property is a Firefox
            // specific property that indicates whether it is a
            // user-generated event.
            if (!e.isTrusted)
                return;

	    if (typeof events[e.type] == "undefined")
		return;

	    // Ignore Mutation events
	    if (events[e.type] == "MutationEvent")
		return;
	    
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

            var logEntry = {evType: "event"};
            for (var p in propsToLog) {
                if (typeof e[p] != "undefined" && e[p] != null && e[p] != propsToLog[p]) {
                    logEntry[p] = e[p];
                }
            }

            // XXX: Use ids for DOM nodes that have ids, but since
            // multiple nodes can have the same id, make sure that the
            // id maps to the same DOM node (by doing a
            // getElementById) before using id instead of path from
            // document root.
            logEntry["target"] = pathFromRoot(e["target"]);
	    for (var p in ["relatedTarget"]) {
		if (e[p] != null && typeof e[p] != "undefined" && e[p] != e["target"])
		    logEntry[p] = pathFromRoot(e[p]);
	    }

            log_push(logEntry);
        }
        
        for(var i in events) {
            window.addEventListener(i, handler, true);
        }
    }

    registerEventHandlers();

    pdos.recorder = {
	log: function() {
	    return log;
	},
	
        printLog: function() {
            if (!console || !console.log)
                return;
            
            for (var i in log) {
                debug(log[i]);
            }
        },

        submitLog: function() {
            debug("submitting Log...");
            var url = "http://pdos.csail.mit.edu/~rameshch/replay/log_results.php?" + Math.random();
    
            var xhr = new XMLHttpRequest();
            xhr.open("POST", url, true);
            var body = "var logstr = \"" + escape(JSON.stringify(log)) + "\";";
            xhr.setRequestHeader("Content-length", body.length);
            xhr.setRequestHeader("Content-type", "application/octet-stream");

            xhr.onreadystatechange = function(evt) {
                if (xhr.readyState == 4) {
                    debug("submitLog: status = "+xhr.status);
                    if (xhr.status == 200) 
                        debug("submitLog: responseText = "+xhr.responseText);
                }
            }
            
            xhr.sendAsBinary(body);
        }
    };
}();

