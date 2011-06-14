
var atexit = {
    exitHandlers: [],
    observerService: null,
    
    init: function() {
        dbg.dbg(VERBOSE, "atexit: init");
        this.observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
        this.observerService.addObserver(this, "quit-application-requested", false);
        atexit.register(atexit.uninit);
    },

    uninit: function() {
        atexit.observerService.removeObserver(atexit, "quit-application-requested", false);
    },

    observe: function(subject, topic, data) {
        if (topic != "quit-application-requested")
            return;

        dbg.dbg(VERBOSE, "calling atexit handlers");
        for (var i in atexit.exitHandlers) {
            try {
                atexit.exitHandlers[i]();
            } catch (e) {
                dbg.dbg(WARN, "error running handler: call stack: " + dbg.formatStack(e.stack.split("\n")));
            }
        }
    },

    register: function(handler) {
        atexit.exitHandlers.push(handler);
    }
};

atexit.init()