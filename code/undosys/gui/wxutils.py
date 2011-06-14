import wx

def wrap_sizer( direction, *windows ) :
    sizer = wx.BoxSizer( direction )

    for wnd in windows :
        sizer.Add( wnd , 1, wx.EXPAND )

    return sizer

def wrap_asizer( wnd ) :
    return wrap_sizer( wx.VERTICAL, wnd )

def wxText( prnt, text ) :
    return wx.StaticText( prnt, wx.ID_ANY, text )

def wxButton( prnt, text, func ) :
    btn = wx.Button( prnt, wx.ID_ANY, text )
    prnt.Bind( wx.EVT_BUTTON, func, btn )

    return btn

def wxClock( parent ) :
    import wx.lib.analogclock as ac

    cl = ac.AnalogClock( parent,
                         style        = wx.STATIC_BORDER,
                         hoursStyle   = ac.TICKS_SQUARE,
                         minutesStyle = ac.TICKS_CIRCLE,
                         clockStyle   = ac.SHOW_HOURS_TICKS \
                             | ac.SHOW_MINUTES_TICKS        \
                             | ac.SHOW_HOURS_HAND           \
                             | ac.SHOW_MINUTES_HAND         \
                             | ac.SHOW_SECONDS_HAND )

    cl.SetTickSize( 12, target = ac.HOUR )

    return cl

def wxMsg( obj, msg ) :
    dlg = wx.MessageDialog( obj, msg, "Information", wx.OK | wx.ICON_INFORMATION )
    dlg.ShowModal()
    dlg.Destroy()

def wxChoose( obj, items, title ) :
    while True :
        dlg = wx.SingleChoiceDialog( obj, title, 'Choose one', items )
        if dlg.ShowModal() == wx.ID_OK :
            rtn = dlg.GetStringSelection()
            dlg.Destroy()
            return rtn

def wxInput( obj, question, default = '' ) :
    while True :
        dlg = wx.TextEntryDialog( obj, question, 'Input', default )
        if dlg.ShowModal() == wx.ID_OK :
            rtn = dlg.GetValue()
            dlg.Destroy()
            return rtn

def wxLabel( obj, label, wnd, label_ratio = 1, wnd_ratio = 3 ) :
    sizer = wx.BoxSizer( wx.HORIZONTAL )
    sizer.Add( wx.StaticText( obj, wx.ID_ANY, label, style = wx.ALIGN_CENTER ),
               label_ratio,
               wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER | wx.ALL, 3 )
    sizer.Add( wnd, wnd_ratio, wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER | wx.ALL, 2 )

    return sizer
        
def wxHSizer() :
    return wx.BoxSizer( wx.HORIZONTAL )

def wxVSizer() :
    return wx.BoxSizer( wx.VERTICAL )

def wxStaticBox( obj, title, dir = wx.VERTICAL ) :
    box = wx.StaticBox( obj, wx.ID_ANY, title )
    return wx.StaticBoxSizer( box, dir )
