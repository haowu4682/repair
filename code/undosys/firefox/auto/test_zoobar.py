#! /usr/bin/python

import ff

if __name__ == "__main__":
    (opt, args) = ff.parse_args()
    
    # default
    if not opt.url:
        opt.url = "localhost:8080"

    win = ff.launch(opt.profile, opt.url, use_default=True)
    
    c = ff.cmd()

    # register alice/alice
    c.click(45,152)
    c.click(396,238)
    c.type("alice")
    c.click(50,184)
    c.click(456,272)
    c.type("alice")
    c.click(628,268)

    # wait until loading
    c.wait(0.05)
    
    # login alice and alice
    c.click(50,184)
    c.click(456,272)
    c.type("alice")
    c.click(563,272)

    # wait until loading
    c.wait(500000)

    # navigating
    navi = ff.cmd()

    # click user/transfer
    navi.click(360,240)
    navi.click(420,240)
    navi.click(310,240)

    # logout
    logout = ff.cmd()
    logout.click(960,145)
    logout.wait(0.05)

    # execute!
    c.execute(win)
    for i in range(10):
        navi.execute(win)
    logout.execute(win)
    
    win.close()
    ff.done()