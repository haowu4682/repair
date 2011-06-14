#!/usr/bin/python

import smtpd
import asyncore
import undo
import subprocess

## XXX need to add a special SmtpUndoManager that will take care of repairing
##     the return edge from deliver_one(), by compensating for a change to
##     the past SMTP response.

@undo.redoable
def deliver_one(mailfrom, rcptto, data):
    (user, domain) = rcptto.split('@')
    args = ['procmail', '-f', mailfrom, '-d', user]
    p = subprocess.Popen(args, bufsize=len(data),
			 stdin=subprocess.PIPE,
			 stdout=subprocess.PIPE,
			 stderr=subprocess.PIPE,
			 close_fds=True)
    stdout, stderr = p.communicate(data)
    status = p.wait()
    return (status == 0)

class UndoMailServer(smtpd.SMTPServer):
    def process_message(self, peer, mailfrom, rcpttos, data):
	status = []
	for r in rcpttos:
	    status.append(deliver_one(mailfrom, r, data))
	if all(status):
	    return '250 ok'
	else:
	    return '550 failure'

s = UndoMailServer(('127.0.0.1', 8025), None)
asyncore.loop()

