import os
import threading
import time

import os
import sys
import subprocess
import threading
import Queue
import time
import atexit
import signal


from os.path import abspath, dirname, join, exists, realpath

def file_write(fn, buf):
    fp = open(fn, "w")
    fp.write(buf)
    fp.close()

class Apache(threading.Thread):
    def __init__(self):
        super(Apache, self).__init__()
        self.port = 8080
        
        self.fifo_prefix = "/tmp/retro_fifo"
        if exists(self.fifo_in): os.unlink(self.fifo_in)
        if exists(self.fifo_out): os.unlink(self.fifo_out)
        os.mkfifo(self.fifo_in)
        os.mkfifo(self.fifo_out)

        httpddir = join(abspath(dirname(__file__)), "../test/webserv")
        httpd    = join(httpddir, "apache.sh")

        ## XXX: path to zoobardir should not have any .. in it, otherwise
        ## php-cgi seems to return a "No input file" error. Haven't fully
        ## figured out what the cause is, but it looks like an interaction
        ## between some env vars and presence of .. in the path. Specifically,
        ## do not prefix zoobardir with httpddir to make it an absolute path.
        zoobardir = realpath(join(httpddir, "zoobar"))
        print zoobardir

        print "Starting httpd:%s %s in %s" % (self.port, zoobardir, httpddir)
        os.environ['RETRO_RERUN'] = self.fifo_prefix
        self.httpd = subprocess.Popen([httpd, str(self.port), zoobardir], cwd = httpddir)
        
        self.input = Queue.Queue()

        self._stop = threading.Event()

    theone = None
    @staticmethod
    def get():
        if Apache.theone is None:
            Apache.theone = Apache()
            Apache.theone.start()
            atexit.register(Apache.shutdown)
            signal.signal(signal.SIGTERM, Apache.signalh)
        return Apache.theone

    @staticmethod
    def shutdown():
        if Apache.theone is not None:
            print "Shutting down httpd"
            Apache.theone.stop()
            Apache.theone = None
            
    @staticmethod
    def signalh(sig, frame):
        Apache.shutdown()
        exit(1)

    @property
    def fifo_in(self):
        return self.fifo_prefix + ".in"

    @property
    def fifo_out(self):
        return self.fifo_prefix + ".out"
    
    def run(self):
        req = []
        fp = open(self.fifo_out)
        while True:
            line = fp.readline().strip()
            if line == "":
                time.sleep(0.01) ## XXX: don't poll 
                continue
            req.append(line)
            if line.find(" httpreq_end ") != -1:
                self.input.put(req)
                req = []
            if line.find("THREAD_EXIT") != -1:
                fp.close()
                return
            
    def has_request(self):
        return not self.input.empty()
        
    def get_request(self):
        return self.input.get()
        
    def send_response(self, resp):
        file_write(self.fifo_in, resp)
        
    def stop(self):
        # kill apache.sh
        self.httpd.kill()
        self.httpd.wait()

        # XXX kill apache process
        os.kill(int(open("/tmp/apache2.pid").read()), 9)

        file_write(self.fifo_out, "THREAD_EXIT\n")
        self.join()

        os.unlink(self.fifo_in)
        os.unlink(self.fifo_out)

def test_httpd():
    httpd = Apache.get()
    
    resphtml = "This is a test response"
    resp = "Content-length: %d\nContent-type: text/html\n\n%s\n" % (len(resphtml), resphtml)
    
    idx = 0
    try:
        while True:
            ## Though polling is not ideal, we do it here, since otherwise 
            ## get_request blocks in Queue.Queue and a KeyboardInterrupt does
            ## not terminate this loop
            time.sleep(0.05)
            if not httpd.has_request():
                continue
            
            httpd.send_response(resp)
            req = httpd.get_request()
            print "Got request %d:\n%s" % (idx, "\n".join(req))
            idx += 1
    except KeyboardInterrupt:
        print "Stopping httpd"
        exit(0)
        
if __name__ == "__main__":
    test_httpd()
