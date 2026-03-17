# The following has been generated automatically from src/core/lexers/qgsvbscriptscript.h
# monkey patching scoped based enum
QgsVBScriptScript.UnaryOperator.uoNot.__doc__ = "Not"
QgsVBScriptScript.UnaryOperator.uoMinus.__doc__ = "-"
QgsVBScriptScript.UnaryOperator.uoPos.__doc__ = "+"
QgsVBScriptScript.UnaryOperator.__doc__ = """Unary operators.

* ``uoNot``: Not
* ``uoMinus``: -
* ``uoPos``: +

"""
# --
# monkey patching scoped based enum
QgsVBScriptScript.BinaryOperator.boAdd.__doc__ = "+"
QgsVBScriptScript.BinaryOperator.boSubtract.__doc__ = "-"
QgsVBScriptScript.BinaryOperator.boMultiply.__doc__ = "*"
QgsVBScriptScript.BinaryOperator.boDivide.__doc__ = "/"
QgsVBScriptScript.BinaryOperator.boIntegerDivide.__doc__ = "Integer divide"
QgsVBScriptScript.BinaryOperator.boMod.__doc__ = "Mod (%)"
QgsVBScriptScript.BinaryOperator.boPower.__doc__ = "Power (^)"
QgsVBScriptScript.BinaryOperator.boConcat.__doc__ = "String concat (&)"
QgsVBScriptScript.BinaryOperator.boEQ.__doc__ = "="
QgsVBScriptScript.BinaryOperator.boNE.__doc__ = "<>"
QgsVBScriptScript.BinaryOperator.boLT.__doc__ = "<"
QgsVBScriptScript.BinaryOperator.boGT.__doc__ = ">"
QgsVBScriptScript.BinaryOperator.boLE.__doc__ = "<="
QgsVBScriptScript.BinaryOperator.boGE.__doc__ = ">="
QgsVBScriptScript.BinaryOperator.boAnd.__doc__ = "And"
QgsVBScriptScript.BinaryOperator.boOr.__doc__ = "Or"
QgsVBScriptScript.BinaryOperator.boXor.__doc__ = "Xor"
QgsVBScriptScript.BinaryOperator.boEqv.__doc__ = "Eqv"
QgsVBScriptScript.BinaryOperator.boImp.__doc__ = "Imp"
QgsVBScriptScript.BinaryOperator.boIs.__doc__ = "Is"
QgsVBScriptScript.BinaryOperator.boLike.__doc__ = "Like"
QgsVBScriptScript.BinaryOperator.__doc__ = """Binary operators.

* ``boAdd``: +
* ``boSubtract``: -
* ``boMultiply``: *
* ``boDivide``: /
* ``boIntegerDivide``: Integer divide
* ``boMod``: Mod (%)
* ``boPower``: Power (^)
* ``boConcat``: String concat (&)
* ``boEQ``: =
* ``boNE``: <>
* ``boLT``: <
* ``boGT``: >
* ``boLE``: <=
* ``boGE``: >=
* ``boAnd``: And
* ``boOr``: Or
* ``boXor``: Xor
* ``boEqv``: Eqv
* ``boImp``: Imp
* ``boIs``: Is
* ``boLike``: Like

"""
# --
# monkey patching scoped based enum
QgsVBScriptScript.NodeType.ntLiteral.__doc__ = "Literal value, eg integer / float / string / date / bool / Empty / Nothing / Null"
QgsVBScriptScript.NodeType.ntIdent.__doc__ = "Variable / function name"
QgsVBScriptScript.NodeType.ntMe.__doc__ = "\"Me\" node"
QgsVBScriptScript.NodeType.ntNew.__doc__ = "\"New\""
QgsVBScriptScript.NodeType.ntUnaryOp.__doc__ = "Unary operator"
QgsVBScriptScript.NodeType.ntBinaryOp.__doc__ = "Binary operator"
QgsVBScriptScript.NodeType.ntMember.__doc__ = "Object member (e.g. obj.member)"
QgsVBScriptScript.NodeType.ntFunction.__doc__ = "Function call or array index (e.g. arr(i) or arr(i,j))"
QgsVBScriptScript.NodeType.ntCall.__doc__ = "Call function statement"
QgsVBScriptScript.NodeType.ntAssign.__doc__ = "Assignment (let)"
QgsVBScriptScript.NodeType.ntSetAssign.__doc__ = "Assignment (set)"
QgsVBScriptScript.NodeType.ntDim.__doc__ = "Declare array"
QgsVBScriptScript.NodeType.ntReDim.__doc__ = "Redeclare array"
QgsVBScriptScript.NodeType.ntConst.__doc__ = "Const statement"
QgsVBScriptScript.NodeType.ntIf.__doc__ = "If block"
QgsVBScriptScript.NodeType.ntElseIf.__doc__ = "Else if block"
QgsVBScriptScript.NodeType.ntSelect.__doc__ = "Select block"
QgsVBScriptScript.NodeType.ntCase.__doc__ = "Case block"
QgsVBScriptScript.NodeType.ntFor.__doc__ = "For loop"
QgsVBScriptScript.NodeType.ntForEach.__doc__ = "For each loop"
QgsVBScriptScript.NodeType.ntWhile.__doc__ = "While loop"
QgsVBScriptScript.NodeType.ntDo.__doc__ = "Do loop"
QgsVBScriptScript.NodeType.ntWith.__doc__ = "With statement"
QgsVBScriptScript.NodeType.ntExit.__doc__ = "Exit statement"
QgsVBScriptScript.NodeType.ntOnError.__doc__ = "On error statement"
QgsVBScriptScript.NodeType.ntErase.__doc__ = "Erase statement"
QgsVBScriptScript.NodeType.ntRandomize.__doc__ = "Randomize statement"
QgsVBScriptScript.NodeType.ntStop.__doc__ = "Stop statement"
QgsVBScriptScript.NodeType.ntOptionExplicit.__doc__ = "Option explicit"
QgsVBScriptScript.NodeType.ntSubDef.__doc__ = "Subroutine definition"
QgsVBScriptScript.NodeType.ntFuncDef.__doc__ = "Function definition"
QgsVBScriptScript.NodeType.ntPropGet.__doc__ = "Retrieve property"
QgsVBScriptScript.NodeType.ntPropLet.__doc__ = "Let property"
QgsVBScriptScript.NodeType.ntPropSet.__doc__ = "Set property"
QgsVBScriptScript.NodeType.ntClassDef.__doc__ = "Class definition"
QgsVBScriptScript.NodeType.ntCallStmt.__doc__ = "Call statement"
QgsVBScriptScript.NodeType.ntStmtList.__doc__ = "List of statements"
QgsVBScriptScript.NodeType.ntExprList.__doc__ = "List of expressions"
QgsVBScriptScript.NodeType.ntParamList.__doc__ = "List of parameters"
QgsVBScriptScript.NodeType.ntDimList.__doc__ = "Dimension list"
QgsVBScriptScript.NodeType.ntCaseList.__doc__ = "Case list"
QgsVBScriptScript.NodeType.ntElseIfList.__doc__ = "Else if list"
QgsVBScriptScript.NodeType.__doc__ = """Node types.

* ``ntLiteral``: Literal value, eg integer / float / string / date / bool / Empty / Nothing / Null
* ``ntIdent``: Variable / function name
* ``ntMe``: \"Me\" node
* ``ntNew``: \"New\"
* ``ntUnaryOp``: Unary operator
* ``ntBinaryOp``: Binary operator
* ``ntMember``: Object member (e.g. obj.member)
* ``ntFunction``: Function call or array index (e.g. arr(i) or arr(i,j))
* ``ntCall``: Call function statement
* ``ntAssign``: Assignment (let)
* ``ntSetAssign``: Assignment (set)
* ``ntDim``: Declare array
* ``ntReDim``: Redeclare array
* ``ntConst``: Const statement
* ``ntIf``: If block
* ``ntElseIf``: Else if block
* ``ntSelect``: Select block
* ``ntCase``: Case block
* ``ntFor``: For loop
* ``ntForEach``: For each loop
* ``ntWhile``: While loop
* ``ntDo``: Do loop
* ``ntWith``: With statement
* ``ntExit``: Exit statement
* ``ntOnError``: On error statement
* ``ntErase``: Erase statement
* ``ntRandomize``: Randomize statement
* ``ntStop``: Stop statement
* ``ntOptionExplicit``: Option explicit
* ``ntSubDef``: Subroutine definition
* ``ntFuncDef``: Function definition
* ``ntPropGet``: Retrieve property
* ``ntPropLet``: Let property
* ``ntPropSet``: Set property
* ``ntClassDef``: Class definition
* ``ntCallStmt``: Call statement
* ``ntStmtList``: List of statements
* ``ntExprList``: List of expressions
* ``ntParamList``: List of parameters
* ``ntDimList``: Dimension list
* ``ntCaseList``: Case list
* ``ntElseIfList``: Else if list

"""
# --
# monkey patching scoped based enum
QgsVBScriptScript.DoMode.doInfinite.__doc__ = ""
QgsVBScriptScript.DoMode.doWhilePre.__doc__ = ""
QgsVBScriptScript.DoMode.doUntilPre.__doc__ = ""
QgsVBScriptScript.DoMode.doWhilePost.__doc__ = ""
QgsVBScriptScript.DoMode.doUntilPost.__doc__ = ""
QgsVBScriptScript.DoMode.__doc__ = """

* ``doInfinite``: 
* ``doWhilePre``: 
* ``doUntilPre``: 
* ``doWhilePost``: 
* ``doUntilPost``: 

"""
# --
# monkey patching scoped based enum
QgsVBScriptScript.NodeOnError.Mode.ResumeNext.__doc__ = ""
QgsVBScriptScript.NodeOnError.Mode.GoTo0.__doc__ = ""
QgsVBScriptScript.NodeOnError.Mode.__doc__ = """

* ``ResumeNext``: 
* ``GoTo0``: 

"""
# --
# monkey patching scoped based enum
QgsVBScriptScript.PropKind.pkGet.__doc__ = ""
QgsVBScriptScript.PropKind.pkLet.__doc__ = ""
QgsVBScriptScript.PropKind.pkSet.__doc__ = ""
QgsVBScriptScript.PropKind.__doc__ = """

* ``pkGet``: 
* ``pkLet``: 
* ``pkSet``: 

"""
# --
try:
    QgsVBScriptScript.Parameter.__attribute_docs__ = {'name': 'Parameter name', 'byRef': 'Parameter is passed by reference', 'isArray': 'Parameter is an array'}
    QgsVBScriptScript.Parameter.__annotations__ = {'name': str, 'byRef': bool, 'isArray': bool}
    QgsVBScriptScript.Parameter.__doc__ = """Represents an individual subroutine or function parameter."""
    QgsVBScriptScript.Parameter.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.DimVar.__attribute_docs__ = {'name': 'Variable name', 'dims': 'Dimension nodes'}
    QgsVBScriptScript.DimVar.__annotations__ = {'name': str, 'dims': 'QgsVBScriptScript.NodeExpressionList'}
    QgsVBScriptScript.DimVar.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.Node.__abstract_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.Node.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.Visitor.__abstract_methods__ = ['visit']
    QgsVBScriptScript.Visitor.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeLiteral.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeLiteral.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeIdent.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeIdent.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeUnaryOperator.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeUnaryOperator.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeBinaryOperator.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeBinaryOperator.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeMember.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeMember.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeFunctionCallOrArrayIndex.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeFunctionCallOrArrayIndex.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeFunctionCall.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeFunctionCall.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeExpressionList.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeExpressionList.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeStatementList.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeStatementList.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeParameterList.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeParameterList.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeAssign.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeAssign.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeDim.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeDim.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeReDim.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeReDim.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeConst.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeConst.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeIf.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeIf.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeSelect.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeSelect.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeFor.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeFor.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeForEach.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeForEach.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeWhile.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeWhile.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeDo.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeDo.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeWith.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeWith.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeExit.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeExit.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeOnError.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeOnError.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeSubDef.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeSubDef.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeFuncDef.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeFuncDef.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodePropDef.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodePropDef.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeClassDef.__overridden_methods__ = ['nodeType', 'dump', 'accept', 'clone']
    QgsVBScriptScript.NodeClassDef.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.RecursiveVisitor.__overridden_methods__ = ['visit']
    QgsVBScriptScript.RecursiveVisitor.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeElseIf.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
try:
    QgsVBScriptScript.NodeCase.__group__ = ['lexers']
except (NameError, AttributeError):
    pass
