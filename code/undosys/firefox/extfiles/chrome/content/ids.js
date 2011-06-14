
// XXX: we don't have a reverse lookup table to lookup pages from page
//      ids. May need one later.

var ids = {
    clientIdFile: fs.retroDir() + "/clientId",
    pageIdFile  : fs.retroDir() + "/savedPageId",

    clientId: null,
    reqId: 0,
    lastPageId: null,

    // XXX: we use list here rather than a map, since JS doesn't
    // support using objects as indexes in associative maps
    // { win: winRef, pages: [] }, where pages are:
    // { id: pageId, url: URL, parent: parentPage }
    windows: [],
    
    getClientId: function () {
        if (ids.clientId == null) {
            ids.clientId = fs.readFile(ids.clientIdFile);
            if (ids.clientId == null) {
                // XXX: not cryptographically secure!
                ids.clientId = Math.floor(Math.random() * 1000000);
                fs.writeFile(ids.clientIdFile, ids.clientId);
            }
        }

        return ids.clientId;
    },

    savePageId: function() {
        dbg.dbg(VERBOSE, "saving page id to " + ids.pageIdFile);
        fs.writeFile(ids.pageIdFile, ids.lastPageId);
    },

    lookupWin: function(win) {
        // search for win in window list
        for (var i in ids.windows) {
            if (ids.windows[i].win == win) 
                return ids.windows[i];
        }

        // window not found -- create a new entry
        var w = { win: win, pages: [] };
        ids.windows.push(w);
        return w;
    },

    getPageId: function(win, url) {
        dbg.assert(win, "ids.getPageId: invalid win parameter");
        dbg.assert(url, "ids.getPageId: invalid url parameter");
        
        // search for url in pages from newest to oldest entry
        var w = ids.lookupWin(win);
        for (var i = w.pages.length - 1; i >= 0; i--) {
            if (w.pages[i].url == url) 
                return w.pages[i].id;
        }

        dbg.dbg(WARN, 'page id not found for ' + url);
        return null;
    },

    getWinPageId: function(win) {
        return ids.getPageId(win, win.location);
    },

    allocatePageId: function(win, url) {
        dbg.assert(win, "ids.getPageId: invalid win parameter");
        dbg.assert(url, "ids.getPageId: invalid url parameter");

        var w = ids.lookupWin(win);
        var parentId = win.parent == win ? null : ids.getPageId(win.parent, win.parent.location);
        w.pages.push({ id: ids.lastPageId, url: url, parent: parentId });
        if (w.pages.length > 3) {
            var pageId = w.pages.shift().id;
            log.remove(pageId);
        }

        dbg.dbg(VERBOSE, "allocated page id " + ids.lastPageId + " for " + url);
        return ids.lastPageId++;
    },

    getNextReqId: function(pageId) {
        return ids.reqId++;
    },

    init: function() {
        ids.lastPageId = fs.readIntFromFile(ids.pageIdFile);
        atexit.register(ids.savePageId);
    }
};

ids.init();