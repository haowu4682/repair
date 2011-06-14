
/**
 * XXX: TODO
 *     deal with subframes by tracking subframe creation
 *     may have to reset cookies for subsequent requests, if the URL differs
 *     from the previous ones
 **/

if (typeof pdos == "undefined") {
    var pdos = {};
}

pdos.RepairMgr = {
    repairing   : false,
    
    log         : [],
    currentEvent: null,
    handlers    : {},    // event type -> handler
    pollInterval: 2,     // ms
    
    /**
     * navigate to the page being repaired
     **/
    visitPage: function() {
        // search log for the first top-level page and visit it
        for (var i in pdos.RepairMgr.log) {
            var e = pdos.RepairMgr.log[i];
            if (e.evType == "HTTPRequest" && e.isMainDoc && e.isTopWindow) {
                gBrowser.contentWindow.location = e.URI.asciiSpec;
                return true;
            }
        }
        
        return false;
    },

    closeBrowser: function() {
        // take screenshot at the end of repair
        var imgFile = pdos.RepairMgr.logDir + "/screenshot.png";
        pdos.robot.grab(imgFile);
        gBrowser.contentWindow.close();
    },
    
    /**
     * dispatch the next event with a handler, that has not already been
     * dispatched
     **/
    dispatch: function() {
        while (pdos.RepairMgr.log.length > 0) {
            var e = pdos.RepairMgr.log.shift();
            if (e.evType in pdos.RepairMgr.handlers) {
                if (e.dispatched) {
                    pdos.u.debug(VVVERBOSE, "event already dispatched: " + JSON.stringify(e));
                } else {
                    pdos.u.debug(VVVERBOSE, "dispatching event: " + JSON.stringify(e));
                    
                    // set dispatched to true before calling handler to be
                    // consistent with getMatchingEvent
                    // XXX: this needs to change for dispatching to subframes
                    e.dispatched = true; 
                    pdos.RepairMgr.handlers[e.evType](e, gBrowser.contentWindow);
                } 
                
                pdos.RepairMgr.currentEvent = e;
                setTimeout(pdos.RepairMgr.wait, pdos.RepairMgr.pollInterval);
                return;
            } else {
                pdos.u.debug(VVVERBOSE, "ignoring event: " + JSON.stringify(e));
            }
        }
        
        pdos.RepairMgr.repairing = false;
        pdos.u.debug(VERBOSE, "*** Repair DONE. Closing browser after 3 seconds ***");
        setTimeout(pdos.RepairMgr.closeBrowser, 3000);
    },
    
    /**
     * wait for current event to be done
     **/
    wait: function() {
        var e = pdos.RepairMgr.currentEvent;
        pdos.u.debug(VVVERBOSE, "waiting for event to be done: " + JSON.stringify(e));

        var func = pdos.RepairMgr.wait;
        var t = pdos.u.events[e.type];
        if ((t != 'KeyboardEvent' && t != 'MouseEvent') ||
            ('done' in e && e.done)) {
            func = pdos.RepairMgr.dispatch;
        }

        setTimeout(func, pdos.RepairMgr.pollInterval);
    },
    
    repair: function() {
        if (pdos.u.mode() != "replay") {
            pdos.u.debug(WARN, "repair called in non-replay mode");
            return;
        }

        // read event log for this page -- pageToRepair contains
        // clientId pageId
        var subdir = fs.readFile("/tmp/retro/pageToRepair").replace(/\s+/, "/");
        pdos.RepairMgr.logDir = fs.repairDir() + "/" + subdir;

        var log = [];
        for (var i=0; ; i++) {
            dbg.dbg(VERBOSE, "reading log file " + pdos.RepairMgr.logDir + "/" + i);
            var data = fs.readFile(pdos.RepairMgr.logDir + "/" + i);
            if (data == null)
                break;
            log = log.concat(JSON.parse(data));
        }
        pdos.RepairMgr.log = log;
        
        pdos.RepairMgr.repairing = true;
        if (!pdos.RepairMgr.visitPage()) {
            pdos.u.debug(WARN, "visitPage failed");
            return;
        }
        
        setTimeout(pdos.RepairMgr.dispatch, pdos.RepairMgr.pollInterval);
    },
    
    registerHandler: function(evType, handler) {
        pdos.u.debug(VERBOSE, "registering handler for event type " + evType);
        pdos.RepairMgr.handlers[evType] = handler;
    },
    
    getMatchingEvent: function(filterFn) {
        for (var i in pdos.RepairMgr.log) {
            var e = pdos.RepairMgr.log[i];
            if (filterFn(e)) {
                if (i > 0) {
                    pdos.u.debug(WARN, "out-of-order replay event: " + JSON.stringify(e));
                }
                e.dispatched = true;
                return e;
            }
        }
        
        return null;
    }
};

// XXX: do this on app start instead of with a setTimeout
if (pdos.u.mode() == "replay") {
    setTimeout(pdos.RepairMgr.repair, 2000);
}
