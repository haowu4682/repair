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

if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.ReplayDOM = {
    // Per page replay logs and callbacks
    random   : {}, 
    dates    : {},
    timeouts : {},
    callbacks: {},
    callbackIds: {},
    
    log      : [],   // setTimeoutCall, setIntervalCall, and event entries for all pages
    
    /**
     * The init, QueryInterface and onStateChange functions are quite similar
     * to those in pdos.RecordDOM
     **/
    init: function() {
	// Check for requirements
	if (typeof MD5 == "undefined") {
	    pdos.u.debug(FATAL, "ReplayDOM requires MD5. Include md5.js before ReplayDOM.js in the html file");
	}
	
	// Add replay log filter
	pdos.u.addReplayLogFilter(this.logFilter);	
	    
	/**
	 * Register web progress listener. We are interested in intercepting 
	 * document load when the state of the doc is START | REQUEST, since
	 * at that state the base JS engine for the doc is initialized, but
	 * scripts in the doc have not run yet
	 **/
	pdos.u.debug(VERBOSE, "initializing ReplayDOM");
	var dls = pdos.u.CC['@mozilla.org/docloaderservice;1'].getService(pdos.u.CI.nsIWebProgress);
	dls.addProgressListener(this, pdos.u.CI.nsIWebProgress.NOTIFY_STATE_DOCUMENT |
				      pdos.u.CI.nsIWebProgress.NOTIFY_STATE_REQUEST);
	pdos.u.debug(VERBOSE, "done initializing ReplayDOM");
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
    
    logFilter: function(replayLog) {
	function getArrayForPage(arr, pageId) {
	    var ret = arr[pageId];
	    if (typeof ret == "undefined") {
		arr[pageId] = [];
	    }
	    
	    return arr[pageId];
	}
    
	for (var i in replayLog) {
	    if (replayLog[i].evType == "setTimeout" ||
		replayLog[i].evType == "setInterval") {
		getArrayForPage(pdos.ReplayDOM.timeouts,
				replayLog[i].pageId).push(replayLog[i]);
	    }
	    
	    if (replayLog[i].evType == "random") {
		getArrayForPage(pdos.ReplayDOM.random,
				replayLog[i].pageId).push(replayLog[i]);
	    }
	    
	    if (replayLog[i].evType == "date") {
		getArrayForPage(pdos.ReplayDOM.dates,
				replayLog[i].pageId).push(replayLog[i]);
	    }
	    
	    if (replayLog[i].evType == "setTimeoutCall" ||
		replayLog[i].evType == "setIntervalCall" ||
		replayLog[i].evType == "event") {
		pdos.ReplayDOM.log.push(replayLog[i])
	    }
	}
	
	pdos.u.debug(VVERBOSE, "logFilter: arrays:" +
		     "\ntimeouts: " + JSON.stringify(pdos.ReplayDOM.timeouts) +
		     "\nrandom: " + JSON.stringify(pdos.ReplayDOM.random) +
		     "\ndate: " + JSON.stringify(pdos.ReplayDOM.dates));
    },

    onStateChange: function(wp, req, stateFlag, status) {
	try {
	    pdos.u.debug(VVVERBOSE, "ReplayDOM.onState change called for window: " +
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
		    pdos.u.debug(VERBOSE, "already intercepted window: " + win.location);
		    return;
		}
		
		pdos.u.debug(VERBOSE, "intercepting window: " + win.location);
		this.intercept(win);
		pdos.u.readPageIdFromLog(win);
		pdos.u.debug(VERBOSE, "done intercepting window: " + win.location);
		win.intercepted = true;
	    }
	} catch (e) {
	    pdos.u.debug(WARN, "caught exception: " + e + "\n" + e.stack);
	}
    },

    /**
     * Checks whether there are any more entries in the replay log. If pageId
     * is valid, logHasNext checks whether the particular page's replay log
     * (i.e. arr[pageId]) has more replay log entries. Otherwise, it checks
     * whether arr has more log entries.
     *
     * If the in-memory array is empty, the function reads log entries from
     * disk, as necessary. The function returns true if there are more log
     * entries in-memory or on disk. 
     **/
    logHasNext: function(arr, pageId) {
	while (true) {
	    // Are the logs for a particular page?
	    var test_arr = arr;
	    if (typeof pageId != "undefined" && pageId != null) {
		test_arr = arr[pageId];
	    }
	    
	    // Is the in-memory log empty?
	    if (typeof test_arr != "undefined" && test_arr.length > 0) {
		return true;
	    }
	    
	    // In-memory log is empty. Read from disk
	    if (!pdos.u.readReplayLog()) {
		// Reached end of on-disk logs
		return false;
	    }
	}
    },
    
    intercept: function(win) {

	pdos.u.debug(VERBOSE, "actually intercepting window: " + win.location);
	
	// Keep a trusted copy of pageId that was set by readPageIdFromLog. The
	// value in win.pageId is in "user" space
	var pageId = pdos.u.getPageId(win);
	if (typeof pageId == "undefined" || pageId == null) {
	    pdos.u.debug(FATAL, "undefined pageId for window");
	}
	
	function checkEvent(actual, expected) {
	    if (actual != expected) {
		pdos.u.debug(FATAL, "checkEvent failed: expected " + expected + " got " + actual);
	    }
	}
    
	win.Math.random = function() {
	    var arr = pdos.ReplayDOM.random;
	    if (!pdos.ReplayDOM.logHasNext(arr, pageId)) {
		pdos.u.debug(WARN, "error: no more recorded random numbers to return");
		return Math.random();
	    }
	    
	    var item = arr[pageId].shift();
	    checkEvent(item.evType, "random");
	    return item.value;
	};
    
	win.Date = function(t) {
	    var arr = pdos.ReplayDOM.dates;
	    if (typeof t === "undefined") {
		if (!pdos.ReplayDOM.logHasNext(arr, pageId)) {
		    pdos.u.debug(WARN, "error: no more recorded dates to return");
		    return new Date();
		}
		
		var item = arr[pageId].shift();
		checkEvent(item.evType, "date");
		return new Date(item.value);
	    } else {
		return new Date(t);
	    }
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
    
	function getMapForPage(arr, pageId) {
	    var ret = arr[pageId];
	    if (typeof ret == "undefined") {
		arr[pageId] = {};
	    }
	    
	    return arr[pageId];
	}
    
	win.setTimeout = function(cb, timeout) {
	    var arr = pdos.ReplayDOM.timeouts;
	    if (!pdos.ReplayDOM.logHasNext(arr, pageId)) {
		pdos.u.debug(WARN, "error: no more timeout events: " + pageId +
			     " : timeouts=\n" + JSON.stringify(pdos.ReplayDOM.timeouts));
		
		// XXX: instead do a normal setTimeout with a wrapped callback
		// that does evalInSandbox
		return -1;
	    }
	    
	    var cbMD5 = computeMD5(cb);
	    var item = arr[pageId].shift();
	    pdos.u.debug(VVERBOSE, "setTimeout called with cb: " + cbMD5 + " timeout " + timeout);
	    checkEvent(item.evType, "setTimeout");
	    if (item.cb != cbMD5 || item.timeout != timeout) {
		pdos.u.debug(WARN, "setTimeout: error: expected (" + item.cb +
			     ", " + item.timeout + ") got (" + cbMD5 + ", " +
			     timeout + ")");
	    }
    
	    pdos.u.debug(VVERBOSE, "setTimeout: inserting cb " + cbMD5 + " into cbtable");
	    getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[item.ret] = item.cb;
	    getMapForPage(pdos.ReplayDOM.callbacks, pageId)[item.cb] = cb;
	    return item.ret;
	};
    
	win.clearTimeout = function(id) {
	    pdos.u.debug(VVERBOSE, "clearTimeout: " + id);
	    var cb = getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[id];
	    delete getMapForPage(pdos.ReplayDOM.callbacks, pageId)[cb];
	    delete getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[id];
	};
    
	win.setInterval = function(cb, interval) {
	    var arr = pdos.ReplayDOM.timeouts;
	    if (!pdos.ReplayDOM.logHasNext(arr, pageId)) {
		pdos.u.debug(WARN, "error: no more timeout events: " + pageId +
			     " : timeouts=\n" + JSON.stringify(pdos.ReplayDOM.timeouts));
		
		// XXX: instead do a normal setInterval with a wrapped callback
		// that does evalInSandbox
		return -1;
	    }
	    
	    var cbMD5 = computeMD5(cb);
	    var item = arr[pageId].shift();
	    pdos.u.debug(VVERBOSE, "setInterval called with cb: " + cbMD5 + " interval " + interval);
	    checkEvent(item.evType, "setInterval");
	    if (item.cb != cbMD5 || item.interval != interval) {
		pdos.u.debug(WARN, "setInterval: error: expected (" + item.cb +
			     ", " + item.interval + ") got (" + cbMD5 + ", " +
			     interval + ")");
	    }
    
	    pdos.u.debug(VVERBOSE, "setInterval: inserting cb " + cbMD5 + " into cbtable");
	    getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[item.ret] = item.cb;
	    getMapForPage(pdos.ReplayDOM.callbacks, pageId)[item.cb] = cb;
	    return item.ret;
	};

	win.clearInterval = function(id) {
	    pdos.u.debug(VVERBOSE, "clearInterval: " + id);
	    var cb = getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[id];
	    delete getMapForPage(pdos.ReplayDOM.callbacks, pageId)[cb];
	    delete getMapForPage(pdos.ReplayDOM.callbackIds, pageId)[id];
	};
    	
	function registerEventHandlers() {
	    pdos.u.debug(VERBOSE, "registering event handlers");
	    function isEventLogged(type) {
		for (var i in pdos.u.events) {
		    //if (type == i && pdos.u.events[i] != "MutationEvent")
		    if (type == i && ((pdos.u.events[i] == "DragEvent" ) ||
				      (pdos.u.events[i] == "MouseEvent") ||
				      (pdos.u.events[i] == "KeyboardEvent")))
			return true;
		}
    
		return false;
	    }
	    
	    function captureHandler(e) {
		/**
		 * Screen all input events if replay is not going on. This
		 * avoids user accidentally modifying the web page from its
		 * initial state.
		 * 
		 * XXX: Ideally, we should screen all input events other than
		 * those that are sent as part of replay. However, since replay
		 * sends input events via Robot, they cannot be easily
		 * distinguished from user's input events. One way to do this
		 * is by the properties of the event, such as keycode or
		 * screen location, but since one event sent via Robot can
		 * induce several other events (at slightly different screen
		 * locations, for example), it is difficult to screen this way.
		 * Another way to screen is by using the target DOM element that
		 * should get the event; however, adjacent DOM elements may
		 * need to also get events, for example, in the case of
		 * mousemove and those events cannot be blocked.
		 **/
		if (!pdos.ReplayDOM.replaying && e.type != "beforeunload") {
		    // We allow beforeunload so that firefox doesn't give
		    // annoying dialog box when it is asked to Quit
		    pdos.u.cancelEvent(e);
		    return;
		}
		
		pdos.u.debug(VVVERBOSE, "captureHandler: got event of type " + e.type);
		if (e.isTrusted) {
		    pdos.u.debug(VVVERBOSE, "captureHandler: got trusted event. setting it as last event");
		    pdos.ReplayDOM.lastEvent = e;
		}
	    }
    
	    function bubbleHandler(e) {
		pdos.u.debug(VVVERBOSE, "bubbleHandler: got event of type " + e.type);
	    }
    
	    pdos.u.debug(VERBOSE, "registering event handlers");
	    for(var i in pdos.u.events) {
		win.addEventListener(i, captureHandler, true);
		win.addEventListener(i, bubbleHandler, false);
	    }
	}
	
	registerEventHandlers();

	pdos.u.debug(VERBOSE, "finished interception of " + win.location);
    },
    
    nodeFromPath: function(win, path) {
	function nodeFromPathInternal(root, path_arr) {
	    if (path_arr.length == 0)
		return root;
    
	    var idx = path_arr.shift();
	    if (idx >= root.childNodes.length) {
		pdos.u.debug(FATAL, "nodeFromPath: error at node " + root + " : idx = " + idx);
	    }
    
	    return nodeFromPathInternal(root.childNodes[idx], path_arr);
	}
    
	if (path === null) {
	    pdos.u.debug(WARN, "path is null");
	}

	pdos.u.debug(VVERBOSE, "nodeFromPath: "+path);
	
	if (path === "null")
	    return null;
	
	if (path === "window")
	    return win;
	
	var path_arr = path.split(":");
	return nodeFromPathInternal(win.document, path_arr);
    },
    
    replaying: false,  // Used by captureHandler to block input events
    currentEventTime: null,
    lastEvent: null,
    
    sleeptime: 1,      // ms

    checkForQuiescence: function() {
	var quiesceTime = 10; // ms
	var time = (new Date()).getTime();
	if (pdos.ReplayDOM.lastEvent != null && 
	    ((time - pdos.ReplayDOM.lastEvent.timeStamp > quiesceTime) &&
	     (time - pdos.ReplayDOM.currentEventTime > quiesceTime))){
	    pdos.u.debug(VVERBOSE, "in quiesced state. dispatching next event.");
	    setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
	} else {
	    pdos.u.debug(VVERBOSE, "waiting for quiescence");
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.ReplayDOM.sleeptime);
	}
    },

    dispatchNextEvent: function() {
	var item = null;
	var win = null;
	var longsleeptime = 10; // ms
	
	while (true) {
	    if (!pdos.ReplayDOM.logHasNext(pdos.ReplayDOM.log)) {
		// Replay is done. Reset replaying flag so that further
		// input events are blocked
		pdos.u.debug(WARN, "no more log events. finished replaying?");
		pdos.ReplayDOM.replaying = false;
		return false;
	    }
	    
	    item = pdos.ReplayDOM.log.shift();
	    
	    // Dispatch event to the right page's window object
	    win = pdos.u.winFromPageId(item.pageId);
	    if (win == null) {
		// The page may not have been loaded yet. Wait for it to be
		// loaded.
		// XXX: integrate HTTPRequest/Response with this dispatcher, so
		// you don't wait by polling
		if (typeof pdos.u.tabs[item.tabId] != "undefined" &&
		    pdos.u.tabs[item.tabId] != null) {
		    pdos.u.debug(WARN, "got event for a different page. Waiting for page to load: " +
				       JSON.stringify(item));
		    pdos.ReplayDOM.log.unshift(item);
		    setTimeout(pdos.ReplayDOM.dispatchNextEvent, longsleeptime);
		    return false;
		} else {
		    pdos.u.debug(WARN, "ignoring event for different tab " +
					JSON.stringify(item));
		}
	    } else {
		break;
	    }
	}
	
	pdos.u.debug(VERBOSE, "Processing event " + JSON.stringify(item));

	if (item.evType == "setTimeoutCall" || item.evType == "setIntervalCall") {
	    var cb;
	    var pageCallbacks = pdos.ReplayDOM.callbacks[item.pageId];
	    if (typeof pageCallbacks != "undefined" && pageCallbacks != null) {
		cb = [item.cb];
	    }
	    
	    if (typeof cb == "undefined") {
		// Callback not registered yet. Likely setTimeout has not been
		// called. So, wait for callback to be registered.
		// XXX: sequence event dispatching right so that this doesn't
		// happen. That can be done by having dispatcher wait until all
		// previous events in the replay log (including setTimeout) have
		// occurred.
		pdos.u.debug(WARN, "Callback not registered yet. Will retry dispatching event later");
		pdos.ReplayDOM.log.unshift();
		setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
		return true;
	    }
	    
	    pdos.u.debug(VERBOSE, "processing: " + item.evType + " cb: " + cb);
	    var wrappedCb = function() {
		pdos.u.evalInSandbox(win, cb);
		pdos.u.debug(VVERBOSE, item.evType + " finished");
	    };

	    setTimeout(wrappedCb, 1);
	    setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
	    return true;
	}

	/**
	 * The log item is an event. Dispatch it based on type of event to
	 * the right DOM element. While dispatching it, make sure to set
	 * currentEventTime to the time when the event was dispatched.
	 **/
	if (! (item.type in pdos.u.events)) {
	    pdos.u.debug(FATAL, "dispatchNextEvent: error: unknown event type: " + item.type);
	}

	var evClass = pdos.u.events[item.type];
	pdos.ReplayDOM.currentEventTime = (new Date()).getTime();
	
	/**
	 * Of all JS events, we only record and replay the user's keyboard
	 * and mouse events, since that replicates the user's actions. The
	 * rest of the events should be automatically generated from these
	 * events.
	 **/
	// XXX: for Keyboard events, it may help to "click" on the right DOM
	// element so it gets focus and gets the key. This may be especially
	// relevant when replaying only part of the event stream, maybe
	// because the replay environment is different from recording env.
	if (evClass == "KeyboardEvent") {
	    /**
	     * We only replay the keypress events, because:
	     * 1. Playing keyup + keydown events AND keypress events will
	     *    cause repeated keystrokes
	     * 2. keyup + keydown events are not generated for all
	     *    keystrokes. For example, if a key is kept pressed, it will
	     *    only generated one keydown and keyup event, but many
	     *    keypress events depending on how many characters were
	     *    generated.
	     * 3. As far as I can tell, keypress events are generated for
	     *    every keystroke
	     **/
	    if (item.type != "keypress") {
		setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
		return true;
	    }
	    
	    /**
	     * To generate robot keydown and keyup events from JS keypress
	     * events, we convert the charCode in keypress events into
	     * keyCode required by robot, using a conversion table.
	     * We also generate the required keydown and keyup events for
	     * the modifier keys that were turned on in the keypress event.
	     **/
	    var keyCode = item.keyCode;
	    if (typeof keyCode == "undefined" || keyCode == null || keyCode == 0) {
		keyCode = keycodes[item.charCode];
		if (typeof keyCode == "undefined") {
		    pdos.u.debug(WARN, "error dispatching keypress: could not find keycode for charcode " + item.charCode);
		    setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
		    return true;			
		} 
	    }
	    
	    var modKeys = ["shiftKey", "ctrlKey", "altKey", "metaKey"];
	    for (var i in modKeys) {
		if (item[modKeys[i]]) {
		    pdos.robot.keydown(keycodes[modKeys[i]]);
		}
	    }
	    pdos.robot.keydown(keyCode);
	    pdos.robot.keyup(keyCode);
	    for (var i in modKeys) {
		if (item[modKeys[i]]) {
		    pdos.robot.keyup(keycodes[modKeys[i]]);
		}
	    }
	    
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.ReplayDOM.sleeptime);
	    return true;
	} else if (evClass == "MouseEvent") {
	    pdos.u.debug(VERBOSE, "dispatching mouse event");
	    // Only dispatch mousemove, mouseup, mousedown, and scroll events.
	    // The remaining mouse events would be automatically generated from
	    // these events.
	    if (item.type != "mousedown" && item.type != "mouseup" &&
		item.type != "mousemove" && item.type != "DOMMouseScroll") {
		pdos.u.debug(VVERBOSE, "not one of the mouse events to dispatch");
		setTimeout(pdos.ReplayDOM.dispatchNextEvent, pdos.ReplayDOM.sleeptime);
		return true;
	    }
	    
	    pdos.robot.mousemove(item.screenX, item.screenY);
	    if (item.type == "mousedown" || item.type == "mouseup") {
		// Dispatch mouseup and mousedown events
		pdos.robot[item.type] ();
	    } else if (item.type == "DOMMouseScroll") {
		pdos.robot.mousewheel(item.detail);
	    }
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.ReplayDOM.sleeptime);
	    return true;
	}
	
	pdos.u.debug(WARN, "cannot dispatch event class: " + evClass + ".\n"+
			   "only keyboard and mouse events supported");
	return false;
    },

    startReplay: function() {
	this.replaying = true;
	this.dispatchNextEvent();
    }
};

if (pdos.u.mode() == "replay") {
    pdos.ReplayDOM.init();
}
