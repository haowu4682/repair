#! /usr/bin/python

import ff

# slow response of php
ff.CTRL_TIME = 200000

if __name__ == "__main__":
    (opt, args) = ff.parse_args()
    
    # default
    if not opt.url:
        opt.url = "localhost:8080/index.php/Main_Page"

    win = ff.launch(opt.profile, opt.url, use_default=True)

    # go home
    go_home = ff.cmd()
    go_home.click(75,200)

    # define user navigation action
    main_menu = { "page": (200,160),
                  "disc": (270,160),
                  "edit": (355,160),
                  "hist": (420,160) }

    main_navi = ff.cmd()
    main_navi.click(*main_menu["page"])
    main_navi.click(*main_menu["disc"])
    main_navi.click(*main_menu["page"])
    main_navi.click(*main_menu["edit"])
    main_navi.click(*main_menu["hist"])

    # define 
    side_menu = { "main"  : (40, 320),
                  "portal": (40, 335),
                  "event" : (40, 340),
                  "recent": (40, 375),
                  "random": (40, 390),
                  "help"  : (40, 405) }

    side_navi = ff.cmd()
    side_navi.click(*side_menu["main"])
    side_navi.click(*side_menu["portal"])
    side_navi.click(*side_menu["event"])
    side_navi.click(*side_menu["recent"])
    side_navi.click(*side_menu["random"])
    side_navi.click(*side_menu["help"])
    side_navi.click(*side_menu["event"])
    side_navi.click(*side_menu["recent"])

    # search
    search = ff.cmd()
    search.dclick(100, 460)
    search.type("hello")
    search.click(40, 435)
    search.click(40, 485)
    search.wait(1)

    # edit main page
    edit_link = ff.cmd()
    edit_link.click(*main_menu["edit"])
    edit_link.click(500, 400)
    edit_link.key("Page_Down")
    edit_link.key("Page_Down")
    # damm trick to put '*'
    edit_link.shift_type("8")
    edit_link.type(" [[http://google.com]]")
    edit_link.key("Tab")
    edit_link.key("Tab")
    edit_link.key("Return")
    edit_link.wait(0.5)

    # test!
    search.execute(win)
    go_home.execute(win)

    edit_link.execute(win)
    edit_link.execute(win)
    
    for i in range(1):
        main_navi.execute(win)

    for i in range(1):
        side_navi.execute(win)
                  
    win.close()
    ff.done()