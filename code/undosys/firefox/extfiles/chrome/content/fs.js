
if (typeof FileIO == "undefined") {
    throw new Error("please include io.js before fs.js");
}

var fs = {
    createDir: function (dir) {
        var fp = DirIO.open(dir);
        if (fp.exists() && fp.isDirectory()) {
            return;
        }
        
        if (!DirIO.create(fp)) {
            dbg.dbg(WARN, "error creating directory " + dir);
        }
    },

    readFile: function(fn) {
        var fp = FileIO.open(fn);
        if (!fp || !fp.exists()) {
            dbg.dbg(WARN, "error opening file " + fn);
            return null;
        }
        
        return FileIO.read(fp);
    },

    readIntFromFile: function(fn) {
        var ret = parseInt(fs.readFile(fn));
        if (isNaN(ret)) {
            dbg.dbg(WARN, "file " + fn + " doesn't contain an integer");
            ret = 0;
        }
        return ret;
    },

    writeFile: function(fn, data) {
        FileIO.write(FileIO.open(fn), "" + data);
    },

    ffProfileDir: function() {
        return DirIO.get('ProfD').path;
    },

    retroDir: function() {
        return fs.ffProfileDir() + "/retro";
    },

    repairDir: function() {
        return "/tmp/retro/logs";
    }
};