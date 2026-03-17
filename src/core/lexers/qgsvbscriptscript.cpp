/***************************************************************************
                       vbscriptstatement.cpp
                       ---------------------
    Implementation of QgsVBScriptScript: constructor, dump(), visitor
    accept() methods, and RecursiveVisitor helpers.
 ***************************************************************************/

#include "qgsvbscriptscript.h"

#include <QString>
#include <QStringList>

using namespace Qt::StringLiterals;

extern QgsVBScriptScript::Node *vbs_parse_string( const QString &code, QString &errorMsg );


// -----------------------------------------------------------------------
// Operator text tables
// -----------------------------------------------------------------------
const char *QgsVBScriptScript::BINARY_OP_TEXT[] = {
  "+",   "-",   "*",   "/",   "\\",  "Mod", "^", // boAdd..boPow
  "&",                                           // boConcat
  "=",   "<>",  "<",   ">",   "<=",  ">=",       // boEQ..boGE
  "And", "Or",  "Xor", "Eqv", "Imp",             // boAnd..boImp
  "Is",  "Like"                                  // boIs, boLike
};
const char *QgsVBScriptScript::UNARY_OP_TEXT[] = { "Not", "-", "+" };

// -----------------------------------------------------------------------
// QgsVBScriptScript constructor — calls vbs_parse()
// -----------------------------------------------------------------------
QgsVBScriptScript::QgsVBScriptScript( const QString &code )
  : mStatement( code )
{
  mRootNode.reset( vbs_parse_string( code, mParserErrorString ) );
}

QgsVBScriptScript::~QgsVBScriptScript() = default;

bool QgsVBScriptScript::hasParserError() const
{
  return !mParserErrorString.isEmpty();
}
QString QgsVBScriptScript::parserErrorString() const
{
  return mParserErrorString;
}

// -----------------------------------------------------------------------
// dump() implementations
// -----------------------------------------------------------------------
QString QgsVBScriptScript::NodeLiteral::dump() const
{
  switch ( mValue.type() )
  {
    case QVariant::String:
      return u"\""_s + mValue.toString() + u"\""_s;
    case QVariant::Bool:
      return mValue.toBool() ? u"True"_s : u"False"_s;
    case QVariant::Double:
    case QVariant::Int:
    case QVariant::LongLong:
      return mValue.toString();
    default:
      return u"Null"_s;
  }
}

QString QgsVBScriptScript::NodeUnaryOperator::dump() const
{
  return QString::fromLatin1( QgsVBScriptScript::UNARY_OP_TEXT[static_cast< int >( mOp )] ) + u"("_s + mOperand->dump() + u")"_s;
}

QString QgsVBScriptScript::NodeBinaryOperator::dump() const
{
  return u"("_s + mLeft->dump() + u" "_s + QString::fromLatin1( QgsVBScriptScript::BINARY_OP_TEXT[static_cast< int >( mOp )] ) + u" "_s + mRight->dump() + u")"_s;
}

QString QgsVBScriptScript::NodeMember::dump() const
{
  return mObj->dump() + u"."_s + mMember;
}

QString QgsVBScriptScript::NodeFunctionCallOrArrayIndex::dump() const
{
  return mCalleeOrBase->dump() + u"("_s + mArguments->dump() + u")"_s;
}

QString QgsVBScriptScript::NodeFunctionCall::dump() const
{
  return mCallee->dump() + u"("_s + mArguments->dump() + u")"_s;
}

QgsVBScriptScript::NodeExpressionList::~NodeExpressionList()
{
  qDeleteAll( mExpressions );
}

QString QgsVBScriptScript::NodeExpressionList::dump() const
{
  QStringList parts;
  for ( auto *e : mExpressions )
    parts << e->dump();
  return parts.join( ", "_L1 );
}

QgsVBScriptScript::NodeStatementList::~NodeStatementList()
{
  qDeleteAll( mStatements );
}

QString QgsVBScriptScript::NodeStatementList::dump() const
{
  QStringList parts;
  for ( auto *s : mStatements )
    parts << s->dump();
  return parts.join( '\n'_L1 );
}

QString QgsVBScriptScript::NodeParameterList::dump() const
{
  QStringList parts;
  for ( const auto &p : mParams )
  {
    QString s = p.byRef ? u"ByRef "_s : u"ByVal "_s;
    s += p.name;
    if ( p.isArray )
      s += "()"_L1;
    parts << s;
  }
  return parts.join( ", "_L1 );
}

QString QgsVBScriptScript::NodeAssign::dump() const
{
  QString prefix = mIsSet ? u"Set "_s : QString();
  return prefix + mLhs->dump() + u" = "_s + mRhs->dump();
}

QgsVBScriptScript::NodeDim::~NodeDim()
{
  for ( auto &d : mVars )
    delete d.dims;
}

QString QgsVBScriptScript::NodeDim::dump() const
{
  QStringList parts;
  for ( const auto &v : mVars )
  {
    QString s = v.name;
    if ( v.dims )
      s += u"("_s + v.dims->dump() + u")"_s;
    parts << s;
  }
  QString vis = mVisibility.isEmpty() ? QString() : mVisibility + u" "_s;
  return vis + u"Dim "_s + parts.join( ", "_L1 );
}

QgsVBScriptScript::NodeReDim::~NodeReDim()
{
  for ( auto &d : mVars )
    delete d.dims;
}

QString QgsVBScriptScript::NodeReDim::dump() const
{
  QStringList parts;
  for ( const auto &v : mVars )
  {
    QString s = v.name;
    if ( v.dims )
      s += u"("_s + v.dims->dump() + u")"_s;
    parts << s;
  }
  return u"ReDim "_s + ( mPreserve ? u"Preserve "_s : QString() ) + parts.join( ", "_L1 );
}

QString QgsVBScriptScript::NodeConst::dump() const
{
  return ( mVisibility.isEmpty() ? QString() : mVisibility + u" "_s ) + u"Const "_s + mName + u" = "_s + mValue->dump();
}

QgsVBScriptScript::NodeIf::~NodeIf()
{
  qDeleteAll( mElseIfs );
}

QString QgsVBScriptScript::NodeIf::dump() const
{
  QString s = u"If "_s + mCondition->dump() + u" Then\n"_s;
  s += mThen->dump();
  for ( auto *ei : mElseIfs )
    s += u"\nElseIf "_s + ei->condition()->dump() + u" Then\n"_s + ei->thenBody()->dump();
  if ( mElse )
    s += u"\nElse\n"_s + mElse->dump();
  s += "\nEnd If"_L1;
  return s;
}

QString QgsVBScriptScript::NodeSelect::dump() const
{
  QString s = u"Select Case "_s + mExpr->dump() + u"\n"_s;
  for ( auto *c : mCases )
  {
    if ( c->expressions )
      s += u"Case "_s + c->expressions->dump() + u"\n"_s;
    else
      s += "Case Else\n"_L1;
    s += c->body->dump() + u"\n"_s;
  }
  return s + u"End Select"_s;
}

QString QgsVBScriptScript::NodeFor::dump() const
{
  QString s = u"For "_s + mVar + u" = "_s + mFrom->dump() + u" To "_s + mTo->dump();
  if ( mStep )
    s += u" Step "_s + mStep->dump();
  s += u"\n"_s + mBody->dump() + u"\nNext"_s;
  return s;
}

QString QgsVBScriptScript::NodeForEach::dump() const
{
  return u"For Each "_s + mVar + u" In "_s + mCollection->dump() + u"\n"_s + mBody->dump() + u"\nNext"_s;
}

QString QgsVBScriptScript::NodeWhile::dump() const
{
  return u"While "_s + mCond->dump() + u"\n"_s + mBody->dump() + u"\nWend"_s;
}

QString QgsVBScriptScript::NodeDo::dump() const
{
  QString s = u"Do"_s;
  if ( mMode == DoMode::doWhilePre )
    s += u" While "_s + mCond->dump();
  if ( mMode == DoMode::doUntilPre )
    s += u" Until "_s + mCond->dump();
  s += u"\n"_s + mBody->dump() + u"\nLoop"_s;
  if ( mMode == DoMode::doWhilePost )
    s += u" While "_s + mCond->dump();
  if ( mMode == DoMode::doUntilPost )
    s += u" Until "_s + mCond->dump();
  return s;
}

QString QgsVBScriptScript::NodeWith::dump() const
{
  return u"With "_s + mObj->dump() + u"\n"_s + mBody->dump() + u"\nEnd With"_s;
}

QString QgsVBScriptScript::NodeOnError::dump() const
{
  return mMode == Mode::ResumeNext ? u"On Error Resume Next"_s : u"On Error GoTo 0"_s;
}

QString QgsVBScriptScript::NodeSubDef::dump() const
{
  return ( mVis.isEmpty() ? QString() : mVis + u" "_s ) + u"Sub "_s + mName + u"("_s + mParams->dump() + u")\n"_s + mBody->dump() + u"\nEnd Sub"_s;
}

QString QgsVBScriptScript::NodeFuncDef::dump() const
{
  return ( mVis.isEmpty() ? QString() : mVis + u" "_s ) + u"Function "_s + mName + u"("_s + mParams->dump() + u")\n"_s + mBody->dump() + u"\nEnd Function"_s;
}

QString QgsVBScriptScript::NodePropDef::dump() const
{
  QString kw = mKind == PropKind::pkGet ? u"Get"_s : mKind == PropKind::pkLet ? u"Let"_s : u"Set"_s;
  return ( mVis.isEmpty() ? QString() : mVis + u" "_s ) + u"Property "_s + kw + u" "_s + mName + u"("_s + mParams->dump() + u")\n"_s + mBody->dump() + u"\nEnd Property"_s;
}

QString QgsVBScriptScript::NodeClassDef::dump() const
{
  return u"Class "_s + mName + u"\n"_s + mBody->dump() + u"\nEnd Class"_s;
}

// -----------------------------------------------------------------------
// accept() implementations
// -----------------------------------------------------------------------
void QgsVBScriptScript::NodeLiteral ::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeLiteral::clone() const
{
  return new NodeLiteral( mValue );
}

void QgsVBScriptScript::NodeIdent ::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeIdent::clone() const
{
  return new NodeIdent( mName );
}

void QgsVBScriptScript::NodeUnaryOperator::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeUnaryOperator::clone() const
{
  return new NodeUnaryOperator( mOp, mOperand->clone() );
}

void QgsVBScriptScript::NodeBinaryOperator::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeBinaryOperator::clone() const
{
  return new NodeBinaryOperator( mOp, mLeft->clone(), mRight->clone() );
}

void QgsVBScriptScript::NodeMember ::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeMember::clone() const
{
  return new NodeMember( mObj->clone(), mMember );
}

void QgsVBScriptScript::NodeFunctionCallOrArrayIndex::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeFunctionCall ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeAssign ::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeAssign::clone() const
{
  return new NodeAssign( mLhs->clone(), mRhs->clone(), mIsSet );
}
void QgsVBScriptScript::NodeDim ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeReDim ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeConst ::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeConst::clone() const
{
  return new NodeConst( mVisibility, mName, mValue->clone() );
}
void QgsVBScriptScript::NodeIf ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeSelect ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeFor ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeForEach ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeWhile ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeDo ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeWith ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeExit ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeOnError ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeSubDef ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeFuncDef ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodePropDef ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeClassDef ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeStatementList ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeExpressionList ::accept( Visitor &v ) const
{
  v.visit( *this );
}
void QgsVBScriptScript::NodeParameterList::accept( Visitor &v ) const
{
  v.visit( *this );
}

QgsVBScriptScript::Node *QgsVBScriptScript::NodeParameterList::clone() const
{
  auto *n = new NodeParameterList();
  n->mParams = mParams;
  return n;
}

// -----------------------------------------------------------------------
// clone() implementations for compound nodes
// -----------------------------------------------------------------------
QgsVBScriptScript::Node *QgsVBScriptScript::NodeFunctionCallOrArrayIndex::clone() const
{
  return new NodeFunctionCallOrArrayIndex( mCalleeOrBase->clone(), static_cast<NodeExpressionList *>( mArguments->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeFunctionCall::clone() const
{
  return new NodeFunctionCall( mCallee->clone(), static_cast<NodeExpressionList *>( mArguments->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeExpressionList::clone() const
{
  auto *n = new NodeExpressionList();
  for ( auto *e : mExpressions )
    n->append( e->clone() );
  return n;
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeStatementList::clone() const
{
  auto *n = new NodeStatementList();
  for ( auto *s : mStatements )
    n->append( s->clone() );
  return n;
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeDim::clone() const
{
  auto *n = new NodeDim( mVisibility );
  for ( const auto &v : mVars )
  {
    DimVar dv = v;
    if ( v.dims )
      dv.dims = static_cast<NodeExpressionList *>( v.dims->clone() );
    n->append( dv );
  }
  return n;
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeReDim::clone() const
{
  auto *n = new NodeReDim( mPreserve );
  for ( const auto &v : mVars )
  {
    DimVar dv = v;
    if ( v.dims )
      dv.dims = static_cast<NodeExpressionList *>( v.dims->clone() );
    n->append( dv );
  }
  return n;
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeIf::clone() const
{
  QList<NodeElseIf *> eis;
  for ( auto *ei : mElseIfs )
    eis.append( new NodeElseIf( ei->condition()->clone(), static_cast<NodeStatementList *>( ei->thenBody()->clone() ) ) );
  return new NodeIf( mCondition->clone(), static_cast<NodeStatementList *>( mThen->clone() ), eis, mElse ? static_cast<NodeStatementList *>( mElse->clone() ) : nullptr );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeSelect::clone() const
{
  QList<NodeCase *> cases;
  for ( auto *c : mCases )
    cases.append( new NodeCase( c->expressions ? static_cast<NodeExpressionList *>( c->expressions->clone() ) : nullptr, static_cast<NodeStatementList *>( c->body->clone() ) ) );
  return new NodeSelect( mExpr->clone(), cases );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeFor::clone() const
{
  return new NodeFor( mVar, mFrom->clone(), mTo->clone(), mStep ? mStep->clone() : nullptr, static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeForEach::clone() const
{
  return new NodeForEach( mVar, mCollection->clone(), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeWhile::clone() const
{
  return new NodeWhile( mCond->clone(), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeDo::clone() const
{
  return new NodeDo( mMode, mCond ? mCond->clone() : nullptr, static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeWith::clone() const
{
  return new NodeWith( mObj->clone(), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeSubDef::clone() const
{
  return new NodeSubDef( mVis, mName, static_cast<NodeParameterList *>( mParams->clone() ), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeFuncDef::clone() const
{
  return new NodeFuncDef( mVis, mName, static_cast<NodeParameterList *>( mParams->clone() ), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodePropDef::clone() const
{
  return new NodePropDef( mVis, mKind, mName, static_cast<NodeParameterList *>( mParams->clone() ), static_cast<NodeStatementList *>( mBody->clone() ) );
}
QgsVBScriptScript::Node *QgsVBScriptScript::NodeClassDef::clone() const
{
  return new NodeClassDef( mName, static_cast<NodeStatementList *>( mBody->clone() ) );
}

// -----------------------------------------------------------------------
// RecursiveVisitor non-trivial overrides
// -----------------------------------------------------------------------
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeIf &n )
{
  n.condition()->accept( *this );
  n.thenBody()->accept( *this );
  for ( auto *ei : n.elseIfs() )
  {
    ei->condition()->accept( *this );
    ei->thenBody()->accept( *this );
  }
  if ( n.elseBody() )
    n.elseBody()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeSelect &n )
{
  n.expr()->accept( *this );
  for ( auto *c : n.cases() )
  {
    if ( c->expressions )
      c->expressions->accept( *this );
    c->body->accept( *this );
  }
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeFor &n )
{
  n.from()->accept( *this );
  n.to()->accept( *this );
  if ( n.step() )
    n.step()->accept( *this );
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeForEach &n )
{
  n.collection()->accept( *this );
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeWhile &n )
{
  n.condition()->accept( *this );
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeDo &n )
{
  if ( n.condition() )
    n.condition()->accept( *this );
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeWith &n )
{
  n.obj()->accept( *this );
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeSubDef &n )
{
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeFuncDef &n )
{
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodePropDef &n )
{
  n.body()->accept( *this );
}
void QgsVBScriptScript::RecursiveVisitor::visit( const NodeClassDef &n )
{
  n.body()->accept( *this );
}

QgsVBScriptScript::NodeElseIf::~NodeElseIf() = default;
