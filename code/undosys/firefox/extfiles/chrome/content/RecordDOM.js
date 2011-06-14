
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

if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.RecordDOM = {

    init: function() {
	// Check for requirements
	if (typeof MD5 == "undefined") {
	    pdos.u.debug(FATAL, "RecordDOM requires MD5. Include md5.js before RecordDOM.js in the html file");
	}
	    
	/**
	 * Register web progress listener. We are interested in intercepting 
	 * document load when the state of the doc is START | REQUEST, since
	 * at that state the base JS engine for the doc is initialized, but
	 * scripts in the doc have not run yet
	 **/
	pdos.u.debug(VERBOSE, "initializing RecordDOM");
	var dls = pdos.u.CC['@mozilla.org/docloaderservice;1'].getService(pdos.u.CI.nsIWebProgress);
	dls.addProgressListener(this, pdos.u.CI.nsIWebProgress.NOTIFY_STATE_DOCUMENT |
				      pdos.u.CI.nsIWebProgress.NOTIFY_STATE_REQUEST);
	pdos.u.debug(VERBOSE, "done initializing RecordDOM");	
    },
        
    QueryInterface: function(aIID) {
	if(!aIID.equals(pdos.u.CI.nsISupports) && 
	    !aIID.equals(pdos.u.CI.nsISupportsWeakReference) &&
	    !aIID.equals(pdos.u.CI.nsIWebProgressListener)) {
	   
	    pdos.u.debug(WARN, "QueryInterface: throwing error");
	    throw pdos.u.CR.NS_ERROR_NO_INTERFACE;
	}
	 
	return this;
    },
    
    onStateChange: function(wp, req, stateFlag, status) {
	try {
	    pdos.u.debug(VVERBOSE, "RecordDOM.onState change called for window: " +
			 wp.DOMWindow.location + " : " + pdos.u.wpStateToString(stateFlag));
	    
	    /**
	     * We wait until stateFlag is STATE_START | STATE_IS_REQUEST to
	     * intercept, since changes made to DOMWindow before then do not
	     * stick and won't be visible to the client HTML code, and after
	     * that the client window's HTML Javascript begins to run.
	     **/
	    var st = pdos.u.START | pdos.u.IS_REQUEST;
	    if (wp.DOMWindow &&
		!(wp.DOMWindow instanceof pdos.u.CI.nsIDOMChromeWindow) &&
		(stateFlag & st) == st) {

		var win = wp.DOMWindow.wrappedJSObject;
		if (win.intercepted) {
		    pdos.u.debug(VERBOSE, "already intercepted...");
		    return;
		}
		
		pdos.u.debug(VERBOSE, "intercepting window: " + win.location);
		this.intercept(win);
		pdos.u.debug(VERBOSE, "done intercepting window: " + win.location);
		win.intercepted = true;
	    }
	} catch (e) {
	    pdos.u.debug(WARN, "caught exception: " + e);
	}
    },
	
    intercept: function(win) {
	win.Math.random = function() {
	    var ret = Math.random.call(win.Math);
	    log.record({evType: "random", value: ret}, win);
	    return ret;
	};
    
	win.Date = function(t) {
	    var ret;
    
	    if (typeof t === "undefined") {
		ret = new Date();
		log.record({evType: "date", value: ret.valueOf()}, win);
	    } else {
		ret = new Date(t);
	    }
    
	    return ret;
	};
    
	// XXX: rethink using MD5 of function toString as the function signature
	function computeMD5(cb) {
	    if (typeof cb == "function") {
		return MD5(cb.toString())
		// XXX: fix this - funcToString does not seem to be defined
		//return MD5(funcToString.call(cb));
	    } else if (typeof cb == "string") {
		return MD5(cb);
	    } else if (typeof cb != "undefined" && typeof cb.toString != "undefined") {
		return MD5(cb.toString());
	    } else {
		return "undefined";
	    }
	}
    
	function wrap(win, cb, type, cbMD5) {
	    pdos.u.debug(VERBOSE, "wrapping cb with MD5 " + cbMD5 +
			 " for type " + type + ":\n" + cb);
	    return function() {
		log.record({evType: type, cb: cbMD5}, win);
		pdos.u.evalInSandbox(win, cb);
	    };
	}
	
	win.setTimeout = function(cb, timeout) {
	    if (typeof timeout != "number") {
		pdos.u.debug(WARN, "timeout is not a number. making it 0. cb =\n" + cb);
		timeout = 1;
	    }
	    var cbMD5 = computeMD5(cb); 
	    var mycb =  wrap(win, cb, "setTimeoutCall", cbMD5);
	    var ret = setTimeout(mycb, timeout);
	    log.record({evType: "setTimeout", cb: cbMD5, timeout: timeout, ret: ret}, win);
	    
	    return ret;
	};
    
	win.clearTimeout = function(id) {
	    log.record({evType: "clearTimeout", id: id}, win);
	    clearTimeout(id);
	};
	
	win.setInterval = function(cb, interval) {
	    if (typeof interval != "number") {
		pdos.u.debug(WARN, "interval is not a number. making it 0. cb =\n" + cb);
		interval = 1;
	    }
	    var cbMD5 = computeMD5(cb); 
	    var mycb =  wrap(win, cb, "setIntervalCall", cbMD5);
	    var ret = setInterval(mycb, interval);
	    log.record({evType: "setInterval", cb: cbMD5, interval: interval, ret: ret}, win);
	    
	    return ret;
	};
    
	win.clearInterval = function(id) {
	    log.record({evType: "clearInterval", id: id}, win);
	    clearInterval(id);
	};
    
	// pathFromRoot only accesses ownerDocument and parentNode fields for
	// DOM nodes, which are read-only values. Hence this access is
	// safe and does not need a XPCNativeWrapper
	// XXX: this is not correct, since all fields can be overridden using
	// setter and getter functions, thereby executing arbitrary functions
	// when these fields are accessed. So, we still need XPCNativeWrapper
	function pathFromRoot(node) {
	    if (node === null) {
		return "null";
	    }
	    
	    // If ownerDocument is null, then node is the document itself
	    if (node.ownerDocument == null) {
		return "";
	    }
	    
	    if (typeof node.parentNode === "undefined" || node.parentNode == null) {
		pdos.u.debug(WARN, "no parentNode for: " + node);
		return "null";
	    }
	    
	    parent = node.parentNode;
	    for (var i in parent.childNodes) {
		if (parent.childNodes[i] == node) {
		    var parentPath = pathFromRoot(parent);
		    if (parentPath != "")
			parentPath += ":";
		    return parentPath + i;
		}
	    }
    
	    pdos.u.debug(FATAL, "error: cannot find path from root for node: " + node);
	    
	    // Dead code since debug(FATAL) throws an exception. This is there
	    // so KomodoEdit doesn't complain that function doesn't always
	    // return value.
	    return ""; 
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
		// Record all user-generated events. Don't need to record
		// app-generated events, since those are not
		// non-deterministic. The isTrusted property is a Firefox
		// specific property that indicates whether it is a
		// user-generated event.
		if (!e.isTrusted)
		    return;
    
		if (typeof pdos.u.events[e.type] == "undefined")
		    return;
    
		// Ignore Mutation events
		if (pdos.u.events[e.type] == "MutationEvent")
		    return;
		
		var logEntry = {evType: "event"};
		for (var p in pdos.u.eventPropertiesToLog) {
		    if (typeof e[p] != "undefined" && e[p] != null &&
			e[p] != pdos.u.eventPropertiesToLog[p]) {
			logEntry[p] = e[p];
		    }
		}
    
		// XXX: Use ids for DOM nodes that have ids, but since
		// multiple nodes can have the same id, make sure that the
		// id maps to the same DOM node (by doing a
		// getElementById) before using id instead of path from
		// document root.
		var target = e["target"].wrappedJSObject;
    
		// XXX: do we need to do anything with relatedTarget or
		// originalTarget?
		logEntry["target"] = pathFromRoot(target);
		log.record(logEntry, win);
	    }
	    
	    for(var i in pdos.u.events) {
		if (pdos.u.events[i] == "DragEvent" ||
		    pdos.u.events[i] == "KeyboardEvent" ||
		    pdos.u.events[i] == "MouseEvent") {
		    
		    win.addEventListener(i, handler, true);
		}
	    }
	}
    
	registerEventHandlers();
    }
};

if (pdos.u.mode() == "record") {
    pdos.RecordDOM.init();
}
