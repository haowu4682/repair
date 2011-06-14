#! /usr/bin/python

import ff

if __name__ == "__main__":
    (opt, args) = ff.parse_args()

    box = [(210,150),
           (210,180),
           (210,210),
           (210,240)]
    
    # default
    if not opt.url:
        opt.url = "../tests/textbox_test.html"
    
    win = ff.launch(opt.profile, opt.url)
    
    c = ff.cmd()
    for (x,y) in box:
        c.click(x,y)
        c.type("long time ago")
    c.execute(win)

    win.close()
    ff.done()
