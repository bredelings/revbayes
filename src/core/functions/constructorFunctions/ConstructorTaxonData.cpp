/**
 * @file
 * This file contains the implementation of ConstructorTaxonData, which is 
 * the constructor function for TaxonData from a name and vector of characters.
 *
 * @brief Implementation of ConstructorTaxonData
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date: 2012-02-25 10:17:07 +0100 (Sat, 25 Feb 2012) $
 * @author The RevBayes core team
 * @license GPL version 3
 * @version 1.0
 * @since 2009-09-17, version 1.0
 *
 * $Id: $
 */

#include "ArgumentRule.h"
#include "Character.h"
#include "ConstructorTaxonData.h"
#include "DAGNode.h"
#include "MemberObject.h"
#include "RbFunction.h"
#include "RbUtil.h"
#include "TaxonData.h"
#include "TypeSpec.h"
#include "VectorString.h"
#include "ValueRule.h"

#include <sstream>


/** Constructor */
ConstructorTaxonData::ConstructorTaxonData() : RbFunction(), name( NULL ), chars( NULL ), retVal( Character::getClassName() ) {
    
}


///** Constructor */
//ConstructorTaxonData::ConstructorTaxonData( const ConstructorTaxonData& c ) : RbFunction(c), name( c.name ), chars( c.chars ), retVal( c.retVal ) {
//    
//}


/** Clone the object */
ConstructorTaxonData* ConstructorTaxonData::clone(void) const {
    
    return new ConstructorTaxonData(*this);
}


/** Execute function: we reset our template object here and give out a copy */
const RbLanguageObject& ConstructorTaxonData::executeFunction(void) {
    
    // TODO: Maybe we want to have specialized taxondata vectors?!
    retVal = TaxonData( Character::getClassName() );
    
    // the name of the taxon
    const std::string& n = static_cast<RbString&>( name->getValue() ).getValue();
    retVal.setTaxonName( n );
    
    // set the vector of characters
    Vector& v = static_cast<Vector&>( chars->getValue() );
    for (size_t i = 0; i < v.size(); i++) {
        Character* c = static_cast<Character*>( v.getElement( i ).clone() );
        retVal.addCharacter( c );
    }
    
    return retVal;
}


/** Get argument rules */
const ArgumentRules& ConstructorTaxonData::getArgumentRules(void) const {
        
   static ArgumentRules argRules = MemberRules();
   static bool        rulesSet = false;
   
   if (!rulesSet) {
       
       argRules.push_back( new ValueRule( "name", RbString::getClassTypeSpec() ) );
       argRules.push_back( new ValueRule( "x"   , TypeSpec( Vector::getClassTypeSpec(), new TypeSpec( Character::getClassTypeSpec() ) ) ) );
       
       rulesSet = true;
   }
   
   return argRules;
    
}


/** Get class name of object */
const std::string& ConstructorTaxonData::getClassName(void) { 
    
    static std::string rbClassName = "Constructor function";
    
	return rbClassName; 
}

/** Get class type spec describing type of object */
const TypeSpec& ConstructorTaxonData::getClassTypeSpec(void) { 
    
    static TypeSpec rbClass = TypeSpec( getClassName(), new TypeSpec( RbFunction::getClassTypeSpec() ) );
    
	return rbClass; 
}

/** Get type spec */
const TypeSpec& ConstructorTaxonData::getTypeSpec( void ) const {
    
    static TypeSpec typeSpec = getClassTypeSpec();
    
    return typeSpec;
}


/** Get return type */
const TypeSpec& ConstructorTaxonData::getReturnType(void) const {
    
    return TaxonData::getClassTypeSpec();
}


/** We catch here the setting of the argument variables to store our parameters. */
void ConstructorTaxonData::setArgumentVariable(std::string const &name, const RbVariablePtr& var) {
    
    if ( name == "name" ) {
        this->name = var;
    }
    else if ( name == "x" ) {
        chars = var;
    }
    else {
        RbFunction::setArgumentVariable(name, var);
    }
    
}



