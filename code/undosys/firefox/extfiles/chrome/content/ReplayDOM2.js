if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.ReplayDOM = {
    /**
     * The init, QueryInterface and onStateChange functions are quite similar
     * to those in pdos.RecordDOM
     **/
    init: function() {
	// Register replay handler for keyboard and mouse events
	pdos.RepairMgr.registerHandler("event", pdos.ReplayDOM.evHandler);
	    
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
    
    onStateChange: function(wp, req, stateFlag, status) {
	try {
	    pdos.u.debug(VVVERBOSE, "ReplayDOM.onStateChange called for window: " +
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
		pdos.u.debug(VERBOSE, "done intercepting window: " + win.location);
		win.intercepted = true;
	    }
	} catch (e) {
	    pdos.u.debug(WARN, "caught exception: " + e + "\n" + e.stack);
	}
    },

    intercept: function(win) {

	pdos.u.debug(VERBOSE, "intercepting window: " + win.location);
	
	function captureHandler(e) {
	    /**
	     * Screen all input events if repair is not going on. This
	     * avoids user accidentally modifying the web page from its
	     * initial state.
	     * 
	     * XXX: Ideally, we should screen all input events except
	     * those that are sent as part of replay. However, since replay
	     * sends input events via Robot, they cannot be easily
	     * distinguished from user's input events. One way to do this
	     * is by the properties of the event, such as keycode or
	     * screen location; however, since one event sent via Robot can
	     * induce several other events (at slightly different screen
	     * locations, for example), it is difficult to screen this way.
	     * Another way to screen could be by using the target DOM element 
	     * that should get the event; however, adjacent DOM elements may 
	     * also need to get events, as for example, in the case of
	     * mousemove; hence those events cannot be blocked.
	     **/
	    if (!pdos.RepairMgr.repairing && e.type != "beforeunload") {
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

	pdos.u.debug(VERBOSE, "finished interception of " + win.location);
    },
    
    currentEvent    : null,
    currentEventTime: null,
    lastEvent       : null,
    
    quiesceTime     : 5,     // ms

    checkForQuiescence: function() {
	var time = (new Date()).getTime();
	if (pdos.ReplayDOM.lastEvent != null && 
	    ((time - pdos.ReplayDOM.lastEvent.timeStamp > pdos.ReplayDOM.quiesceTime) &&
	     (time - pdos.ReplayDOM.currentEventTime > pdos.ReplayDOM.quiesceTime))){
	    pdos.u.debug(VVERBOSE, "in quiesced state. dispatching next event.");
	    pdos.ReplayDOM.currentEvent.done = true;
	} else {
	    pdos.u.debug(VVERBOSE, "waiting for quiescence");
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.RepairMgr.pollInterval);
	}
    },

    evHandler: function(e, win) {
	pdos.ReplayDOM.currentEvent = e;
	pdos.ReplayDOM.currentEventTime = (new Date()).getTime();
	
	var evClass = pdos.u.events[e.type];

	/**
	 * XXX: for Keyboard events, it may help to set focus on the DOM
	 * element so that the key is sent to it. In particular, this may
	 * be necessary when replaying only part of the event stream.
	 **/
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
	     *    all keystrokes
	     **/
	    if (e.type != "keypress") {
		e.done = true;
		return true;
	    }
	    
	    /**
	     * To generate robot keydown and keyup events from JS keypress
	     * events, we convert the charCode in keypress events into
	     * keyCode required by robot, using a conversion table.
	     * We also generate the required keydown and keyup events for
	     * the modifier keys that were turned on in the keypress event.
	     **/
	    var keyCode = e.keyCode;
	    if (typeof keyCode == "undefined" || keyCode == null || keyCode == 0) {
		keyCode = keycodes[e.charCode];
		if (typeof keyCode == "undefined") {
		    pdos.u.debug(WARN, "error dispatching keypress: could not find keycode for charcode " + e.charCode);
		    e.done = true;
		    return false;
		} 
	    }
	    
	    var modKeys = ["shiftKey", "ctrlKey", "altKey", "metaKey"];
	    for (var i in modKeys) {
		if (e[modKeys[i]]) {
		    pdos.robot.keydown(keycodes[modKeys[i]]);
		}
	    }
	    pdos.robot.keydown(keyCode);
	    pdos.robot.keyup(keyCode);
	    for (var i in modKeys) {
		if (e[modKeys[i]]) {
		    pdos.robot.keyup(keycodes[modKeys[i]]);
		}
	    }
	    
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.RepairMgr.pollInterval);
	    return true;
	} else if (evClass == "MouseEvent") {
	    pdos.u.debug(VERBOSE, "dispatching mouse event");
	    // Only dispatch mousemove, mouseup, mousedown, and scroll events.
	    // The remaining mouse events would be automatically generated from
	    // these events.
	    if (e.type != "mousedown" && e.type != "mouseup" &&
		e.type != "mousemove" && e.type != "DOMMouseScroll") {
		pdos.u.debug(VVERBOSE, "not one of the mouse events to dispatch");
		e.done = true;
		return true;
	    }
	    
	    pdos.robot.mousemove(e.screenX, e.screenY);
	    if (e.type == "mousedown" || e.type == "mouseup") {
		pdos.robot[e.type] ();
	    } else if (e.type == "DOMMouseScroll") {
		pdos.robot.mousewheel(e.detail);
	    }
	    setTimeout(pdos.ReplayDOM.checkForQuiescence, pdos.RepairMgr.pollInterval);
	    return true;
	}
	
	pdos.u.debug(WARN, "cannot dispatch event class: " + evClass + ".\n"+
			   "only keyboard and mouse events supported");
	return false;
    }
};

if (pdos.u.mode() == "replay") {
    pdos.ReplayDOM.init();
}
