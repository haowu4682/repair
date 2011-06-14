#! /usr/bin/python

import ff

ff.CTRL_TIME = 200000

if __name__ == "__main__":
    (opt, args) = ff.parse_args()

    box = [(50,200),
           (90,220),
           (210,210),
           (210,240)]
    
    # default
    if not opt.url:
        opt.url = "../tests/test.html"
    
    win = ff.launch(opt.profile, opt.url)
    
    c = ff.cmd()
    
    # btn
    c.click(50, 200)
    # box
    c.click(90, 220)
    c.type("logn time ago")
    # expand
    c.click(60, 320)
    # combo: orange
    c.click(40, 290)
    c.click(40, 413)
    # link
    c.click(50, 730)
    # back
    c.click(25, 55)
    
    c.execute(win)

    win.close()
    ff.done()
