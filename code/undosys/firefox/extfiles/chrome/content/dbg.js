
const FATAL    = 0x0;
const WARN     = 0x1;
const VERBOSE  = 0x2;
const VVERBOSE = 0x3;
const VVVERBOSE = 0x4;

function AssertException(message) {
    this.message = message;
}

AssertException.prototype.toString = function () {
    return 'AssertException: ' + this.message;
}

var dbg = {
    lvlToStr: {
        0x0 : "FATAL",
        0x1 : "WARN",
        0x2 : "VERBOSE",
        0x3 : "VVERBOSE",
        0x4 : "VVVERBOSE"
    },
    
    dbglevel: VERBOSE,  // default level

    readDbgLevel: function() {
        var lvl = fs.readFile(fs.retroDir() + '/debug');
        if (lvl != null && (lvl.trim() in dbg.strToLvl)) 
            dbg.dbglevel = dbg.strToLvl[lvl.trim()];
    },

    logToErrConsole: function(msg) {
        var acs = Components.classes["@mozilla.org/consoleservice;1"].getService(Components.interfaces.nsIConsoleService);
        acs.logStringMessage(msg);
    },

    log: function(msg) {
        dump ? dump(msg + "\n") : logToErrConsole(msg);
    },

    fileFromFrame: function(stackFrame) {
        return stackFrame.split("@")[1];
    },
    
    formatStack: function(stack) {
        var ret = "";
        for (var i=1; i < stack.length; i++) {
            var fname = dbg.fileFromFrame(stack[i]);
            ret += fname ? "["+i+"] "+fname+"\n" : "";
        }

        return ret;
    },
    
    assert: function(exp, msg) {
        if (!exp) {
            throw new AssertException(msg);
        }
    },
    
    dbg: function(lvl, msg) {
        if (lvl > dbg.dbglevel) 
            return;
        
        // Format of debug log entry:
        // Timestamp : filename:line : lvl : msg
        var ts = (new Date()).getTime();
        var callstack = (new Error()).stack.split("\n");
        var fname = dbg.fileFromFrame(callstack[2]);
        var logmsg = ts + " : " + fname + " : " + dbg.lvlToStr[lvl] + " : " + msg;

        // Print stack trace for WARN and FATAL
        if (lvl <= WARN) 
            logmsg += " : STACK TRACE FOLLOWS\n" + dbg.formatStack(callstack);
        
        dbg.log(logmsg);

        if (lvl == FATAL) 
            throw new Error(msg);
    },

    init: function() {
        dbg.strToLvl = {};
        for (var i in dbg.lvlToStr) 
            dbg.strToLvl[dbg.lvlToStr[i]] = parseInt(i);
        
        dbg.readDbgLevel();
    }
};

dbg.init();