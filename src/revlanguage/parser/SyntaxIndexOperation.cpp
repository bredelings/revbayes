#include "RlContainer.h"
#include "Function.h"
#include "Integer.h"
#include "RevNullObject.h"
#include "RlMemberMethod.h"
#include "SyntaxIndexOperation.h"
#include "Workspace.h"

using namespace RevLanguage;


/** Construct from base variable (member object), identifier and index */
SyntaxIndexOperation::SyntaxIndexOperation(SyntaxElement* var, SyntaxElement* indx) : SyntaxElement(),
    index(indx),
    baseVariable(var)
{

}


/** Deep copy constructor */
SyntaxIndexOperation::SyntaxIndexOperation(const SyntaxIndexOperation& x) : SyntaxElement(x)
{

    if ( x.baseVariable != NULL )
    {
        baseVariable = x.baseVariable->clone();
    }
    else
    {
        baseVariable = NULL;
    }

    if ( x.index != NULL )
    {
        index = x.index->clone();
    }
    else
    {
        index = NULL;
    }

}


/** Destructor deletes variable, identifier and index */
SyntaxIndexOperation::~SyntaxIndexOperation()
{

    delete baseVariable;
    delete index;
}


/** Assignment operator */
SyntaxIndexOperation& SyntaxIndexOperation::operator=(const SyntaxIndexOperation& x)
{

    if (&x != this)
    {
        delete baseVariable;
        delete index;


        SyntaxElement::operator=(x);


        if ( x.baseVariable != NULL )
        {
            baseVariable = x.baseVariable->clone();
        }
        else
        {
            baseVariable = NULL;
        }

        if ( x.index != NULL )
        {
            index = x.index->clone();
        }
        else
        {
            index = NULL;
        }

    }

    return (*this);
}



/** Clone syntax element */
SyntaxIndexOperation* SyntaxIndexOperation::clone () const
{

    return new SyntaxIndexOperation(*this);
}


/**
 * @brief Get semantic value (l-value)
 *
 * This function is similar to getValue(), but we only get the
 * slot and index of the variable. Another difference is that we
 * do not throw an error if the variable does not exist in the
 * frame; instead, we return a NULL pointer and set theSlot pointer
 * to NULL as well.
 */
RevPtr<RevVariable> SyntaxIndexOperation::evaluateLHSContent( Environment& env, const std::string &varType)
{

    RevPtr<RevVariable> indexVar     = index->evaluateContent(env);
    RevPtr<RevVariable> theParentVar = baseVariable->evaluateLHSContent(env, varType);
    
    // first we need to check if the parent variable is a deterministic vector
    if ( theParentVar->isVectorVariable() )
    {
        // everything is fine and we can add this element
    }
    else if ( theParentVar->getRevObject() == RevNullObject::getInstance() )
    {

    }
    else
    {
        RevObject& theParentObj = theParentVar->getRevObject();
        
        Container* c = dynamic_cast<Container*>( &theParentObj );
        if ( c != NULL )
        {
            
            if ( theParentObj.getDagNode()->isConstant() && c->allowsModificationToCompositeContainer() )
            {
                for ( size_t i=1; i<=c->size(); ++i)
                {
                    std::string elementIdentifier = theParentVar->getName() + "[" + i + "]";
                
                    RevPtr<RevVariable> theElementVar = NULL;
                    if ( !env.existsVariable( elementIdentifier ) )
                    {
                        // create a new slot
                        RevPtr<RevVariable> theElementVar = RevPtr<RevVariable>( new RevVariable( c->getElement(i-1) ) );
                        env.addVariable(elementIdentifier,theElementVar);
                        theElementVar->setName( elementIdentifier );
                    }
                
                    // get the slot and variable
                    theElementVar  = env.getVariable( elementIdentifier );
                
                    // set this variable as a hidden variable so that it doesn't show in ls()
                    theElementVar->setElementVariableState( true );
                    
                    theParentVar->addIndex( int(i) );
                }
            }
            else
            {
                if ( !theParentObj.getDagNode()->isConstant() )
                {
                    throw RbException("We cannot create a composite container from a non-constant container");
                }
                else if ( !c->allowsModificationToCompositeContainer() )
                {
                    throw RbException("An object of type '" + theParentObj.getType() + "' does not allow transformation into a composite container.");
                }
            }
        }
        else
        {
            throw RbException("We cannot make a composite container from variable of type '" + theParentObj.getType() + "'.");
        }
    }
    
    // compute the index and internal name for this variable
    int idx = static_cast<Integer&>( indexVar->getRevObject() ).getValue();
    std::string identifier = theParentVar->getName() + "[" + idx + "]";
    
    // mark the parent variable as a vector variable
    theParentVar->setVectorVariableState( true );
    theParentVar->addIndex( idx );


    if ( !env.existsVariable( identifier ) )
    {
        // create a new slot
        RevPtr<RevVariable> theVar = RevPtr<RevVariable>( new RevVariable( NULL ) );
        env.addVariable(identifier,theVar);
        theVar->setName( identifier );
    }

    // get the slot and variable
    RevPtr<RevVariable> theVar  = env.getVariable( identifier );
    
    
    // set this variable as an element variable; which is by default a hidden variable so that it doesn't show in ls()
    theVar->setElementVariableState( true );

    return theVar;
}


/**
 * @brief Get semantic value (r-value)
 *
 * The variable can either be a member or a base variable. In the latter
 * case, its "baseVariable" member is NULL. If the element is a base variable,
 * we get the semantic value of the element by looking it up in the frame.
 * If it is a member variable, we try to find it in the member variable
 * frame of a composite variable found by another SyntaxIndexOperation element.
 *
 * If dynamic == true, then we cannot compute a static index for indexed variables.
 * Instead, we need to deliver an index conisting of variables resulting
 * from dynamic evaluation of the index variables. These need to be put
 * in a dynamic lookup variable.
 */
RevPtr<RevVariable> SyntaxIndexOperation::evaluateContent( Environment& env, bool dynamic)
{

    RevPtr<RevVariable> indexVar     = index->evaluateContent(env,dynamic);
    RevPtr<RevVariable> theParentVar = baseVariable->evaluateContent( env );
    std::string identifier = theParentVar->getName() + "[" + indexVar->getRevObject().toString() + "]";

    RevPtr<RevVariable> theVar = NULL;

    // test whether we want to directly assess the variable or if we want to assess subelement of this container
    // if this variable already exists in the workspace
    if ( env.existsVariable( identifier ) )
    {
        // get the from the workspace
        theVar = env.getVariable( identifier );

    }
    else
    {


        try
        {
            // convert the value into a member object
            std::vector<Argument> args;
            args.push_back( Argument( theParentVar, "v" ) );
            args.push_back( Argument( indexVar, "index" ) );
            
            Function* f = Workspace::userWorkspace().getFunction("[]", args, false).clone();
            f->processArguments( args, false );
            theVar = f->execute();
            theVar->setName( identifier );
            
            delete f;
        }
        catch (RbException e)
        {
            // We are trying to find a member function
        
            // Now we get a reference to the member object inside
            RevObject &theMemberObject = theParentVar->getRevObject();
            
            const MethodTable& mt = theMemberObject.getMethods();
            
            // convert the value into a member object
            std::vector<Argument> args;
            args.push_back( Argument( indexVar, "index" ) );
            
            Function* theFunction = mt.getFunction( "[]", args, !dynamic ).clone();
            theFunction->processArguments(args, !dynamic);
            
            MemberMethod* theMemberMethod = dynamic_cast<MemberMethod*>( theFunction );
            if ( theMemberMethod != NULL )
            {
                theMemberMethod->setMemberObject( theParentVar );

                theVar = theFunction->execute();
                theVar->setName( identifier );
            }
            else
            {
                delete theFunction;
                throw e;
            }
            
        }

    }

    return theVar;

}


/**
 * Get the pointer to the internal base variable for this syntax element.
 */
SyntaxElement* SyntaxIndexOperation::getBaseVariable( void )
{
    // return the internal pointer
    return baseVariable;
}


/** Print info about the syntax element */
void SyntaxIndexOperation::printValue(std::ostream& o) const {

    o << "<" << this << "> SyntaxIndexOperation:" << std::endl;
    if (baseVariable != NULL)
    {
        o << "base variable   = <" << baseVariable << "> ";
        baseVariable->printValue(o);
        o << std::endl;
    }
    if ( index != NULL )
    {
        o << "index " << " = <" << index << "> ";
        index->printValue(o);
        o << std::endl;
    }
    else
    {
        o << "index " << " = < NULL >" << std::endl;
    }

    o << std::endl;

}


/**
 * Update the variable.
 * We need to refresh the composite variables so that the relationships are properly set.
 */
void SyntaxIndexOperation::updateVariable( Environment& env, const std::string &n )
{
    
    std::string varName = n;
    size_t pos = varName.find_last_of('[');
    if ( pos != std::string::npos)
    {
        std::string parentName = varName.substr(0,pos);
        
        if ( env.existsVariable(parentName) )
        {
            RevPtr<RevVariable> &parentVariable = env.getVariable(parentName);
            
            const std::set<int>& indices = parentVariable->getElementIndices();
            if ( indices.empty() )
            {
                throw RbException("Cannot create a vector variable with name '" + parentName + "' because it doesn't have elements.");
            }
            std::vector<Argument> args;
            for (std::set<int>::const_iterator it = indices.begin(); it != indices.end(); ++it)
            {
                std::string elementIdentifier = parentName + "[" + *it + "]";
                RevPtr<RevVariable>& elementVar = env.getVariable( elementIdentifier );
                // check that the element is not NULL
                if ( elementVar == NULL || elementVar->getRevObject() == RevNullObject::getInstance() )
                {
                    throw RbException("Cannot create vector variable with name '" + parentName + "' because element with name '" + elementIdentifier + "' is NULL." );
                }
                args.push_back( Argument( elementVar ) );
            }
            Function* func = Workspace::userWorkspace().getFunction("v",args,false).clone();
            func->processArguments(args,false);
            
            // Evaluate the function (call the static evaluation function)
            RevPtr<RevVariable> funcReturnValue = func->execute();
            
            // free the memory of our copy
            delete func;
            
            parentVariable->replaceRevObject( funcReturnValue->getRevObject().clone() );
            
            SyntaxIndexOperation *parentExpression = dynamic_cast< SyntaxIndexOperation *>( baseVariable );
            if ( parentExpression != NULL )
            {
                parentExpression->updateVariable(env, parentName);
            }
        }
    }
    

}

