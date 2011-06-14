# Adapted from simpleSQL.py that comes along with pyparsing library, with
# additions to parse simple INSERT, DELETE, and UPDATE statements
#
# simple demo of using the parsing library to do simple-minded SQL parsing
# could be extended to include where clauses etc.
#
# Copyright (c) 2003, Paul McGuire
#
from pyparsing import Literal, CaselessLiteral, Word, delimitedList, Optional, \
    Combine, Group, alphas, nums, alphanums, ParseException, Forward, oneOf, quotedString, \
    ZeroOrMore, restOfLine, Keyword, Suppress, upcaseTokens, removeQuotes

def unicodeToStr(toks): return [str(x) for x in toks]

# define SQL tokens
selectStmt = Forward()
selectToken = Keyword("select", caseless=True)
fromToken   = Keyword("from", caseless=True)

ident          = Word( alphas, alphanums + "_$" ).setName("identifier")
columnName     = ( delimitedList( ident, ".", combine=True ) ).addParseAction(upcaseTokens).addParseAction(unicodeToStr)
columnNameList = Group( delimitedList( columnName ) )
tableName      = ( delimitedList( ident, ".", combine=True ) ).addParseAction(upcaseTokens).addParseAction(unicodeToStr)
tableNameList  = Group( delimitedList( tableName ) )

whereExpression = Forward()
and_ = Keyword("and", caseless=True)
or_ = Keyword("or", caseless=True)
in_ = Keyword("in", caseless=True)

E = CaselessLiteral("E")
binop = oneOf("= != < > >= <= eq ne lt le gt ge", caseless=True)
arithSign = Word("+-",exact=1)
realNum = Combine( Optional(arithSign) + ( Word( nums ) + "." + Optional( Word(nums) )  |
                                                         ( "." + Word(nums) ) ) + 
            Optional( E + Optional(arithSign) + Word(nums) ) )
intNum = Combine( Optional(arithSign) + Word( nums ) + 
            Optional( E + Optional("+") + Word(nums) ) )

Rval     = realNum | intNum | quotedString.addParseAction(removeQuotes).addParseAction(unicodeToStr)
RvalList = Group( delimitedList( Rval ) )

columnRval = Rval | columnName # need to add support for alg expressions
whereCondition = Group(
    ( columnName + binop + columnRval ) |
    ( columnName + in_ + "(" + delimitedList( columnRval ) + ")" ) |
    ( columnName + in_ + "(" + selectStmt + ")" ) |
    ( Suppress("(") + whereExpression + Suppress(")") )
    )
whereExpression << whereCondition + ZeroOrMore( ( and_ | or_ ) + whereExpression ) 

# define the grammar
selectStmt      << ( selectToken + 
                   ( '*' | columnNameList ).setResultsName( "columns" ) + 
                   fromToken + 
                   tableNameList.setResultsName( "tables" ) + 
                   Optional( Group( CaselessLiteral("where") + whereExpression ), "" ).setResultsName("where") )

# INSERT statements
insertToken = Keyword("insert", caseless=True)
intoToken   = Keyword("into", caseless=True)
valuesToken = Keyword("values", caseless=True)

insertStmt  = ( insertToken + intoToken + tableName.setResultsName("tables") + 
                "(" + columnNameList.setResultsName("columns") + ")" +
                valuesToken + 
                "(" + RvalList.setResultsName("vals") + ")" )

# DELETE statements
deleteToken = Keyword("delete", caseless=True)

deleteStmt  = ( deleteToken + fromToken + tableName.setResultsName("tables") +
                Optional( Group( CaselessLiteral("where") + whereExpression ), "" ).setResultsName("where") )

# UPDATE statements
updateToken = Keyword("update", caseless = True)
setToken    = Keyword("set", caseless = True)
assign      = Group( columnName + Suppress("=") + Rval )
assignList  = Group( delimitedList( assign ) )

updateStmt  = ( updateToken + tableName.setResultsName("tables") +
                setToken + assignList.setResultsName("updates") +
                Optional( Group( CaselessLiteral("where") + whereExpression ), "" ).setResultsName("where") )

simpleSQL = selectStmt | insertStmt | deleteStmt | updateStmt

# define Oracle comment format, and ignore them
oracleSqlComment = "--" + restOfLine
simpleSQL.ignore( oracleSqlComment )
