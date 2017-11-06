#include "ArgumentRule.h"
#include "ArgumentRules.h"
#include "MetropolisHastingsMove.h"
#include "Move_NodeRateTimeSlide.h"
#include "ModelVector.h"
#include "NodeRateTimeSlideProposal.h"
#include "RbException.h"
#include "RealPos.h"
#include "RevObject.h"
#include "RlTimeTree.h"
#include "TypedDagNode.h"
#include "TypeSpec.h"


using namespace RevLanguage;

Move_NodeRateTimeSlide::Move_NodeRateTimeSlide() : Move()
{
    
}


/**
 * The clone function is a convenience function to create proper copies of inherited objected.
 * E.g. a.clone() will create a clone of the correct type even if 'a' is of derived type 'b'.
 *
 * \return A new copy of the process.
 */
Move_NodeRateTimeSlide* Move_NodeRateTimeSlide::clone(void) const
{
    
	return new Move_NodeRateTimeSlide(*this);
}


void Move_NodeRateTimeSlide::constructInternalObject( void )
{
    // we free the memory first
    delete value;
    
    // now allocate a new move
    RevBayesCore::TypedDagNode<RevBayesCore::Tree> *tmp = static_cast<const TimeTree &>( tree->getRevObject() ).getDagNode();
    RevBayesCore::StochasticNode<RevBayesCore::Tree> *t = static_cast<RevBayesCore::StochasticNode<RevBayesCore::Tree> *>( tmp );
    
    double w = static_cast<const RealPos &>( weight->getRevObject() ).getValue();
    
    RevBayesCore::Proposal *p;

    RevBayesCore::TypedDagNode<RevBayesCore::RbVector<double> >* r = static_cast<const ModelVector<RealPos> &>( rates->getRevObject() ).getDagNode();
    if( r->isStochastic() == true )
    {
        RevBayesCore::StochasticNode<RevBayesCore::RbVector<double> >* stoch = dynamic_cast<RevBayesCore::StochasticNode<RevBayesCore::RbVector<double> >* >( r );

        p = new RevBayesCore::NodeRateTimeSlideProposal(t, stoch);
    }
    else
    {

        std::vector<const RevBayesCore::DagNode*> parents = r->getParents();
        std::vector< RevBayesCore::StochasticNode<double> *> n;
        for (std::vector<const RevBayesCore::DagNode*>::const_iterator it = parents.begin(); it != parents.end(); ++it)
        {
            const RevBayesCore::StochasticNode<double> *the_node = dynamic_cast< const RevBayesCore::StochasticNode<double>* >( *it );
            if ( the_node != NULL )
            {
                n.push_back( const_cast< RevBayesCore::StochasticNode<double>* >( the_node ) );
            }
            else
            {
                throw RbException("Substitution rates vector isn't stochastic or a vector of stochastic nodes.");
            }
        }

        p = new RevBayesCore::NodeRateTimeSlideProposal(t, n);
    }

    value = new RevBayesCore::MetropolisHastingsMove(p,w,false);
}


/** Get Rev type of object */
const std::string& Move_NodeRateTimeSlide::getClassType(void)
{
    
    static std::string rev_type = "Move_NodeRateTimeSlide";
    
	return rev_type; 
}

/** Get class type spec describing type of object */
const TypeSpec& Move_NodeRateTimeSlide::getClassTypeSpec(void)
{
    
    static TypeSpec rev_type_spec = TypeSpec( getClassType(), new TypeSpec( Move::getClassTypeSpec() ) );
    
	return rev_type_spec; 
}


/**
 * Get the Rev name for the constructor function.
 *
 * \return Rev name of constructor function.
 */
std::string Move_NodeRateTimeSlide::getMoveName( void ) const
{
    // create a constructor function name variable that is the same for all instance of this class
    std::string c_name = "NodeRateTimeSlide";
    
    return c_name;
}


/** Return member rules (no members) */
const MemberRules& Move_NodeRateTimeSlide::getParameterRules(void) const
{
    
    static MemberRules memberRules;
    static bool rules_set = false;
    
    if ( !rules_set )
    {
        
        memberRules.push_back( new ArgumentRule( "tree", TimeTree::getClassTypeSpec(), "The tree on which this move operates.", ArgumentRule::BY_REFERENCE, ArgumentRule::STOCHASTIC ) );
        memberRules.push_back( new ArgumentRule( "rates", ModelVector<RealPos>::getClassTypeSpec(), "The vector of branch-specific substitution rates.",  ArgumentRule::BY_REFERENCE, ArgumentRule::ANY ) );

        /* Inherit weight from Move, put it after variable */
        const MemberRules& inheritedRules = Move::getParameterRules();
        memberRules.insert( memberRules.end(), inheritedRules.begin(), inheritedRules.end() );
        
        rules_set = true;
    }
    
    return memberRules;
}

/** Get type spec */
const TypeSpec& Move_NodeRateTimeSlide::getTypeSpec( void ) const
{
    
    static TypeSpec type_spec = getClassTypeSpec();
    
    return type_spec;
}



/** Get type spec */
void Move_NodeRateTimeSlide::printValue(std::ostream &o) const
{
    
    o << "Move_NodeRateTimeSlide(";
    if (tree != NULL)
    {
        o << tree->getName();
    }
    else
    {
        o << "?";
    }
    o << ")";
}


/** Set a NearestNeighborInterchange variable */
void Move_NodeRateTimeSlide::setConstParameter(const std::string& name, const RevPtr<const RevVariable> &var)
{
    
    if ( name == "tree" )
    {
        tree = var;
    }
    else if( name == "rates" )
    {
        rates = var;
    }
    else
    {
        Move::setConstParameter(name, var);
    }
}
