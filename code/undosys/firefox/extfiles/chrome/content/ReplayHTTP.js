
if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.ReplayHTTP = {
    observerService: null,
    
    httpLog: [],

    /**
     * The format of the init, uninit and observe functions in ReplayHTTP is 
     * very similar to the ones in RecordHTTP
     **/
    init: function() {
        pdos.u.debug(VERBOSE, "initializing ReplayHTTP");
        
	// turn on private browsing
	pdos.ReplayHTTP.enablePrivateBrowsing();
	
        // Register with observer service
        this.observerService = pdos.u.CC["@mozilla.org/observer-service;1"].getService(pdos.u.CI.nsIObserverService);
        this.observerService.addObserver(this, "http-on-modify-request", false);
        this.observerService.addObserver(this, "http-on-examine-response", false);
        this.observerService.addObserver(this, "http-on-examine-cached-response", false);
        
        // To unload when the browser exits
        this.observerService.addObserver(this, "quit-application-requested", false);

	// Add replay log handler for HTTPRequest and HTTPResponse events
	pdos.RepairMgr.registerHandler("HTTPRequest", pdos.ReplayHTTP.evHandler);
	pdos.RepairMgr.registerHandler("HTTPResponse", pdos.ReplayHTTP.evHandler);

        pdos.u.debug(VERBOSE, "initialized ReplayHTTP");
    },
    
    /**
     * run redo in firefox private browsing mode, which creates a temporary
     * cookie database that starts out empty
     **/
    enablePrivateBrowsing: function() {
	pdos.u.debug(VERBOSE, "enabling private browsing mode");
        var pbs = Components.classes["@mozilla.org/privatebrowsing;1"]
                    .getService(Components.interfaces.nsIPrivateBrowsingService);
        pbs.privateBrowsingEnabled = true;
    },
    
    evHandler: function(e, win) {
	pdos.ReplayHTTP.httpLog.push(e);
    },
    
    uninit: function() {
        pdos.u.debug(VERBOSE, "uninitializing ReplayHTTP");
        
        // Unregister with observer service
        this.observerService.removeObserver(this, "http-on-modify-request", false);
        this.observerService.removeObserver(this, "http-on-examine-response", false);
        this.observerService.removeObserver(this, "http-on-examine-cached-response", false);
        this.observerService.removeObserver(this, "quit-application-requested", false);
        
        pdos.u.debug(VERBOSE, "uninitialized ReplayHTTP");
    },
    
    /**
     * returns a function that checks equivalence of a log event with the
     * current HTTPRequest / HTTPResponse 
     **/
    cmp: function(subject, topic) {
	var evType = null;
	if (topic == "http-on-modify-request") {
	    evType = "HTTPRequest";
	} else if (topic == "http-on-examine-response") {
	    evType = "HTTPResponse";
	}
	
	// XXX: filter function may need to compare more than just URI
	return function(e) {
	    return (e != null && evType != null && e.evType == evType &&
		    e.URI.asciiSpec == subject.URI.asciiSpec);
	};
    },
    
    /**
     * searches httpLog for an event matching e and returns matching index, or
     * -1 if no match
     **/
    searchHttpLog: function (subject, topic) {
	var filterFn = pdos.ReplayHTTP.cmp(subject, topic);
	for (var i in pdos.ReplayHTTP.httpLog) {
	    if (filterFn(pdos.ReplayHTTP.httpLog[i])) {
		return i;
	    }
	}
	
	return -1;
    },
    
    injectPostData: function(httpChan, e) {
	if (this.evNum > 1) {
	    // inject post data only into the first replayed page
	    return;
	}
	
        if (e.requestMethod.toUpperCase() != "POST") {
            return;
        }

        var match = e.postData.match("Content-Type:\\s+(\\S+)\r\n.*\r\n\r\n(.*)");
        if (match == null) {
            pdos.u.debug(WARN, "invalid postContent: " + e.postData);
            return;
        }
        var contentType = match[1];
        var data = match[2];
        
        pdos.u.debug(VVERBOSE, "updating post content with " + e.postData + "\ndata: " + data);

        var uploadChan = httpChan.QueryInterface(pdos.u.CI.nsIUploadChannel);
        var stringStream = pdos.u.CC["@mozilla.org/io/string-input-stream;1"].createInstance(pdos.u.CI["nsIStringInputStream"]);

        if ("data" in stringStream) // Gecko 1.9 or newer
            stringStream.data = data;
        else // 1.8 or older
            stringStream.setData(data, data.length);

        uploadChan.setUploadStream(stringStream, contentType, -1);

        // need to set request method to POST, due to FF bug. see:
        // https://developer.mozilla.org/en/Creating_Sandboxed_HTTP_Connections#Creating_HTTP_POSTs
        httpChan.requestMethod = "POST"; 
    },
    
    setRequestParams: function(subject, e) {
	subject.setRequestHeader("X_CLIENT_ID", e.headers["X_CLIENT_ID"], false);
	subject.setRequestHeader("X_REQ_ID", e.headers["X_REQ_ID"], false);
	subject.setRequestHeader("X_PAGE_ID", e.headers["X_PAGE_ID"], false);

        // XXX: hack to save id of the page being repaired
        pdos.ReplayHTTP.pageId = e.headers["X_PAGE_ID"]; 

	/**
	 * set cookie from the recorded value, if no cookie is being sent by
	 * the browser. since firefox is started in private browsing mode, the
	 * browser does not send a cookie either (i) if this URI is being
	 * visited the first time during this redo session or (ii) the server
         * has not set a cookie for this URI yet, even though it has been
	 * visited before during this redo.
	 **/
	try {
	    subject.getRequestHeader("Cookie");
	} catch (err) {
	    // cookie not set, so copy recorded cookie
	    if ('Cookie' in e.headers) {
		var cookie = unescape(e.headers["Cookie"]);
		pdos.u.debug(VERBOSE, "setting cookie for: " + subject.URI.asciiSpec + " to: " + cookie);
		subject.setRequestHeader("Cookie", cookie, false);
	    }
	}

	// replay post data
	this.injectPostData(subject, e);
    },
    
    checkRequestParams: function(subject, e) {
	// XXX: check for equiv of this request with recorded request event,
	// otherwise print warning
    },
    
    checkResponseParams: function(subject, e) {
	// XXX: check for equiv of this response with recorded response event,
	// otherwise print warning
    },
    
    evNum: 0,
    observe: function(subject, topic, data) {
        pdos.u.debug(VVERBOSE, "observe function called: " + topic);
	
        if (topic == "quit-application-requested") {
            this.uninit();
            return;
        }
	
	if (!pdos.RepairMgr.repairing) {
	    pdos.u.debug(VVERBOSE, "repair not in progress");
	    return;
	}
	
	if (topic != "http-on-modify-request" &&
	    topic != "http-on-examine-response") {
	    return;
	}
        
        subject.QueryInterface(Components.interfaces.nsIHttpChannel);
        
        // Ignore requests that are not from a document window -- these events
	// are also ignored during recording
        if (!subject.notificationCallbacks) {
            pdos.u.debug(VERBOSE, "undefined notificationCallbacks for " + subject.URI.asciiSpec);
            return;
        }
        var wp = null;
        try {
            wp = subject.notificationCallbacks.getInterface(pdos.u.CI.nsIWebProgress);
        } catch (e) {
            pdos.u.debug(VERBOSE, "caught exception during getInterface on nsIWebProgress: " + e);
        }
        if (!wp || !wp.DOMWindow || !wp.DOMWindow.wrappedJSObject) {
            pdos.u.debug(VERBOSE, "undefined DOM Window for " + subject.URI.asciiSpec);
            return;            
        }
        
	// clean up deleted entries at the head of log
	while (pdos.ReplayHTTP.httpLog.length > 0 &&
	       pdos.ReplayHTTP.httpLog[0] == null) {
	    pdos.ReplayHTTP.httpLog.shift();
	}
	
	// look for the matching recorded event
	var ev = null;
	var idx = pdos.ReplayHTTP.searchHttpLog(subject, topic);
	if (idx >= 0) {
	    ev = pdos.ReplayHTTP.httpLog[idx];
	} else {
	    ev = pdos.RepairMgr.getMatchingEvent(pdos.ReplayHTTP.cmp(subject, topic));
	}
	if (ev == null) {
	    pdos.u.debug(WARN, "no matching recorded event for URI="+
			 subject.URI.asciiSpec + " topic=" + topic);

            // if this is http req that does not load the top-level page,
            // then it is a req within this page. so, attach the current
            // page id to it
            var mainDocFlag = Components.interfaces.nsIChannel.LOAD_INITIAL_DOCUMENT_URI;
            var mainDoc = ((subject.loadFlags & mainDocFlag) == mainDocFlag);
            var topWindow = (wp.DOMWindow.wrappedJSObject == wp.DOMWindow.top.wrappedJSObject);
            if (topic == "http-on-modify-request" && !(mainDoc && topWindow)) {
	        subject.setRequestHeader("X_PAGE_ID", pdos.ReplayHTTP.pageId, false);
            }
            
	    return;  
	}
	
	// process the event
	this.evNum++;
	if (topic == "http-on-modify-request") {
	    pdos.ReplayHTTP.setRequestParams(subject, ev);
	    pdos.ReplayHTTP.checkRequestParams(subject, ev);
        } else if (topic == "http-on-examine-response") {
	    pdos.ReplayHTTP.checkResponseParams(subject, ev);
        } else if (topic == "http-on-examine-cached-response") {
            //XXX: Do the same as for on-examine-response
            pdos.u.debug(WARN, "http-on-examine-cached-response");
	}	    
	
	// mark event done and delete it from log
	ev.done = true;
	if (idx > 0) {
	    delete(pdos.ReplayHTTP.httpLog[idx]);
	}
    }
};

if (pdos.u.mode() == "replay") {
    pdos.ReplayHTTP.init();
}

