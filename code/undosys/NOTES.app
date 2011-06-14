Web application, server-side:
    phpBB?  or ruby equivalent
    trojan of server-side code
    or install buggy plug-in/extension
    retroactively patch plug-in/extension
    audit the data leaked via bug
    undo changes from the bug (e.g. attacker's accounts, etc)

Client-side:
    cross-site scripting attacks
    record javascript inputs, replay on server-side
    ensure XSS attack didn't tamper with legitimate JS
    track causality across HTTP requests

