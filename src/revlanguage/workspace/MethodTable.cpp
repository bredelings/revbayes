#include "ArgumentRule.h"
#include "MethodTable.h"
#include "RbException.h"
#include "Function.h"
#include "RbUtil.h"
#include "MemberProcedure.h"

#include <sstream>

using namespace RevLanguage;

/** Basic constructor, empty table with or without parent */
MethodTable::MethodTable( MethodTable* parent ) : FunctionTable( parent )
{
}


/** Insert inherited methods into method table */
void MethodTable::insertInheritedMethods( const MethodTable& inheritedMethods )
{
    
    for ( MethodTable::const_iterator it = inheritedMethods.begin(); it != inheritedMethods.end(); ++it )
    {
        Function* the_function = (*it).second->clone();

        try
        {
            addFunction( the_function );
        }
        catch (RbException &)
        {
            
        }
        
    }
    
}

