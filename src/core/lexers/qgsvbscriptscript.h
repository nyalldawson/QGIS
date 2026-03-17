/***************************************************************************
                               qgsvbscriptscript.h
                             ---------------------
    begin                : March 2026
    copyright            : (C) 2025 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSCRIPTSTATEMENT_H
#define QGSVBSCRIPTSTATEMENT_H

#include <memory>

#include "qgis_core.h"
#include "qgis_sip.h"

#include <QList>
#include <QString>
#include <QVariant>

using namespace Qt::StringLiterals;

/**
 * \ingroup core
 * \brief Parses complete (multi-statement) VBScript scripts.
 * \since QGIS 4.2
 */
class CORE_EXPORT QgsVBScriptScript
{
  public:
    /**
     * Creates a new script based on the provided string.
     */
    explicit QgsVBScriptScript( const QString &code );
    ~QgsVBScriptScript();

    /**
     * Returns TRUE if an error occurred when parsing the input statement.
     *
     * \see parserErrorString()
     */
    bool hasParserError() const;

    /**
     * Returns parser error string, if an error occurred while parsing.
     *
     * \see hasParserError()
     */
    QString parserErrorString() const;

    /**
     * Unary operators.
     */
    enum class UnaryOperator : int
    {
      uoNot,   //!< Not
      uoMinus, //!< -
      uoPos    //!< +
    };

    /**
     * Binary operators.
     */
    enum class BinaryOperator : int
    {
      // Arithmetic
      boAdd,           //!< +
      boSubtract,      //!< -
      boMultiply,      //!< *
      boDivide,        //!< /
      boIntegerDivide, //!< Integer divide
      boMod,           //!< Mod (%)
      boPower,         //!< Power (^)
      // String
      boConcat, //!< String concat (&)
      // Comparison
      boEQ, //!< =
      boNE, //!< <>
      boLT, //!< <
      boGT, //!< >
      boLE, //!< <=
      boGE, //!< >=
      // Logical
      boAnd, //!< And
      boOr,  //!< Or
      boXor, //!< Xor
      boEqv, //!< Eqv
      boImp, //!< Imp
      // Object
      boIs,  //!< Is
      boLike //!< Like
    };

    SIP_SKIP static const char *BINARY_OP_TEXT[];
    SIP_SKIP static const char *UNARY_OP_TEXT[];

    /**
     * Node types.
     */
    enum class NodeType : int
    {
      ntLiteral,        //!< Literal value, eg integer / float / string / date / bool / Empty / Nothing / Null
      ntIdent,          //!< Variable / function name
      ntMe,             //!< "Me" node
      ntNew,            //!< "New"
      ntUnaryOp,        //!< Unary operator
      ntBinaryOp,       //!< Binary operator
      ntMember,         //!< Object member (e.g. obj.member)
      ntFunction,       //!< Function call or array index (e.g. arr(i) or arr(i,j))
      ntCall,           //!< Call function statement
      ntAssign,         //!< Assignment (let)
      ntSetAssign,      //!< Assignment (set)
      ntDim,            //!< Declare array
      ntReDim,          //!< Redeclare array
      ntConst,          //!< Const statement
      ntIf,             //!< If block
      ntElseIf,         //!< Else if block
      ntSelect,         //!< Select block
      ntCase,           //!< Case block
      ntFor,            //!< For loop
      ntForEach,        //!< For each loop
      ntWhile,          //!< While loop
      ntDo,             //!< Do loop
      ntWith,           //!< With statement
      ntExit,           //!< Exit statement
      ntOnError,        //!< On error statement
      ntErase,          //!< Erase statement
      ntRandomize,      //!< Randomize statement
      ntStop,           //!< Stop statement
      ntOptionExplicit, //!< Option explicit
      ntSubDef,         //!< Subroutine definition
      ntFuncDef,        //!< Function definition
      ntPropGet,        //!< Retrieve property
      ntPropLet,        //!< Let property
      ntPropSet,        //!< Set property
      ntClassDef,       //!< Class definition
      ntCallStmt,       //!< Call statement
      ntStmtList,       //!< List of statements
      ntExprList,       //!< List of expressions
      ntParamList,      //!< List of parameters
      ntDimList,        //!< Dimension list
      ntCaseList,       //!< Case list
      ntElseIfList,     //!< Else if list
    };

    class Node;
    class NodeLiteral;
    class NodeIdent;
    class NodeUnaryOperator;
    class NodeBinaryOperator;
    class NodeMember;
    class NodeFunctionCallOrArrayIndex;
    class NodeFunctionCall;
    class NodeAssign;
    class NodeDim;
    class NodeReDim;
    class NodeConst;
    class NodeIf;
    class NodeSelect;
    class NodeFor;
    class NodeForEach;
    class NodeWhile;
    class NodeDo;
    class NodeWith;
    class NodeExit;
    class NodeOnError;
    class NodeSubDef;
    class NodeFuncDef;
    class NodePropDef;
    class NodeClassDef;
    class NodeStatementList;
    class NodeExpressionList;
    class Visitor;

    /**
     * Returns the root node of the statement.
     *
     * The root node is NULLPTR if parsing has failed.
     */
    const QgsVBScriptScript::Node *rootNode() const { return mRootNode.get(); }

    /**
     * \ingroup core
     * \brief Abstract node class for VBScript script nodes.
     */
    class CORE_EXPORT Node
    {
#ifdef SIP_RUN
        SIP_CONVERT_TO_SUBCLASS_CODE
        switch ( sipCpp->nodeType() )
        {
          case QgsVBScriptScript::NodeType::ntLiteral:
            sipType = sipType_QgsVBScriptScript_NodeLiteral;
            break;
          case QgsVBScriptScript::NodeType::ntIdent:
            sipType = sipType_QgsVBScriptScript_NodeIdent;
            break;
          case QgsVBScriptScript::NodeType::ntMe:
            sipType = sipType_QgsVBScriptScript_NodeIdent;
            break;
          case QgsVBScriptScript::NodeType::ntNew:
            sipType = sipType_QgsVBScriptScript_NodeFunctionCall;
            break;
          case QgsVBScriptScript::NodeType::ntUnaryOp:
            sipType = sipType_QgsVBScriptScript_NodeUnaryOperator;
            break;
          case QgsVBScriptScript::NodeType::ntBinaryOp:
            sipType = sipType_QgsVBScriptScript_NodeBinaryOperator;
            break;
          case QgsVBScriptScript::NodeType::ntMember:
            sipType = sipType_QgsVBScriptScript_NodeMember;
            break;
          case QgsVBScriptScript::NodeType::ntFunction:
            sipType = sipType_QgsVBScriptScript_NodeFunctionCallOrArrayIndex;
            break;
          case QgsVBScriptScript::NodeType::ntCall:
            sipType = sipType_QgsVBScriptScript_NodeFunctionCall;
            break;
          case QgsVBScriptScript::NodeType::ntAssign:
          case QgsVBScriptScript::NodeType::ntSetAssign:
            sipType = sipType_QgsVBScriptScript_NodeAssign;
            break;
          case QgsVBScriptScript::NodeType::ntDim:
            sipType = sipType_QgsVBScriptScript_NodeDim;
            break;
          case QgsVBScriptScript::NodeType::ntReDim:
            sipType = sipType_QgsVBScriptScript_NodeReDim;
            break;
          case QgsVBScriptScript::NodeType::ntConst:
            sipType = sipType_QgsVBScriptScript_NodeConst;
            break;
          case QgsVBScriptScript::NodeType::ntIf:
            sipType = sipType_QgsVBScriptScript_NodeIf;
            break;
          case QgsVBScriptScript::NodeType::ntElseIf:
            sipType = sipType_QgsVBScriptScript_NodeIf;
            break;
          case QgsVBScriptScript::NodeType::ntSelect:
            sipType = sipType_QgsVBScriptScript_NodeSelect;
            break;
          case QgsVBScriptScript::NodeType::ntCase:
            sipType = sipType_QgsVBScriptScript_NodeSelect;
            break;
          case QgsVBScriptScript::NodeType::ntFor:
            sipType = sipType_QgsVBScriptScript_NodeFor;
            break;
          case QgsVBScriptScript::NodeType::ntForEach:
            sipType = sipType_QgsVBScriptScript_NodeForEach;
            break;
          case QgsVBScriptScript::NodeType::ntWhile:
            sipType = sipType_QgsVBScriptScript_NodeWhile;
            break;
          case QgsVBScriptScript::NodeType::ntDo:
            sipType = sipType_QgsVBScriptScript_NodeDo;
            break;
          case QgsVBScriptScript::NodeType::ntWith:
            sipType = sipType_QgsVBScriptScript_NodeWith;
            break;
          case QgsVBScriptScript::NodeType::ntExit:
            sipType = sipType_QgsVBScriptScript_NodeExit;
            break;
          case QgsVBScriptScript::NodeType::ntOnError:
            sipType = sipType_QgsVBScriptScript_NodeOnError;
            break;
          case QgsVBScriptScript::NodeType::ntErase:
          case QgsVBScriptScript::NodeType::ntRandomize:
          case QgsVBScriptScript::NodeType::ntStop:
          case QgsVBScriptScript::NodeType::ntOptionExplicit:
            sipType = sipType_QgsVBScriptScript_NodeIdent;
            break;
          case QgsVBScriptScript::NodeType::ntSubDef:
            sipType = sipType_QgsVBScriptScript_NodeSubDef;
            break;
          case QgsVBScriptScript::NodeType::ntFuncDef:
            sipType = sipType_QgsVBScriptScript_NodeFuncDef;
            break;
          case QgsVBScriptScript::NodeType::ntPropGet:
          case QgsVBScriptScript::NodeType::ntPropLet:
          case QgsVBScriptScript::NodeType::ntPropSet:
            sipType = sipType_QgsVBScriptScript_NodePropDef;
            break;
          case QgsVBScriptScript::NodeType::ntClassDef:
            sipType = sipType_QgsVBScriptScript_NodeClassDef;
            break;
          case QgsVBScriptScript::NodeType::ntCallStmt:
            sipType = sipType_QgsVBScriptScript_NodeFunctionCall;
            break;
          case QgsVBScriptScript::NodeType::ntStmtList:
            sipType = sipType_QgsVBScriptScript_NodeStatementList;
            break;
          case QgsVBScriptScript::NodeType::ntExprList:
            sipType = sipType_QgsVBScriptScript_NodeExpressionList;
            break;
          case QgsVBScriptScript::NodeType::ntParamList:
            sipType = sipType_QgsVBScriptScript_NodeParameterList;
            break;
          case QgsVBScriptScript::NodeType::ntDimList:
            sipType = sipType_QgsVBScriptScript_NodeDim;
            break;
          case QgsVBScriptScript::NodeType::ntCaseList:
            sipType = sipType_QgsVBScriptScript_NodeSelect;
            break;
          case QgsVBScriptScript::NodeType::ntElseIfList:
            sipType = sipType_QgsVBScriptScript_NodeIf;
            break;
          default:
            sipType = nullptr;
            break;
        }
      SIP_END
#endif
      public:
        virtual ~Node() = default;

        /**
         * Returns the type of the node.
         */
        virtual NodeType nodeType() const = 0;

        /**
         * Returns a statement which represents this node as string.
         */
        virtual QString dump() const = 0;

        /**
         * Supports the visitor pattern.
         *
         * For any implementation this should look like
         *
         * \code{.cpp}
         *   v.visit( *this );
         * \endcode
         *
         * \code{py}
         *   v.visit(self)
         * \endcode
         *
         * \param v A visitor that visits this node.
         */
        virtual void accept( Visitor &v ) const = 0;

        /**
         * Generate a clone of this node.
         *
         * Ownership is transferred to the caller.
         *
         * \returns a deep copy of this node.
         */
        virtual Node *clone() const = 0 SIP_FACTORY;
    };

    /**
     * \ingroup core
     * \brief Literal value (integer, float, string, date, bool, Empty, Nothing, Null).
     */
    class CORE_EXPORT NodeLiteral : public Node
    {
      public:
        //! Constructor
        explicit NodeLiteral( const QVariant &val )
          : mValue( val )
        {}

        //! Returns the value of the literal.
        QVariant value() const { return mValue; }

        NodeType nodeType() const override { return NodeType::ntLiteral; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
        QVariant mValue;
    };

    /**
     * \ingroup core
     * \brief Identifier node (eg variable name).
     */
    class CORE_EXPORT NodeIdent : public Node
    {
      public:
        //! Constructor
        explicit NodeIdent( const QString &name )
          : mName( name )
        {}

        /**
         * Returns the identifier name.
         */
        QString name() const { return mName; }
        NodeType nodeType() const override { return NodeType::ntIdent; }
        QString dump() const override { return mName; }
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
        QString mName;
    };

    /**
     * \ingroup core
     * \brief Unary logical/arithmetical operator ( NOT, - ).
     */
    class CORE_EXPORT NodeUnaryOperator : public Node
    {
      public:
        //! Constructor
        NodeUnaryOperator( UnaryOperator op, Node *operand SIP_TRANSFER )
          : mOp( op )
          , mOperand( operand )
        {}

        //! Returns the operator
        UnaryOperator op() const { return mOp; }
        //! Returns the operand
        Node *operand() const { return mOperand.get(); }

        NodeType nodeType() const override { return NodeType::ntUnaryOp; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeUnaryOperator( const NodeUnaryOperator &other );
#endif

        UnaryOperator mOp;
        std::unique_ptr<Node> mOperand;
    };

    /**
     * \ingroup core
     * \brief Binary logical/arithmetical operator (AND, OR, =, +, ...).
     */
    class CORE_EXPORT NodeBinaryOperator : public Node
    {
      public:
        //! Constructor
        NodeBinaryOperator( BinaryOperator op, Node *left SIP_TRANSFER, Node *right SIP_TRANSFER )
          : mOp( op )
          , mLeft( left )
          , mRight( right )
        {}

        //! Operator
        BinaryOperator op() const { return mOp; }

        //! Left operand
        Node *left() const { return mLeft.get(); }
        //! Right operand
        Node *right() const { return mRight.get(); }

        NodeType nodeType() const override { return NodeType::ntBinaryOp; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeBinaryOperator( const NodeBinaryOperator &other );
#endif

        BinaryOperator mOp;
        std::unique_ptr<Node> mLeft;
        std::unique_ptr<Node> mRight;
    };

    /**
     * \ingroup core
     * \brief Object member (e.g. obj.member).
     */
    class CORE_EXPORT NodeMember : public Node
    {
      public:
        //! Constructor
        NodeMember( Node *obj SIP_TRANSFER, const QString &member )
          : mObj( obj )
          , mMember( member )
        {}

        //! Returns the object
        Node *obj() const { return mObj.get(); }
        //! Returns the name of the member
        QString member() const { return mMember; }

        NodeType nodeType() const override { return NodeType::ntMember; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeMember( const NodeMember &other );
#endif

        std::unique_ptr<Node> mObj;
        QString mMember;
    };

    /**
     * \ingroup core
     * \brief Function call or array index (e.g. arr(i) or arr(i,j)).
     *
     * \warning Usage of this node is ambiguous, and depends on whether the callee
     * refers to a defined function name or array name.
     */
    class CORE_EXPORT NodeFunctionCallOrArrayIndex : public Node
    {
      public:
        //! Constructor
        NodeFunctionCallOrArrayIndex( Node *calleeOrBase SIP_TRANSFER, QgsVBScriptScript::NodeExpressionList *arguments SIP_TRANSFER )
          : mCalleeOrBase( calleeOrBase )
          , mArguments( arguments )
        {}

        //! Returns the name of the function or array
        QgsVBScriptScript::Node *calleeOrBase() const { return mCalleeOrBase.get(); }

        //! Returns the arguments for the function, or array indices
        QgsVBScriptScript::NodeExpressionList *arguments() const { return mArguments.get(); }

        QgsVBScriptScript::NodeType nodeType() const override { return NodeType::ntFunction; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeFunctionCallOrArrayIndex( const NodeFunctionCallOrArrayIndex &other );
#endif

        std::unique_ptr<Node> mCalleeOrBase;
        std::unique_ptr<NodeExpressionList> mArguments;
    };

    /**
     * \ingroup core
     * \brief Function call.
     */
    class CORE_EXPORT NodeFunctionCall : public Node
    {
      public:
        NodeFunctionCall( Node *callee SIP_TRANSFER, QgsVBScriptScript::NodeExpressionList *arguments SIP_TRANSFER )
          : mCallee( callee )
          , mArguments( arguments )
        {}

        //! Retrurns the callee (function being called)
        QgsVBScriptScript::Node *callee() const { return mCallee.get(); }

        //! Returns the arguments for the function
        QgsVBScriptScript::NodeExpressionList *arguments() const { return mArguments.get(); }

        QgsVBScriptScript::NodeType nodeType() const override { return NodeType::ntCall; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        QgsVBScriptScript::Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeFunctionCall( const NodeFunctionCall &other );
#endif

        std::unique_ptr<Node> mCallee;
        std::unique_ptr<NodeExpressionList> mArguments;
    };

    /**
     * \ingroup core
     * \brief Expression list (arguments, array dimensions, case expressions, etc).
     */
    class CORE_EXPORT NodeExpressionList : public Node
    {
      public:
        NodeExpressionList() = default;
        ~NodeExpressionList() override;

        /**
         * Appends a node to the list.
         */
        void append( QgsVBScriptScript::Node *node SIP_TRANSFER ) { mExpressions.append( node ); }

        /**
         * Prepends node to the list.
         */
        void prepend( QgsVBScriptScript::Node *node SIP_TRANSFER ) { mExpressions.prepend( node ); }

        /**
         * Returns the list of expressions contained in the node.
         */
        QList<QgsVBScriptScript::Node *> expressions() const { return mExpressions; }

        /**
         * Returns the total number of expressions contained in the node.
         */
        int count() const { return mExpressions.count(); }

        QgsVBScriptScript::NodeType nodeType() const override { return NodeType::ntExprList; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        QgsVBScriptScript::Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeExpressionList( const NodeExpressionList &other );
#endif

        QList<Node *> mExpressions;
    };

    /**
     * \ingroup core
     * \brief Statement list (body of subroutines, functions, if branches, etc).
     */
    class CORE_EXPORT NodeStatementList : public Node
    {
      public:
        NodeStatementList() = default;
        ~NodeStatementList() override;

        /**
         * Appends a node to the list.
         */
        void append( Node *node SIP_TRANSFER ) { mStatements.append( node ); }

        /**
         * Returns the list of statements contained in the node.
         */
        QList<QgsVBScriptScript::Node *> statements() const { return mStatements; }

        NodeType nodeType() const override { return NodeType::ntStmtList; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeStatementList( const NodeStatementList &other );
#endif

        QList<Node *> mStatements;
    };

    /**
     * \ingroup core
     * \brief Represents an individual subroutine or function parameter.
     */
    struct CORE_EXPORT Parameter
    {
        //! Parameter name
        QString name;
        //! Parameter is passed by reference
        bool byRef = true;
        //! Parameter is an array
        bool isArray = false;
    };

    /**
     * \ingroup core
     * \brief Represents a list of subroutine or function parameters.
     */
    class CORE_EXPORT NodeParameterList : public Node
    {
      public:
        NodeParameterList() = default;

        /**
         * Appends a parameter to the list
         */
        void append( const Parameter &parameter ) { mParams.append( parameter ); }

        /**
         * Returns the list of parameters contained in the node.
         */
        QList<QgsVBScriptScript::Parameter> parameters() const { return mParams; }

        NodeType nodeType() const override { return NodeType::ntParamList; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
        QList<Parameter> mParams;
    };

    /**
     * \ingroup core
     * \brief An assignment node.
     */
    class CORE_EXPORT NodeAssign : public Node
    {
      public:
        //! Constructor
        NodeAssign( Node *lhs SIP_TRANSFER, Node *rhs SIP_TRANSFER, bool isSet = false )
          : mLhs( lhs )
          , mRhs( rhs )
          , mIsSet( isSet )
        {}

        //! Returns the left hand side of the assignment
        Node *lhs() const { return mLhs.get(); }
        //! Returns the right hand side of the assignment
        Node *rhs() const { return mRhs.get(); }

        //! Returns TRUE if the node is a "SET" assignment
        bool isSet() const { return mIsSet; }

        NodeType nodeType() const override { return mIsSet ? NodeType::ntSetAssign : NodeType::ntAssign; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeAssign( const NodeAssign &other );
#endif

        std::unique_ptr<Node> mLhs;
        std::unique_ptr<Node> mRhs;
        bool mIsSet = false;
    };

    /**
     * \ingroup core
     * \brief Represents a dimensioned variable.
     */
    class CORE_EXPORT DimVar
    {
      public:
        //! Variable name
        QString name;

        //! Dimension nodes
        QgsVBScriptScript::NodeExpressionList *dims = nullptr;
    };

    /**
     * \ingroup core
     * \brief Represents an array dimension.
     */
    class CORE_EXPORT NodeDim : public Node
    {
      public:
        //! Constructor
        NodeDim( const QString &visibility = QString() )
          : mVisibility( visibility )
        {}
        ~NodeDim() override;

        /**
         * Appends a variable to the dimension
         */
        void append( const QgsVBScriptScript::DimVar &variable ) { mVars.append( variable ); }

        /**
         * Sets the visibility of the dimension.
         */
        void setVisibility( const QString &visibility ) { mVisibility = visibility; }

        /**
         * Returns the dimensioned variables.
         */
        const QList<QgsVBScriptScript::DimVar> &variables() const { return mVars; }

        /**
         * Returns the visibility of the dimension.
         */
        QString visibility() const { return mVisibility; }

        NodeType nodeType() const override { return NodeType::ntDim; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeDim( const NodeDim &other );
#endif

        QString mVisibility;
        QList<DimVar> mVars;
    };

    /**
     * \ingroup core
     * \brief A re-dimension node.
     */
    class CORE_EXPORT NodeReDim : public Node
    {
      public:
        //! Constructor
        NodeReDim( bool preserve )
          : mPreserve( preserve )
        {}
        ~NodeReDim() override;

        /**
         * Appends a variable to the dimension
         */
        void append( const QgsVBScriptScript::DimVar &variable ) { mVars.append( variable ); }

        /**
         * Returns the dimensioned variables.
         */
        const QList<QgsVBScriptScript::DimVar> &variables() const { return mVars; }

        /**
         * Returns TRUE if the ReDim is set to preserve values.
         */
        bool preserve() const { return mPreserve; }

        NodeType nodeType() const override { return NodeType::ntReDim; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeReDim( const NodeReDim &other );
#endif

        bool mPreserve = false;
        QList<DimVar> mVars;
    };

    /**
     * \ingroup core
     * \brief A const statement.
     */
    class CORE_EXPORT NodeConst : public Node
    {
      public:
        //! Constructor
        NodeConst( const QString &visibility, const QString &name, Node *value SIP_TRANSFER )
          : mVisibility( visibility )
          , mName( name )
          , mValue( value )
        {}

        //! Returns the object visibility
        QString visibility() const { return mVisibility; }

        //! Returns the associated object name
        QString name() const { return mName; }

        //! Returns the object's value
        Node *value() const { return mValue.get(); }

        NodeType nodeType() const override { return NodeType::ntConst; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeConst( const NodeConst &other );
#endif

        QString mVisibility;
        QString mName;
        std::unique_ptr<Node> mValue;
    };

    /**
     * \ingroup core
     * \brief An else if block.
     */
    class CORE_EXPORT NodeElseIf
    {
      public:
        //! Constructor
        NodeElseIf( Node *condition SIP_TRANSFER, NodeStatementList *body SIP_TRANSFER )
          : mCondition( condition )
          , mThenBody( body )
        {}
        ~NodeElseIf();

        //! Returns the condition node
        Node *condition() const { return mCondition.get(); }
        //! Returns the THEN body
        NodeStatementList *thenBody() const { return mThenBody.get(); }

      private:
#ifdef SIP_RUN
        NodeElseIf( const NodeElseIf &other );
#endif
        std::unique_ptr< Node > mCondition;
        std::unique_ptr<NodeStatementList > mThenBody;
    };

    /**
     * \ingroup core
     * \brief An if block.
     */
    class CORE_EXPORT NodeIf : public Node
    {
      public:
        //! Constructor
        NodeIf( Node *condition SIP_TRANSFER, NodeStatementList *then SIP_TRANSFER, QList<QgsVBScriptScript::NodeElseIf *> elseIfs SIP_TRANSFER, NodeStatementList *elseBranch SIP_TRANSFER )
          : mCondition( condition )
          , mThen( then )
          , mElseIfs( elseIfs )
          , mElse( elseBranch )
        {}
        ~NodeIf() override;

        //! Returns the condition node
        Node *condition() const { return mCondition.get(); }
        //! Returns the THEN body
        NodeStatementList *thenBody() const { return mThen.get(); }
        //! Returns a list of ELSE IF blocks
        const QList<QgsVBScriptScript::NodeElseIf *> &elseIfs() const { return mElseIfs; }
        //! Returns the ELSE body
        NodeStatementList *elseBody() const { return mElse.get(); }

        NodeType nodeType() const override { return NodeType::ntIf; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override SIP_FACTORY;

      private:
#ifdef SIP_RUN
        NodeIf( const NodeIf &other );
#endif

        std::unique_ptr<Node> mCondition;
        std::unique_ptr<NodeStatementList> mThen;
        QList<NodeElseIf *> mElseIfs;
        std::unique_ptr<NodeStatementList> mElse;
    };

    // ----------------------------------------------------------------
    // Select Case
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeCase
    {
      public:
        NodeCase( NodeExpressionList *exprs, NodeStatementList *body )
          : expressions( exprs )
          , body( body )
        {}
        ~NodeCase()
        {
          delete expressions;
          delete body;
        }
        NodeExpressionList *expressions; // null = Case Else
        NodeStatementList *body;
    };
    class CORE_EXPORT NodeSelect : public Node
    {
      public:
        NodeSelect( Node *expr, QList<QgsVBScriptScript::NodeCase *> cases )
          : mExpr( expr )
          , mCases( cases )
        {}
        ~NodeSelect() override { qDeleteAll( mCases ); }
        Node *expr() const { return mExpr.get(); }
        const QList<QgsVBScriptScript::NodeCase *> &cases() const { return mCases; }
        NodeType nodeType() const override { return NodeType::ntSelect; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeSelect( const NodeSelect &other );
#endif

        std::unique_ptr<Node> mExpr;
        QList<NodeCase *> mCases;
    };

    // ----------------------------------------------------------------
    // For / Next
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeFor : public Node
    {
      public:
        NodeFor( const QString &var, Node *from, Node *to, Node *step, NodeStatementList *body )
          : mVar( var )
          , mFrom( from )
          , mTo( to )
          , mStep( step )
          , mBody( body )
        {}
        QString varName() const { return mVar; }
        Node *from() const { return mFrom.get(); }
        Node *to() const { return mTo.get(); }
        Node *step() const { return mStep.get(); } // may be null
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntFor; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeFor( const NodeFor &other );
#endif

        QString mVar;
        std::unique_ptr<Node> mFrom, mTo, mStep;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // For Each / Next
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeForEach : public Node
    {
      public:
        NodeForEach( const QString &var, Node *collection, NodeStatementList *body )
          : mVar( var )
          , mCollection( collection )
          , mBody( body )
        {}
        QString varName() const { return mVar; }
        Node *collection() const { return mCollection.get(); }
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntForEach; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeForEach( const NodeForEach &other );
#endif

        QString mVar;
        std::unique_ptr<Node> mCollection;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // While / Wend
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeWhile : public Node
    {
      public:
        NodeWhile( Node *cond, NodeStatementList *body )
          : mCond( cond )
          , mBody( body )
        {}
        Node *condition() const { return mCond.get(); }
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntWhile; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeWhile( const NodeWhile &other );
#endif

        std::unique_ptr<Node> mCond;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // Do / Loop
    // ----------------------------------------------------------------
    enum class DoMode : int
    {
      doInfinite,
      doWhilePre,
      doUntilPre,
      doWhilePost,
      doUntilPost
    };
    class CORE_EXPORT NodeDo : public Node
    {
      public:
        NodeDo( DoMode mode, Node *cond, NodeStatementList *body )
          : mMode( mode )
          , mCond( cond )
          , mBody( body )
        {}
        DoMode mode() const { return mMode; }
        Node *condition() const { return mCond.get(); } // null if infinite
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntDo; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeDo( const NodeDo &other );
#endif

        DoMode mMode;
        std::unique_ptr<Node> mCond;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // With statement
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeWith : public Node
    {
      public:
        NodeWith( Node *obj, NodeStatementList *body )
          : mObj( obj )
          , mBody( body )
        {}
        Node *obj() const { return mObj.get(); }
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntWith; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeWith( const NodeWith &other );
#endif

        std::unique_ptr<Node> mObj;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // Exit statement
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeExit : public Node
    {
      public:
        explicit NodeExit( const QString &what )
          : mWhat( what )
        {}
        QString what() const { return mWhat; } // "Sub","Function","For","Do","Property"
        NodeType nodeType() const override { return NodeType::ntExit; }
        QString dump() const override { return u"Exit "_s + mWhat; }
        void accept( Visitor &v ) const override;
        Node *clone() const override { return new NodeExit( mWhat ); }

      private:
        QString mWhat;
    };

    // ----------------------------------------------------------------
    // On Error
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeOnError : public Node
    {
      public:
        enum class Mode : int
        {
          ResumeNext,
          GoTo0
        };
        explicit NodeOnError( Mode m )
          : mMode( m )
        {}
        Mode mode() const { return mMode; }
        NodeType nodeType() const override { return NodeType::ntOnError; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override { return new NodeOnError( mMode ); }

      private:
        Mode mMode;
    };

    // ----------------------------------------------------------------
    // Sub / Function / Property definitions
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeSubDef : public Node
    {
      public:
        NodeSubDef( const QString &vis, const QString &name, QgsVBScriptScript::NodeParameterList *params, NodeStatementList *body )
          : mVis( vis )
          , mName( name )
          , mParams( params )
          , mBody( body )
        {}
        QString visibility() const { return mVis; }
        QString name() const { return mName; }
        QgsVBScriptScript::NodeParameterList *params() const { return mParams.get(); }
        NodeStatementList *body() const { return mBody.get(); }
        NodeType nodeType() const override { return NodeType::ntSubDef; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeSubDef( const NodeSubDef &other );
#endif

        QString mVis, mName;
        std::unique_ptr<NodeParameterList> mParams;
        std::unique_ptr<NodeStatementList> mBody;
    };

    class CORE_EXPORT NodeFuncDef : public Node
    {
      public:
        NodeFuncDef( const QString &vis, const QString &name, QgsVBScriptScript::NodeParameterList *params, QgsVBScriptScript::NodeStatementList *body )
          : mVis( vis )
          , mName( name )
          , mParams( params )
          , mBody( body )
        {}
        QString visibility() const { return mVis; }
        QString name() const { return mName; }
        QgsVBScriptScript::NodeParameterList *params() const { return mParams.get(); }
        QgsVBScriptScript::NodeStatementList *body() const { return mBody.get(); }
        QgsVBScriptScript::NodeType nodeType() const override { return NodeType::ntFuncDef; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeFuncDef( const NodeFuncDef &other );
#endif

        QString mVis, mName;
        std::unique_ptr<NodeParameterList> mParams;
        std::unique_ptr<NodeStatementList> mBody;
    };

    enum class PropKind : int
    {
      pkGet,
      pkLet,
      pkSet
    };
    class CORE_EXPORT NodePropDef : public Node
    {
      public:
        NodePropDef( const QString &vis, QgsVBScriptScript::PropKind kind, const QString &name, QgsVBScriptScript::NodeParameterList *params, QgsVBScriptScript::NodeStatementList *body )
          : mVis( vis )
          , mKind( kind )
          , mName( name )
          , mParams( params )
          , mBody( body )
        {}
        QString visibility() const { return mVis; }
        QgsVBScriptScript::PropKind kind() const { return mKind; }
        QString name() const { return mName; }
        QgsVBScriptScript::NodeParameterList *params() const { return mParams.get(); }
        QgsVBScriptScript::NodeStatementList *body() const { return mBody.get(); }
        QgsVBScriptScript::NodeType nodeType() const override { return mKind == PropKind::pkGet ? NodeType::ntPropGet : mKind == PropKind::pkLet ? NodeType::ntPropLet : NodeType::ntPropSet; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodePropDef( const NodePropDef &other );
#endif

        QString mVis;
        PropKind mKind;
        QString mName;
        std::unique_ptr<NodeParameterList> mParams;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // Class definition
    // ----------------------------------------------------------------
    class CORE_EXPORT NodeClassDef : public Node
    {
      public:
        NodeClassDef( const QString &name, QgsVBScriptScript::NodeStatementList *body )
          : mName( name )
          , mBody( body )
        {}
        QString name() const { return mName; }
        QgsVBScriptScript::NodeStatementList *body() const { return mBody.get(); }
        QgsVBScriptScript::NodeType nodeType() const override { return NodeType::ntClassDef; }
        QString dump() const override;
        void accept( Visitor &v ) const override;
        Node *clone() const override;

      private:
#ifdef SIP_RUN
        NodeClassDef( const NodeClassDef &other );
#endif

        QString mName;
        std::unique_ptr<NodeStatementList> mBody;
    };

    // ----------------------------------------------------------------
    // Visitor interface
    // ----------------------------------------------------------------
    class CORE_EXPORT Visitor
    {
      public:
        virtual ~Visitor() = default;
        virtual void visit( const NodeLiteral & ) = 0;
        virtual void visit( const NodeIdent & ) = 0;
        virtual void visit( const NodeUnaryOperator & ) = 0;
        virtual void visit( const NodeBinaryOperator & ) = 0;
        virtual void visit( const NodeMember & ) = 0;
        virtual void visit( const NodeFunctionCallOrArrayIndex & ) = 0;
        virtual void visit( const NodeFunctionCall & ) = 0;
        virtual void visit( const NodeAssign & ) = 0;
        virtual void visit( const NodeDim & ) = 0;
        virtual void visit( const NodeReDim & ) = 0;
        virtual void visit( const NodeConst & ) = 0;
        virtual void visit( const NodeIf & ) = 0;
        virtual void visit( const NodeSelect & ) = 0;
        virtual void visit( const NodeFor & ) = 0;
        virtual void visit( const NodeForEach & ) = 0;
        virtual void visit( const NodeWhile & ) = 0;
        virtual void visit( const NodeDo & ) = 0;
        virtual void visit( const NodeWith & ) = 0;
        virtual void visit( const NodeExit & ) = 0;
        virtual void visit( const NodeOnError & ) = 0;
        virtual void visit( const NodeSubDef & ) = 0;
        virtual void visit( const NodeFuncDef & ) = 0;
        virtual void visit( const NodePropDef & ) = 0;
        virtual void visit( const NodeClassDef & ) = 0;
        virtual void visit( const NodeStatementList & ) = 0;
        virtual void visit( const NodeExpressionList & ) = 0;
        virtual void visit( const NodeParameterList & ) = 0;
    };

    // ----------------------------------------------------------------
    // Recursive visitor (no-op base — subclass and override what you need)
    // ----------------------------------------------------------------
    class CORE_EXPORT RecursiveVisitor : public Visitor
    {
      public:
        void visit( const NodeLiteral & ) override {}
        void visit( const NodeIdent & ) override {}
        void visit( const NodeUnaryOperator &n ) override { n.operand()->accept( *this ); }
        void visit( const NodeBinaryOperator &n ) override
        {
          n.left()->accept( *this );
          n.right()->accept( *this );
        }
        void visit( const NodeMember &n ) override { n.obj()->accept( *this ); }
        void visit( const NodeFunctionCallOrArrayIndex &n ) override
        {
          n.calleeOrBase()->accept( *this );
          n.arguments()->accept( *this );
        }
        void visit( const NodeFunctionCall &n ) override
        {
          n.callee()->accept( *this );
          n.arguments()->accept( *this );
        }
        void visit( const NodeAssign &n ) override
        {
          n.lhs()->accept( *this );
          n.rhs()->accept( *this );
        }
        void visit( const NodeDim & ) override {}
        void visit( const NodeReDim & ) override {}
        void visit( const NodeConst &n ) override { n.value()->accept( *this ); }
        void visit( const NodeIf &n ) override;
        void visit( const NodeSelect &n ) override;
        void visit( const NodeFor &n ) override;
        void visit( const NodeForEach &n ) override;
        void visit( const NodeWhile &n ) override;
        void visit( const NodeDo &n ) override;
        void visit( const NodeWith &n ) override;
        void visit( const NodeExit & ) override {}
        void visit( const NodeOnError & ) override {}
        void visit( const NodeSubDef &n ) override;
        void visit( const NodeFuncDef &n ) override;
        void visit( const NodePropDef &n ) override;
        void visit( const NodeClassDef &n ) override;
        void visit( const NodeStatementList &n ) override
        {
          for ( auto *s : n.statements() )
            s->accept( *this );
        }
        void visit( const NodeExpressionList &n ) override
        {
          for ( auto *e : n.expressions() )
            e->accept( *this );
        }
        void visit( const NodeParameterList & ) override {}
    };

    void acceptVisitor( QgsVBScriptScript::Visitor &v ) const
    {
      if ( mRootNode )
        mRootNode->accept( v );
    }

  private:
#ifdef SIP_RUN
    QgsVBScriptScript( const QgsVBScriptScript &other );
#endif

    std::unique_ptr<Node> mRootNode;
    QString mStatement;
    QString mParserErrorString;
};


#endif // QGSVBSCRIPTSTATEMENT_H
