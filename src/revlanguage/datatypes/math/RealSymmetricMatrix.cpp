//
//  RealSymmetricMatrix.cpp
//  revbayes
//
//  Created by Nicolas Lartillot on 2014-03-27.
//  Copyright (c) 2014 revbayes team. All rights reserved.
//

#include "RealSymmetricMatrix.h"


#include "ConstantNode.h"
#include "Integer.h"
#include "Natural.h"
#include "RlBoolean.h"
#include "Probability.h"
#include "RealSymmetricMatrix.h"
#include "RbUtil.h"
#include "RlString.h"
#include "TypeSpec.h"
#include "RlMemberFunction.h"

#include <iomanip>
#include <sstream>

using namespace RevLanguage;

/* Default constructor */
RealSymmetricMatrix::RealSymmetricMatrix(void) : ModelObject<RevBayesCore::MatrixRealSymmetric>( new RevBayesCore::MatrixRealSymmetric(1) ) {
}


/* Construct from double */
RealSymmetricMatrix::RealSymmetricMatrix( RevBayesCore::TypedDagNode<RevBayesCore::MatrixRealSymmetric> * mat ) : ModelObject<RevBayesCore::MatrixRealSymmetric>( mat ) {
}


/* Copy Construct */
RealSymmetricMatrix::RealSymmetricMatrix(const RealSymmetricMatrix& from) : ModelObject<RevBayesCore::MatrixRealSymmetric>( new RevBayesCore::MatrixRealSymmetric(from.getValue().getDim()) ) {
    
}

/** Clone object */
RealSymmetricMatrix* RealSymmetricMatrix::clone(void) const {
    
	return new RealSymmetricMatrix(*this);
}


/** Convert to type. The caller manages the returned object. */
RevObject* RealSymmetricMatrix::convertTo( const TypeSpec& type ) const {
    
    return RevObject::convertTo( type );
}

/** Get Rev type of object */
const std::string& RealSymmetricMatrix::getClassType(void) {
    
    static std::string revType = "RealSymmetricMatrix";
    
	return revType;
}

/** Get class type spec describing type of object */
const TypeSpec& RealSymmetricMatrix::getClassTypeSpec(void) {
    
    static TypeSpec revTypeSpec = TypeSpec( getClassType(), new TypeSpec( RevObject::getClassTypeSpec() ) );
    
	return revTypeSpec;
}

/** Get type spec */
const TypeSpec& RealSymmetricMatrix::getTypeSpec( void ) const {
    
    static TypeSpec typeSpec = getClassTypeSpec();
    
    return typeSpec;
}

/* Get method specifications */
const RevLanguage::MethodTable& RealSymmetricMatrix::getMethods(void) const {
    
    static MethodTable    methods                     = MethodTable();
    static bool           methodsSet                  = false;
    
    if ( methodsSet == false )
    {
        
        ArgumentRules* covArgRules = new ArgumentRules();
        covArgRules->push_back(new ArgumentRule("i", false, Natural::getClassTypeSpec()));
        covArgRules->push_back(new ArgumentRule("j", false, Natural::getClassTypeSpec()));
        methods.addFunction("covariance", new MemberFunction<RealSymmetricMatrix,Real>( this, covArgRules ) );

        ArgumentRules* precArgRules = new ArgumentRules();
        precArgRules->push_back(new ArgumentRule("i", false, Natural::getClassTypeSpec()));
        precArgRules->push_back(new ArgumentRule("j", false, Natural::getClassTypeSpec()));
        methods.addFunction("precision", new MemberFunction<RealSymmetricMatrix,Real>( this, precArgRules ) );
        
        /*
        ArgumentRules* clampArgRules = new ArgumentRules();
        clampArgRules->push_back(new ArgumentRule("data", false, AbstractCharacterData::getClassTypeSpec()));
        clampArgRules->push_back(new ArgumentRule("processIndex", false, Natural::getClassTypeSpec()));
        clampArgRules->push_back(new ArgumentRule("dataIndex", false, Natural::getClassTypeSpec()));
        methods.addFunction("clampAt", new MemberProcedure(MultivariateRealNodeValTree::getClassTypeSpec(), clampArgRules ) );
        */
        
        // necessary call for proper inheritance
        methods.setParentTable( &ModelObject<RevBayesCore::MatrixRealSymmetric>::getMethods() );
        methodsSet = true;
    }
    
    
    return methods;
}



/** Is convertible to type? */
/*
bool RealSymmetricMatrix::isConvertibleTo(const TypeSpec& type) const {
    
    return RevObject::isConvertibleTo(type);
}
*/

/** Print value for user */
void RealSymmetricMatrix::printValue(std::ostream &o) const {
    
    long previousPrecision = o.precision();
    std::ios_base::fmtflags previousFlags = o.flags();
    
    std::fixed( o );
    o.precision( 3 );
    o << dagNode->getValue();
    
    o.setf( previousFlags );
    o.precision( previousPrecision );
}


