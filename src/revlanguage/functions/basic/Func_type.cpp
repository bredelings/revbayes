#include "Argument.h"
#include "ArgumentRule.h"
#include "Func_type.h"
#include "RbException.h"
#include "RlString.h"
#include "RlUtils.h"
#include "TypeSpec.h"

#include <fstream>

using namespace RevLanguage;

/** Default constructor */
Func_type::Func_type( void ) : Procedure() {
    
}


/** Clone object */
Func_type* Func_type::clone( void ) const {
    
    return new Func_type( *this );
}


/** Execute function */
RevPtr<RevVariable> Func_type::execute( void ) {
    
    RlString* type = new RlString( args[0].getVariable()->getRevObject().getType() );
    
    return new RevVariable( type );
}


/** Get argument rules */
const ArgumentRules& Func_type::getArgumentRules( void ) const
{
    
    static ArgumentRules argumentRules = ArgumentRules();
    static bool rulesSet = false;
    
    if ( !rulesSet )
    {
        
        argumentRules.push_back( new ArgumentRule( "x", RevObject::getClassTypeSpec(), "A variable.", ArgumentRule::BY_VALUE, ArgumentRule::ANY ) );
        rulesSet = true;
        
    }
    
    return argumentRules;
}


/** Get Rev type of object */
const std::string& Func_type::getClassType(void)
{
    
    static std::string revType = "Func_type";
    
	return revType; 
}


/** Get class type spec describing type of object */
const TypeSpec& Func_type::getClassTypeSpec(void)
{
    
    static TypeSpec revTypeSpec = TypeSpec( getClassType(), new TypeSpec( Function::getClassTypeSpec() ) );
    
	return revTypeSpec; 
}


/**
 * Get the primary Rev name for this function.
 */
std::string Func_type::getFunctionName( void ) const
{
    // create a name variable that is the same for all instance of this class
    std::string f_name = "type";
    
    return f_name;
}


/**
 * Get the author(s) of this function so they can receive credit (and blame) for it.
 */
std::vector<std::string> Func_type::getHelpAuthor(void) const
{
    // create a vector of authors for this function
    std::vector<std::string> authors;
    authors.push_back( "Sebastian Hoehna" );
    
    return authors;
}


/**
 * Get the (brief) description for this function
 */
std::vector<std::string> Func_type::getHelpDescription(void) const
{
    // create a variable for the description of the function
    std::vector<std::string> descriptions;
    descriptions.push_back( "The value type of a variable." );
    
    return descriptions;
}


/**
 * Get the more detailed description of the function
 */
std::vector<std::string> Func_type::getHelpDetails(void) const
{
    // create a variable for the description of the function
    std::vector<std::string> details;
    
    return details;
}


/**
 * Get an executable and instructive example.
 * These example should help the users to show how this function works but
 * are also used to test if this function still works.
 */
std::string Func_type::getHelpExample(void) const
{
    // create an example as a single string variable.
    std::string example = "";
    example += "a <- 2\n";
    example += "type(a)\n";
    example += "\n";
    example += "b <- 2.0\n";
    example += "type(b)\n";
    
    return example;
}


/**
 * Get some references/citations for this function
 *
 */
std::vector<RevBayesCore::RbHelpReference> Func_type::getHelpReferences(void) const
{
    // create an entry for each reference
    std::vector<RevBayesCore::RbHelpReference> references;
    
    
    return references;
}


/**
 * Get the names of similar and suggested other functions
 */
std::vector<std::string> Func_type::getHelpSeeAlso(void) const
{
    // create an entry for each suggested function
    std::vector<std::string> see_also;
    see_also.push_back( "structure" );
    
    
    return see_also;
}


/**
 * Get the title of this help entry
 */
std::string Func_type::getHelpTitle(void) const
{
    // create a title variable
    std::string title = "The value type of a variable";
    
    return title;
}


/** Get type spec */
const TypeSpec& Func_type::getTypeSpec( void ) const
{
    
    static TypeSpec typeSpec = getClassTypeSpec();
    
    return typeSpec;
}


/** Get return type */
const TypeSpec& Func_type::getReturnType( void ) const
{
    
    static TypeSpec returnTypeSpec = RlString::getClassTypeSpec();
    
    return returnTypeSpec;
}

