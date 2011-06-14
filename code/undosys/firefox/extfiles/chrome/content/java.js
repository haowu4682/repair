
if (typeof pdos == "undefined") {
    var errmsg = "WARNING: java.js: pdos not defined. Include util.js before java.js in XUL file";
    dump(errmsg + "\n");
    throw new Error(errmsg);
}

/**
 * Based on the method to use Java in Firefox extensions described at:
 *   https://developer.mozilla.org/en/Java_in_Firefox_Extensions
 * Needs javaFirefoxExtensionUtils.jar as described in the webpage
 *
 * Also, see the following for details on LiveConnect, which is the mechanism
 * by which Javascript interfaces with Java:
 *   https://jdk6.dev.java.net/plugin2/liveconnect/
 *   http://java.sun.com/products/plugin/1.3/docs/jsobject.html
 **/
pdos.java = {
    classloader: null,
    
    /**
     * This function gives the necessary privileges to your JAR files
     **/
    policyAdd: function(loader, urls) {
        try {
            pdos.u.debug(VERBOSE, "granting permissions for URLS: ");
            for (var i in urls) {
                pdos.u.debug(VERBOSE, "URL " + i + " : " + urls[i]);
            }
            
            // If have trouble with the policy try changing it to 
            // edu.mit.simile.javaFirefoxExtensionUtils.URLSetPolicy        
            var str = 'edu.mit.simile.javaFirefoxExtensionUtils.URLSetPolicy';
            var policyClass = java.lang.Class.forName(str, true, loader);
            var policy = policyClass.newInstance();
            policy.setOuterPolicy(java.security.Policy.getPolicy());
            java.security.Policy.setPolicy(policy);
            policy.addPermission(new java.security.AllPermission());
            for (var j=0; j < urls.length; j++) {
                policy.addURL(urls[j]);
            }
            
            pdos.u.debug(VERBOSE, "done granting permissions");
        } catch(e) {
            pdos.u.debug(WARN, "error in policy add: " + e + "\n" + e.stack);
        }
    },
    
    init: function() {
        pdos.u.debug(VERBOSE, "initializing pdos.java");
        if (typeof pdos.u == "undefined") {
            throw new Error("WARNING: pdos.u not defined. Include util.js before java.js");
        }
        
        // Get extension folder installation path and append 'java' to get the
        // path containing extension's java files
        var extensionJavaPath = Components.classes["@mozilla.org/extensions/manager;1"].
                    getService(Components.interfaces.nsIExtensionManager).
                    getInstallLocation("%EXT_ID%"). // guid of extension
                    getItemLocation("%EXT_ID%");
        extensionJavaPath.append("java");
        pdos.u.debug(VERBOSE, "extension java path: " + extensionJavaPath.path);
        
        var urlArray = [];   // Class loader path components

        // Add extension java dir to class loader path
        urlArray.push(new java.net.URL("file://" + extensionJavaPath.path));
        
        // Add all jar files to class loader path
        var extensionJavaFiles = extensionJavaPath.directoryEntries;
        while (extensionJavaFiles.hasMoreElements()) {
            var fname = extensionJavaFiles.getNext();
            fname.QueryInterface(Components.interfaces.nsIFile);
            pdos.u.debug(VERBOSE, "got extension java file:" + fname.path);
            if (fname.path.match(".jar$") == ".jar") {
                pdos.u.debug(VERBOSE, "adding jar file " + fname.path + " to classpath");
                urlArray.push(new java.net.URL("file://" + fname.path));
            }
        }
        
        this.classloader = java.net.URLClassLoader.newInstance(urlArray);
             
        // Set security policies using the above policyAdd() function
        this.policyAdd(this.classloader, urlArray);

        pdos.u.debug(VERBOSE, "done initializing pdos.java");
    },
    
    /**
     * Creates a Java object which is an instance of the Java class 'className'
     * and returns it.
     * 
     * Example use: pdos.java.newInstance('java.awt.Robot');
     **/
    newInstance: function(className) {
        pdos.u.debug(VERBOSE, "creating class for " + className);
        if (this.classloader == null) {
            this.init();
        }
        
        var aClass = this.classloader.loadClass(className);
        var obj = aClass.newInstance();
        
        pdos.u.debug(VERBOSE, "done creating class for " + className);
        return obj;
    }
};
