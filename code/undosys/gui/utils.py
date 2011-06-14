#
# utilities
#

# singleton
def The( singleton, *args ) :
    if not hasattr( singleton, "__instance" ) :
        singleton.__instance = singleton( *args )

    return singleton.__instance

def unique( ele ) :
    return list(set(ele))

def static( obj, var, init ) :
    if not hasattr( obj, var ) :
        setattr( obj, var, init )
    return getattr( obj, var )
