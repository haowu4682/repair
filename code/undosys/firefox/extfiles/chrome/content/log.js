
var log = {
    dumpAfter : 100,  // flush to file after these many entries
    pageLogs  : {},   // pageid -> { log, fileidx }
    logDir    : fs.retroDir() + "/logs",

    dumpToFile: function(pageId) {
        dbg.dbg(VERBOSE, "dumping log for page " + pageId);
        
        var p = log.pageLogs[pageId];
        fs.createDir(log.logDir);
        fs.createDir(log.logDir + "/" + pageId);
        fs.writeFile(log.logDir + "/" + pageId + "/" + p.logIdx, JSON.stringify(p.log));

        p.log = [];
        p.logIdx++;
    },

    dumpAll: function() {
        for (var p in log.pageLogs)
            if (p != null)
                log.dumpToFile(p);
    },

    remove: function(pageId) {
        dbg.dbg(VERBOSE, "flushing log entries for page " + pageId);
        log.dumpToFile(pageId);
        delete(log.pageLogs[pageId]);
    },
    
    record: function(entry, win) {
        dbg.assert((pdos.u.mode() == "record"), "log.record called in non-record mode");
        
        if (entry.evType != "HTTPRequest" && entry.evType != "HTTPResponse")
            entry.pageId = ids.getWinPageId(win);

        var pid = entry.pageId;
        if (pid == null)
            return;
        
        if (!(pid in log.pageLogs))
            log.pageLogs[pid] = { log: [], logIdx: 0 };
        log.pageLogs[pid].log.push(entry);

        if (log.pageLogs[pid].log.length > log.dumpAfter)
            log.dumpToFile(pid);
    },

    init: function() {
        atexit.register(log.dumpAll);
    }
};

log.init();