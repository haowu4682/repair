#! /usr/bin/python

import wx
import os
import sys
import time
import tarfile
import xdrlib
import record
import struct
import shutil

from wxutils     import *
from utils       import *

import wx.py as py

from wx.lib.mixins.listctrl import CheckListCtrlMixin

#
# this is for the admin
# 
#  1. searching log (to find the attacker)
#  2. managing compressed log/snapshot files seamlessly
#  3. drawing graph with chosen log entries
#  4. reexecute(undo) the attack
#
#    +---------+--------------------+--------+
#    +-+-------+-+------------------+--------+
#    | |       | |                           |
#    +-+-------+ |                           |
#    +-+-------+ |                           |
#    | |       +-+---------------------------+
#    | |       |                             |
#    +-+-------+                             |
#    | |       |                             |
#    +-+-------+-----------------------------+
#

class wxCheckList( wx.ListCtrl, CheckListCtrlMixin ):
    def __init__( self, parent, click ):
        wx.ListCtrl.__init__( self, parent, -1, style=wx.LC_REPORT )
        CheckListCtrlMixin.__init__( self )

        self.click = click
        self.Bind( wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated )

    def OnItemActivated(self, evt):
        self.ToggleItem(evt.m_itemIndex)

    # this is called by the base class when an item is checked/unchecked
    def OnCheckItem( self, index, flag ):
        data = self.GetItemData( index )
        self.click( index, data )
        
class FileListPanel( wx.Panel ):
    def __init__( self, parent, root, click ) :
        wx.Panel.__init__( self, parent, wx.ID_ANY )

        self.click = click
        self.files = []
        self.stats = []
        self.root  = root
        
        self.__init_box()

    def __init_box( self ) :
        main = wxVSizer()
        main.Add( wxText( self, self.__title() ), 0, wx.EXPAND | wx.ALL, 5 )
        main.Add( self.__init_file_list(), 1, wx.EXPAND | wx.ALL, 5 )
        self.SetSizer( main )
        
    def __init_file_list( self ) :
        self.list = wxCheckList( self, self.on_click )

        # init column
        self.list.InsertColumn( 0, "Date" )
        self.list.InsertColumn( 1, "File" )
        self.list.InsertColumn( 2, "Size", wx.LIST_FORMAT_RIGHT )

        self.refresh()
        
        return self.list

    def __title( self ) :
        return "%s (%s)" % ( self.root,
                             os.popen( "du -h %s" % self.root ).read().split()[0] )

    def __to_ctime( self, f ) :
        return time.ctime( float( ".".join(f.split(".")[0:2]) ) )

    def __to_size( self, f ) :
        return str( os.stat( f ).st_size )

    def on_click( self, index, data ) :
        self.stats[ data ] = not self.stats[ data ]
        self.click( self.files[ data ],
                    [ f for (i,f) in enumerate(self.files) if self.stats[i] ] )

    def refresh( self ) :
        self.list.DeleteAllItems()
        
        # open root directory and update file list
        self.stats = []
        self.files = []
        
        lists = []
        for (root,dirs,files) in os.walk( self.root ) :
            for f in files :
                # simple filter
                if not f.endswith( "tar.gz" ) :
                    continue
                
                abspath = os.path.join( root, f )

                self.stats.append( False )
                self.files.append( abspath )
                
                index = self.list.InsertStringItem( sys.maxint, self.__to_ctime( f ) )
                self.list.SetStringItem( index, 1, f )
                self.list.SetStringItem( index, 2, self.__to_size( abspath ) )
                self.list.SetItemData( index, len(self.files) - 1 )

        self.list.SetColumnWidth( 0, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 1, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 2, wx.LIST_AUTOSIZE )

        self.list.SortItems( lambda a,b : self.files[a] > self.files[b] )

class TarFileList( wx.Panel ):
    def __init__( self, parent ) :
        wx.Panel.__init__( self, parent, wx.ID_ANY )
        
        main = wxVSizer()
        main.Add( self.__init_file_list(), 1, wx.EXPAND | wx.ALL, 5 )
        self.SetSizer( main )
        
    def __init_file_list( self ) :
        self.list = wx.ListCtrl( self, wx.ID_ANY, style=wx.LC_REPORT )

        self.list.InsertColumn( 0, "File" )
        self.list.InsertColumn( 1, "Size", wx.LIST_FORMAT_RIGHT )
        
        return self.list

    def __to_size( self, f ) :
        return str( os.stat( f ).st_size )

    def refresh( self, tars ) :
        self.list.DeleteAllItems()
        
        self.list.Freeze()

        files = []
        for tar in tars :
            for l in tarfile.open( tar ).getmembers() :
                files.append( int(l.name.split(".")[-1]) )
            
                index = self.list.InsertStringItem( sys.maxint, l.name )
                self.list.SetStringItem( index, 1, str( l.size ) )
                self.list.SetItemData( index, len(files) - 1 )
            
        self.list.SetColumnWidth( 0, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 1, wx.LIST_AUTOSIZE )

        self.list.SortItems( lambda a,b : files[a] > files[b] )

        self.list.Thaw()

class RecordLogItem :
    def __init__( self, log, beg, end, buf, info ) :
        self.log  = log
        self.beg  = beg
        self.end  = end
        self.buf  = buf
        self.info = info

    def dump( self, out ) :
        out.write( buf )

class RecordLogList( wx.Panel ):
    def __init__( self, parent ) :
        wx.Panel.__init__( self, parent, wx.ID_ANY )
        
        main = wxVSizer()
        main.Add( self.__init_file_list(), 1, wx.EXPAND | wx.ALL, 5 )
        self.SetSizer( main )
        
    def __init_file_list( self ) :
        self.list = wx.ListCtrl( self, wx.ID_ANY, style=wx.LC_REPORT )

        self.list.InsertColumn( 0, "Time" )
        self.list.InsertColumn( 1, "Pid"  )
        self.list.InsertColumn( 2, "E" )
        self.list.InsertColumn( 3, "Name" )
        self.list.InsertColumn( 4, "Size" )
        self.list.InsertColumn( 5, "Info" )
        
        return self.list

    def __to_size( self, f ) :
        return str( os.stat( f ).st_size )

    def get_all_records( self ) :
        items = []
        
        item = -1
        while True :
            item = self.list.GetNextItem( item,
                                          wx.LIST_NEXT_ALL,
                                          wx.LIST_STATE_SELECTED )
            if item == -1 :
                break

            items.append( item )
            
        return [self.history[i] for i in items]
    
    def __parse( self, rec ) :
        if rec.scname == "write" or rec.scname == "read" :
            return "(%d,%d)=%d" % ( struct.unpack("i", rec.args[0].data[0:4])[0],
                                    struct.unpack("i", rec.args[2].data[0:4])[0],
                                    rec.ret )

        return "%d" % rec.ret

    def refresh( self, tars ) :
        self.list.DeleteAllItems()

        dlg = wx.ProgressDialog( "Undosys",
                                 "Parsing record.log",
                                 maximum = 100,
                                 parent = self,
                                 style = wx.PD_CAN_ABORT
                                 | wx.PD_APP_MODAL
                                 | wx.PD_ELAPSED_TIME
                                 | wx.PD_ESTIMATED_TIME
                                 | wx.PD_REMAINING_TIME )

        self.list.Freeze()
        
        seq = 0
        
        self.history = []
        
        files = []
        for (ntar, tar) in enumerate( tars ) :
            tarfd = tarfile.open( tar )
            
            for l in tarfd.getmembers() :
                files.append( l )
                
                unpacker = xdrlib.Unpacker( tarfd.extractfile(l).read() )
                total = [len( unpacker.get_buffer() )] * 2
                
                while True:
                    try:
                        beg = unpacker.get_position()
                        rec = record.syscall_record.unpack( unpacker )
                        end = unpacker.get_position()

                        #print beg, end, (end-beg)

                        index = self.list.InsertStringItem( sys.maxint, str(seq) )
                        self.list.SetStringItem( index, 1, str(rec.pid) )
                        self.list.SetStringItem( index, 2, ">" if rec.enter else "<" )
                        self.list.SetStringItem( index, 3, str(rec.scname) )
                        self.list.SetStringItem( index, 4, str(end-beg) )
                        self.list.SetStringItem( index, 5, self.__parse( rec ) )
                        self.list.SetItemData( index, seq )

                        # info to restore record item info
                        self.history.append( \
                            (tar, RecordLogItem( tar, beg, end,
                                                 unpacker.get_buffer()[beg:end], rec ) ) )
                        
                        seq += 1

                        total[0] -= end - beg
                        (going, skip) = dlg.Update( (total[1] - total[0]) * 100 * (ntar + 1) / total[1] / len(tars) )

                        if not going :
                            break
                        
                        # for dbg
                        # if bonce and seq > 100 :
                        #     bonce = False
                        #     self.list.Freeze()
                        
                    except EOFError:
                        break

        self.list.SetColumnWidth( 0, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 1, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 2, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 3, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 4, wx.LIST_AUTOSIZE )
        self.list.SetColumnWidth( 5, 300 )

        if seq > 100 :
            self.list.Thaw()

        dlg.Destroy()
        
class MainFrame( wx.Frame ) :
    def __init__( self, parent, title ):
        wx.Frame.__init__( self, parent, wx.ID_ANY, title, size = (1500, 1000) )
        self.SetBackgroundColour( ( 255,255,255 ) )

        self.attacker_pid = -1
        self.pickline     = -1
        
        self.record   = FileListPanel( self, "/tmp/record", self.on_record )
        self.snap     = FileListPanel( self, "/tmp/snap"  , self.on_snap   )
        self.snaplist = TarFileList( self )
        self.loglist  = RecordLogList( self )
        self.search   = wx.SearchCtrl( self, size=(200,-1) )
        
        left = wxVSizer()
        left.Add( self.record  , 1, wx.ALL | wx.EXPAND )
        left.Add( self.snap    , 1, wx.ALL | wx.EXPAND )
        left.Add( self.snaplist, 2, wx.ALL | wx.EXPAND )

        top = wxHSizer()
        top.Add( wxButton( self, "Refresh"      , self.on_refresh     ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Flush Logs"   , self.on_flush       ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Dump"         , self.on_dump        ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Graph"        , self.on_graph       ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Record Log"   , self.on_record_log  ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Add Attacker" , self.on_addattacker ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Backtrace"    , self.on_backtrace   ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Pickline"     , self.on_pickline    ), 0, wx.ALL | wx.EXPAND )
        top.Add( wxButton( self, "Restore"      , self.on_restore     ), 0, wx.ALL | wx.EXPAND )
        top.Add( self.search, 1, wx.ALL | wx.EXPAND )
         
        right = wxVSizer()
        right.Add( top, 0, wx.ALL | wx.EXPAND )
        right.Add( self.loglist, 1, wx.ALL | wx.EXPAND )

        main = wxHSizer()
        main.Add( left , 1, wx.ALL | wx.EXPAND )
        main.Add( right, 2, wx.ALL | wx.EXPAND )
        
        self.SetSizer( main )

    def on_record( self, tar, files ) :
        self.loglist.refresh( files )

    def on_snap( self, tar, files ) :
        self.snaplist.refresh( files )

    def on_refresh( self, event ) :
        self.record.refresh()
        self.snap.refresh()

        self.loglist.refresh( [] )
        self.snaplist.refresh( [] )
        
    def on_flush( self, event ) :
        # copy /proc/undosys/record
        if int(open( "/proc/undosys/stat" ).read()) != 0 :
            # flush record logs
            shutil.copyfile( "/proc/undosys/record", "%s.log" % str(time.time()) )

    def on_graph( self, event ) :
        
        self.on_record_log( None )
        
        os.system( "../graph/graph.py -e %s -o test.dot -l /tmp/record.log" %
                   ("-a %d" % self.attacker_pid if self.attacker_pid != -1 else "" ) )
        os.system( "dot -Tpng test.dot > test.png" )
        os.system( "gnome-open test.png" )

    def on_addattacker( self, event ) :
        dlg = wx.TextEntryDialog( self, "Write attacker's pid", "Undosys", "" )
        dlg.SetValue( os.popen( "cd ../graph;./pick-attacker-pid.sh trace.ascii" ).read().rstrip() )
        
        if dlg.ShowModal() == wx.ID_OK:
            self.attacker_pid = int( dlg.GetValue() )

        dlg.Destroy()

    def on_record_log( self, event ) :
        fd = open( "/tmp/record.log", "w" )
        for log in self.loglist.get_all_records() :
            fd.write( log[1].buf )
        fd.close()

    def on_restore( self, event ) :
        pass

    def on_backtrace( self, event ) :
        if self.pickline == -1 :
            self.on_pickline( event )

        os.system( "../graph/graph.py -e %s -o test.dot -l /tmp/record.log" %
                   ("-b %d" % self.pickline if self.pickline != -1 else "" ) )
        os.system( "dot -Tpng test.dot > test.png" )
        os.system( "gnome-open test.png" )

    def on_pickline( self, event ) :
        dlg = wx.TextEntryDialog( self, "Traceback line", "Undosys", "" )
        dlg.SetValue( os.popen( "cd ../graph;./pick-line.sh trace.ascii" ).read().rstrip() )
        
        if dlg.ShowModal() == wx.ID_OK:
            self.pickline = int( dlg.GetValue() )

        dlg.Destroy()

    def on_dump( self, event ) :
        self.on_record_log( event )
        os.system( "../replay/dump /tmp/record.log > ../graph/trace.ascii" )

if __name__ == "__main__" :
    app = wx.PySimpleApp()
    frm = MainFrame( None, "Undosys GUI" )
    frm.Show()
    app.MainLoop()
