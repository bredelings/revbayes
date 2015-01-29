#ifndef RLRealSymmetricMatrix_H
#define RLRealSymmetricMatrix_H

#include <iostream>



#include <iostream>
#include <vector>

#include "ModelObject.h"
#include "RlMatrixReal.h"
#include "TypedDagNode.h"


namespace RevLanguage {
    
    class RealSymmetricMatrix : public MatrixReal  {
        
    public:
        
        RealSymmetricMatrix(void);
        RealSymmetricMatrix(RevBayesCore::TypedDagNode<RevBayesCore::MatrixReal> *mat);                                                            //!< Construct from DAG node
        
        
        // the value type definition
        virtual RealSymmetricMatrix*    clone(void) const;                                                      //!< Clone object
        virtual RevObject*              convertTo(const TypeSpec& type) const;                                  //!< Convert to type
//        void                            constructInternalObject(void);                                          //!< We construct the a new internal SlidingMove.
        static const std::string&       getClassType(void);                                                     //!< Get Rev type
        static const TypeSpec&          getClassTypeSpec(void);                                                 //!< Get class type spec
//        const MemberRules&              getParameterRules(void) const;                                             //!< Get member rules (const)
        virtual const TypeSpec&         getTypeSpec(void) const;                                                //!< Get language type of the object
        void                            printValue(std::ostream& o) const;                                      //!< Print value (for user)
        
    };

}

#endif /* defined(__revbayes__RealSymmetricMatrix__) */
