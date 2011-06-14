#! /usr/bin/python

import ff

if __name__ == "__main__":
    (opt, args) = ff.parse_args()
    
    btn = [(50,170),
           (50,200),
           (50,230),
           (50,260)]

    # default
    if not opt.url:
        opt.url = "../tests/button_test.html"

    win = ff.launch(opt.profile, opt.url)
    
    c = ff.cmd()
    for (x,y) in btn:
        c.click(x,y)
    c.execute(win)

    win.close()
    ff.done()