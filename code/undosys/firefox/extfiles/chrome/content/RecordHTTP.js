
if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.RecordHTTP = {
    observerService: null,
    
    currentLogEntry: null,
    
    requestId: 0,
    
    init: function() {
        pdos.u.debug(VERBOSE, "initializing RecordHTTP");
        
        // Register with observer service
        this.observerService = pdos.u.CC["@mozilla.org/observer-service;1"].getService(pdos.u.CI.nsIObserverService);
        this.observerService.addObserver(this, "http-on-modify-request", false);
        this.observerService.addObserver(this, "http-on-examine-response", false);
        this.observerService.addObserver(this, "http-on-examine-cached-response", false);
        
        // To unload when the browser exits
        atexit.register(pdos.RecordHTTP.uninit);

        pdos.u.debug(VERBOSE, "initialized RecordHTTP");
    },
    
    uninit: function() {
        pdos.u.debug(VERBOSE, "uninitializing RecordHTTP");
        
        // Unregister with observer service
        pdos.RecordHTTP.observerService.removeObserver(pdos.RecordHTTP, "http-on-modify-request", false);
        pdos.RecordHTTP.observerService.removeObserver(pdos.RecordHTTP, "http-on-examine-response", false);
        pdos.RecordHTTP.observerService.removeObserver(pdos.RecordHTTP, "http-on-examine-cached-response", false);
        
        pdos.u.debug(VERBOSE, "uninitialized RecordHTTP");
    },
    
    observe: function(subject, topic, data) {
        pdos.u.debug(VVERBOSE, "observe function called: " + topic);
        
        subject.QueryInterface(Components.interfaces.nsIHttpChannel);
        
        if (!subject.notificationCallbacks) {
            pdos.u.debug(VERBOSE, "undefined notificationCallbacks for " + subject.URI.asciiSpec);
            return;
        }
        
        var wp = null;
        try {
            wp = subject.notificationCallbacks.getInterface(pdos.u.CI.nsIWebProgress);
        } catch (e) {
            pdos.u.debug(VERBOSE, "caught exception during getInterface on nsIWebProgress for "
                         + subject.URI.asciiSpec + " : " + e);
        }
        if (!wp || !wp.DOMWindow || !wp.DOMWindow.wrappedJSObject) {
            pdos.u.debug(VERBOSE, "undefined DOM Window for " + subject.URI.asciiSpec);
            return;            
        }
        
        /**
         * mainDoc flag is set if the request is for the main document in the
         * frame, and not for components (such as images, js, css etc.,).
         * 
         * topWindow flag indicates that the request is for a component in the
         * the top-level frame, and is true even for, say, images in the
         * top-level frame.
         *
         * So, if both mainDoc and topWindow are true, that indicates that the
         * request is for loading the main document in the top-level frame.
         **/
        var mainDocFlag = Components.interfaces.nsIChannel.LOAD_INITIAL_DOCUMENT_URI;
        var mainDoc = ((subject.loadFlags & mainDocFlag) == mainDocFlag);

        var topWindow = (wp.DOMWindow.wrappedJSObject == wp.DOMWindow.top.wrappedJSObject);
        pdos.u.debug(VVERBOSE, subject.URI.asciiSpec + ": mainDoc = " + mainDoc + ": topWindow = " + topWindow);
        
        if (topic == "http-on-modify-request") {
            /* Set client ID */
            subject.setRequestHeader("X_CLIENT_ID", ids.getClientId(), false);
            
            /**
             * Set X_REQ_ID header to request id so that the server can record
             * requests and responses.
             **/
            var reqId = ids.getNextReqId();
            pdos.u.debug(VERBOSE, "setting X_REQ_ID for " + subject.URI.asciiSpec + " to " + reqId);
            subject.setRequestHeader("X_REQ_ID", reqId, false);

            var topPageId = null;
            if (mainDoc && topWindow) {
                // Allocate a new page id when the main document is loaded
                topPageId = ids.allocatePageId(wp.DOMWindow.wrappedJSObject,
                                               subject.URI.asciiSpec);
            } else {
                // If this is not the main doc in top window, get the pageid
                // of top window's doc
                topPageId = ids.getWinPageId(wp.DOMWindow.top.wrappedJSObject);
            }
            
            pdos.u.debug(VERBOSE, "setting X_PAGE_ID for " + subject.URI.asciiSpec + " to " + topPageId);
            subject.setRequestHeader("X_PAGE_ID", topPageId, false);

            /**
             * Record:
             *   All the request headers (incl. cookies for now)
             *   Info in the nsIHTTPChannel and nsIChannel objects (e.g.: referrer)
             *   For POST requests, the request data
             *   Window which generated the request
             * Encapsulate all this info into a single log entry
             **/
            currentLogEntry = {
                "evType"     : "HTTPRequest",
                "headers"  : {},
                "isMainDoc": mainDoc,
                "isTopWindow": topWindow,
                "pageId"   : topPageId
            };
            subject.visitRequestHeaders(this);
            this.saveRequestParams(subject);
            this.savePostData(subject, wp.DOMWindow);
            log.record(currentLogEntry);
        } else if (topic == "http-on-examine-response") {
            /**
             * Record:
             *   All the response headers (incl. new cookie values)
             *   Old cookie value
             *   XXX: Response data 
             *   XXX: Request that generated this response. Keep track of all
             *   issued requests and match them backwards
             **/
            var topPageId = null;
            if (mainDoc) {
                topPageId = ids.getPageId(wp.DOMWindow.wrappedJSObject,
                                          subject.URI.asciiSpec);
            } else {
                topPageId = ids.getWinPageId(wp.DOMWindow.top.wrappedJSObject);
            }
            currentLogEntry = {
                "evType"     : "HTTPResponse",
                "headers"  : {},
                "isMainDoc": mainDoc,
                "pageId"   : topPageId
            };
            subject.visitResponseHeaders(this);
            this.saveResponseParams(subject);
            this.saveCookie(subject);
            log.record(currentLogEntry);
        } else if (topic == "http-on-examine-cached-response") {
            //XXX: Do the same as for on-examine-response
        }
    },
    
    /**
     * Function to visit request and response headers and store them in the
     * current log entry
     **/
    visitHeader: function(key, value) {
        pdos.u.debug(VVERBOSE, "visiting header: " + key + " => " + value);
        currentLogEntry.headers[key] = value;
    },
    
    saveRequestParams: function(httpChan) {
        pdos.u.debug(VVERBOSE, "saving request params");
        currentLogEntry.requestMethod = httpChan.requestMethod;
        currentLogEntry.referrer = httpChan.referrer;
        currentLogEntry.URI = httpChan.URI;
    },

    savePostData: function(httpChan, win) {
        if (httpChan.requestMethod.toUpperCase() != "POST") {
            return;
        }
        pdos.u.debug(VVERBOSE, "saving post data");

        try {
	    var NS_SEEK_SET = pdos.u.CI.nsISeekableStream.NS_SEEK_SET;
            var is = httpChan.QueryInterface(pdos.u.CI.nsIUploadChannel).uploadStream;
	    if (!is) {
		pdos.u.debug(VERBOSE, "is is false");
		return;
	    }
	    
            var ss = is.QueryInterface(pdos.u.CI.nsISeekableStream);
            var prevOffset;
            if (ss) {
                prevOffset = ss.tell();
                ss.seek(NS_SEEK_SET, 0);
            }

            // Read data from the stream..
            var charset = win ? win.document.characterSet : null;
	    var sis = pdos.u.CC["@mozilla.org/binaryinputstream;1"].getService(pdos.u.CI["nsIBinaryInputStream"]);
	    sis.setInputStream(is);

	    var segments = [];
	    for (var count = is.available(); count; count = is.available())
		segments.push(sis.readBytes(count));
	    var text = segments.join("");

	    var conv = pdos.u.CC["@mozilla.org/intl/scriptableunicodeconverter"].getService(pdos.u.CI["nsIScriptableUnicodeConverter"]);
	    conv.charset = charset ? charset : "UTF-8";
	    text = conv.ConvertToUnicode(text);

            // Seek locks the file so, seek to the beginning only if necko hasn't read it yet,
            // since necko doesn't seek to 0 before reading (at lest not till 459384 is fixed).
            if (ss && prevOffset == 0)
                ss.seek(NS_SEEK_SET, 0);

            currentLogEntry.postData = text;
	    pdos.u.debug(VERBOSE, "Got POST data: " + text);
        } catch(exc) {
	    pdos.u.debug(WARN, "caught exception: " + exc);
	}
    },

    saveResponseParams: function(httpChan) {
        pdos.u.debug(VVERBOSE, "saving response params");
        currentLogEntry.responseStatus = httpChan.responseStatus;
        currentLogEntry.contentCharset = httpChan.contentCharset;
        currentLogEntry.contentType = httpChan.contentType;
        currentLogEntry.contentLength = httpChan.contentLength;
        currentLogEntry.URI = httpChan.URI;
    },
    
    saveCookie: function(httpChan) {
        try {
            // Throws exception if header not found, because of which we do not
            // save the old cookie value
            httpChan.getResponseHeader('Set-Cookie');
            
            pdos.u.debug(VVERBOSE, "recording old cookie");
            var cookieSvc = pdos.u.CC["@mozilla.org/cookieService;1"].getService(pdos.u.CI.nsICookieService);
            currentLogEntry.oldCookie = cookieSvc.getCookieString(httpChan.URI, null);
        } catch (e) {
            pdos.u.debug(VVERBOSE, "Set-Cookie header not found in response: " + e);
        }        
    }
};

if (pdos.u.mode() == "record") {
    pdos.RecordHTTP.init();
}

