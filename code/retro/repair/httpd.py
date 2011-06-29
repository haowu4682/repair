import os
import sys
import subprocess
import threading
import Queue
import time
import atexit
import signal

def file_write(fn, buf):
    fp = open(fn, "w")
    fp.write(buf)
    fp.close()

class Httpd(threading.Thread):
    def __init__(self):
        super(Httpd, self).__init__()
        
        self.port = 8080
        
        self.fifo_prefix = "/tmp/retro_fifo"
        if os.path.exists(self.fifo_in): os.unlink(self.fifo_in)
        if os.path.exists(self.fifo_out): os.unlink(self.fifo_out)
        os.mkfifo(self.fifo_in)
        os.mkfifo(self.fifo_out)

        thisdir = os.path.abspath(os.path.dirname(__file__))
        httpddir = os.path.join(thisdir, "../test/webserv")
        httpd = os.path.join(httpddir, "httpd")

        ## XXX: path to zoobardir should not have any .. in it, otherwise
        ## php-cgi seems to return a "No input file" error. Haven't fully
        ## figured out what the cause is, but it looks like an interaction
        ## between some env vars and presence of .. in the path. Specifically,
        ## do not prefix zoobardir with httpddir to make it an absolute path.
        zoobardir = "./zoobar"

        print "Starting httpd"
        os.environ['RETRO_RERUN'] = self.fifo_prefix
        self.httpd = subprocess.Popen([httpd, str(self.port), zoobardir], cwd = httpddir)
        
        self.input = Queue.Queue()
        
    theone = None
    @staticmethod
    def get():
        if Httpd.theone is None:
            Httpd.theone = Httpd()
            Httpd.theone.start()
            atexit.register(Httpd.shutdown)
            signal.signal(signal.SIGTERM, Httpd.signalh)
        return Httpd.theone

    @staticmethod
    def shutdown():
        if Httpd.theone is not None:
            print "Shutting down httpd"
            Httpd.theone.stop()
            Httpd.theone = None
            
    @staticmethod
    def signalh(sig, frame):
        Httpd.shutdown()
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
        self.httpd.kill()
        self.httpd.wait()
        
        file_write(self.fifo_out, "THREAD_EXIT\n")
        self.join()
        
        os.unlink(self.fifo_in)
        os.unlink(self.fifo_out)
        
def test_httpd():
    httpd = Httpd.get()
    
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
            
            req = httpd.get_request()
            print "Got request %d:\n%s" % (idx, "\n".join(req))
            idx += 1
            httpd.send_response(resp)
    except KeyboardInterrupt:
        print "Stopping httpd"
        httpd.shutdown()
        exit(0)
        
if __name__ == "__main__":
    test_httpd()
