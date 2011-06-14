
if (typeof pdos == "undefined" || typeof pdos.java == "undefined") {
    throw new Error("WARNING: java.js: pdos not defined. Include util.js " +
                    "and java.js before robot.js in XUL file");
}

pdos.robot = {
    robot: null,
    
    BUTTON1_MASK 	: 16,
    BUTTON2_MASK 	: 8,
    BUTTON3_MASK 	: 4,

    init: function() {
        if (this.robot != null) {
            return;
        }
        
        pdos.u.debug(VERBOSE, "initializing robot");
        this.robot = pdos.java.newInstance("java.awt.Robot");
        pdos.u.debug(VERBOSE, "done initializing robot");
    },
    
    mousemove: function(x, y) {
        this.init();
        pdos.u.debug(VVERBOSE, "moving mouse to " + x + ", " + y);
        this.robot.mouseMove(x, y);
        pdos.u.debug(VVERBOSE, "done moving mouse to " + x + ", " + y);
    },
    
    mousedown: function() {
        this.init();
        pdos.u.debug(VVERBOSE, "clicking mouse");
        this.robot.mousePress(this.BUTTON1_MASK);
        pdos.u.debug(VVERBOSE, "done clicking mouse");
    },
    
    mouseup: function() {
        this.init();
        pdos.u.debug(VVERBOSE, "releasing mouse");
        this.robot.mouseRelease(this.BUTTON1_MASK);
        pdos.u.debug(VVERBOSE, "done releasing mouse");
    },

    mouseclick: function(x, y) {
        this.init();
        pdos.u.debug(VVERBOSE, "clicking mouse at " + x + ", " + y);
        this.robot.mouseMove(x, y);
        this.robot.mousePress(this.BUTTON1_MASK);
        this.robot.mouseRelease(this.BUTTON1_MASK);        
        pdos.u.debug(VVERBOSE, "done clicking mouse at " + x + ", " + y);
    },
    
    mousewheel: function(amount) {
        this.init();
        pdos.u.debug(VVERBOSE, "mouse scroll by " + amount + " amount");
        this.robot.mouseWheel(amount);
        pdos.u.debug(VVERBOSE, "done scrolling mouse");
    },

    keydown: function(keycode) {
        this.init();
        pdos.u.debug(VVERBOSE, "pressing key: " + keycode);
        this.robot.keyPress(keycode);
        pdos.u.debug(VVERBOSE, "done pressing key: " + keycode);
    },
    
    keyup: function(keycode) {
        this.init();
        pdos.u.debug(VVERBOSE, "releasing key: " + keycode);
        this.robot.keyRelease(keycode);
        pdos.u.debug(VVERBOSE, "done releasing key: " + keycode);
    },
    
    delay: function(millis) {
        this.init();
        pdos.u.debug(VVERBOSE, "sleeping for " + millis + "ms");
        this.robot.delay(millis);
        pdos.u.debug(VVERBOSE, "done sleeping for " + millis + "ms");
    },

    // take a screenshot of firefox window
    grab: function(pn) {
        this.init();
        pdos.u.debug(VVERBOSE, "saving firefox screenshot to " + pn);
        this.robot.setAutoWaitForIdle(true);
        var win = gBrowser.contentWindow;
        var box = new java.awt.Rectangle(win.screenX, win.screenY,
                                         win.outerWidth, win.outerHeight);
        var image = this.robot.createScreenCapture(box);
        var imgfile = new java.io.File(pn);
        Packages.javax.imageio.ImageIO.write(image, "png", imgfile);
    },
    
    /**
     * Simple unit test function that clicks at 25, 800 and types the test
     * string there. If a terminal is present at that location, it displays
     * the test string
     **/
    test: function() {
        var sleeptime = 10; // ms
        
        pdos.robot.init();
        var r = pdos.robot;
        
        r.delay(sleeptime);
        r.mouseclick(25, 800);
        
        // String is in uppercase since charCode matches keycode only for
        // uppercase chars
        var s = "THIS IS A TEST STRING TO TEST JAVA ROBOT CLASS IN JAVASCRIPT";
        for (var i in s) {
            r.delay(sleeptime);
            r.keydown(s.charCodeAt(i));
            r.keyup(s.charCodeAt(i));
        }        
    }
};

// pdos.robot.test();
