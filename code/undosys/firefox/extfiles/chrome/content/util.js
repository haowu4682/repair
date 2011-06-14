
if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.u = {
    // some constants
    CI           : Components.interfaces,
    CC           : Components.classes,
    CR           : Components.results,

    init: function() {
        pdos.u.START        = pdos.u.CI.nsIWebProgressListener.STATE_START;
        pdos.u.REDIRECTING  = pdos.u.CI.nsIWebProgressListener.STATE_REDIRECTING;
        pdos.u.TRANSFERRING = pdos.u.CI.nsIWebProgressListener.STATE_TRANSFERRING;
        pdos.u.STOP         = pdos.u.CI.nsIWebProgressListener.STATE_STOP;
        pdos.u.IS_REQUEST   = pdos.u.CI.nsIWebProgressListener.STATE_IS_REQUEST;
        pdos.u.IS_WINDOW    = pdos.u.CI.nsIWebProgressListener.STATE_IS_WINDOW;
        pdos.u.IS_DOC       = pdos.u.CI.nsIWebProgressListener.STATE_IS_DOCUMENT;
        pdos.u.START_DOC    = pdos.u.START | pdos.u.IS_DOC;
        pdos.u.STOP_DOC     = pdos.u.STOP | pdos.u.IS_DOC;
        pdos.u.ABORT        = pdos.u.CR.NS_BINDING_ABORTED;
    },

    /**
     * Returns the current browser mode (either record or replay)
     **/
    cachedMode: null,
    mode: function() {
        if (pdos.u.cachedMode == null) {
            var modeFile = fs.retroDir() + "/mode";
            pdos.u.cachedMode = fs.readFile(modeFile);

            // if mode not defined, set it to record 
            if (pdos.u.cachedMode == null) {
                pdos.u.cachedMode = "record";
                fs.writeFile(modeFile, pdos.u.cachedMode);
            }
        }
    
        return pdos.u.cachedMode;
    },

    wpStateToString: function(stateFlag) {
        var states = ["START", "STOP", "TRANSFERRING", "REDIRECTING",
                      "IS_DOC", "IS_WINDOW", "IS_REQUEST"];
        var flags = [];
        for (var s in states) {
            if (stateFlag & pdos.u[states[s]])
                flags.push(states[s]);
        }
        
        return flags.join('|');
    },

    // defined elsewhere
    debug  : dbg.dbg,
    events : ev.events,
    eventPropertiesToLog: ev.eventPropertiesToLog,
    cancelEvent: ev.cancelEvent,

    /**
     * Evaluate untrusted code (cb) in a JS sandbox for window win. See:
     *   https://developer.mozilla.org/Talk:en/XPCNativeWrapper
     * for why it is important to do this. 
     **/
    evalInSandbox: function(win, cb) {
        try {
            var sb = Components.utils.Sandbox(win);
            sb.window = win;
            pdos.u.debug(VVERBOSE, "evaluating cb in sandbox: \n" + cb);
            if (typeof cb == "function") {
                sb.unsafeCb = cb;
                Components.utils.evalInSandbox("unsafeCb();", sb);
            } else {
                Components.utils.evalInSandbox(cb, sb);
            }
            pdos.u.debug(VVERBOSE, "done evaluating cb in sandbox: \n" + cb);
        } catch (e) {
            pdos.u.debug(WARN, "exception in cb : " + cb + " : " + e);
        }
    }
};

pdos.u.init();
