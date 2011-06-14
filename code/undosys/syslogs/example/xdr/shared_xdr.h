void test_xdr( XDR * xdrs )
{
    /* OK
    int n = 10;
    xdr_int( xdrs, &n );
    */

    /* OK
    quad_t q = 10;
    xdr_quad_t( xdrs, &q );
    */

    /* OK
    bool_t b = 1;
    xdr_bool( xdrs, &b );
    */

    int i;
    syscall_record rec;

    memset(&rec, 0, sizeof(rec));

    rec.pid = 111;
    rec.scno = 3;
    rec.scname = "close";
    rec.enter = 1;
    rec.ret = 444;
    rec.err = 3;
    rec.snap_id = 0xFFFFFFFF;

    rec.ret_fd_stat   = NULL;
    rec.args.args_len = 0;
    rec.args.args_val = 0;

    /*
    rec.args.args_len = 1;
    rec.args.args_val = malloc( sizeof( syscall_arg ) * rec.args.args_len );

    memset( rec.args.args_val, 0, sizeof( syscall_arg ) * rec.args.args_len );

    for ( i = 0 ; i < rec.args.args_len ; i ++ ) {
        unsigned long l = 0xFF0000FFFE;

        rec.args.args_val[i].argv.argv_len = 0;
        rec.args.args_val[i].argv.argv_val = 0;
        rec.args.args_val[i].data.data_len = sizeof(l);
        rec.args.args_val[i].data.data_val = malloc(sizeof(l));

        memcpy(rec.args.args_val[i].data.data_val, &l, sizeof(l));

        rec.args.args_val[i].namei = malloc( sizeof( namei_infos ) );
        rec.args.args_val[i].namei->ni.ni_len = 2;
        rec.args.args_val[i].namei->ni.ni_val = malloc( sizeof( namei_info ) * 2);
        
        rec.args.args_val[i].namei->ni.ni_val[0].dev = 2;
        rec.args.args_val[i].namei->ni.ni_val[0].ino = 3;
        rec.args.args_val[i].namei->ni.ni_val[0].gen = 4;
        rec.args.args_val[i].namei->ni.ni_val[0].name = "goodluck";

        rec.args.args_val[i].namei->ni.ni_val[1].dev = 2;
        rec.args.args_val[i].namei->ni.ni_val[1].ino = 3;
        rec.args.args_val[i].namei->ni.ni_val[1].gen = 4;
        rec.args.args_val[i].namei->ni.ni_val[1].name = "dameit";
    }
    */
    xdr_syscall_record( xdrs, &rec );
}

