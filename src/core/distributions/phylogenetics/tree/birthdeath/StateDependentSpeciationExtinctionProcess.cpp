#include "AbstractHomologousDiscreteCharacterData.h"
#include "RlAbstractHomologousDiscreteCharacterData.h"
#include "SSE_ODE.h"
#include "Clade.h"
#include "CladogeneticSpeciationRateMatrix.h"
#include "DistributionExponential.h"
#include "HomologousDiscreteCharacterData.h"
#include "StateDependentSpeciationExtinctionProcess.h"
#include "DeterministicNode.h"
#include "MatrixReal.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RateMatrix_JC.h"
#include "RbConstants.h"
#include "RbMathCombinatorialFunctions.h"
#include "RealPos.h"
#include "RlString.h"
#include "StandardState.h"
#include "StochasticNode.h"
#include "TopologyNode.h"

#include <algorithm>
#include <cmath>
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/odeint.hpp>


using namespace RevBayesCore;


/**
 * Constructor.
 *
 * The constructor connects the parameters of the birth-death process (DAG structure)
 * and initializes the probability density by computing the combinatorial constant of the tree structure.
 */
StateDependentSpeciationExtinctionProcess::StateDependentSpeciationExtinctionProcess(const TypedDagNode<double> *age,
                                                                                   const TypedDagNode<RbVector<double> > *ext,
                                                                                   const TypedDagNode<RateGenerator>* q,
                                                                                   const TypedDagNode<double>* r,
                                                                                   const TypedDagNode< Simplex >* p,
                                                                                   const TypedDagNode<double> *rh,
                                                                                   const std::string &cdt,
                                                                                   bool uo,
                                                                                   size_t max_lineages,
                                                                                   bool prune) : TypedDistribution<Tree>( new TreeDiscreteCharacterData() ),
    condition( cdt ),
    active_likelihood( std::vector<bool>(5, 0) ),
    changed_nodes( std::vector<bool>(5, false) ),
    dirty_nodes( std::vector<bool>(5, true) ),
    node_partial_likelihoods( std::vector<std::vector<std::vector<double> > >(5, std::vector<std::vector<double> >(2,std::vector<double>(2*ext->getValue().size(),0))) ),
    extinction_probabilities( std::vector<std::vector<double> >( 500.0, std::vector<double>( ext->getValue().size(), 0) ) ),
    num_states( ext->getValue().size() ),
    scaling_factors( std::vector<std::vector<double> >(5, std::vector<double>(2,0.0) ) ),
    use_cladogenetic_events( false ),
    use_origin( uo ),
    sample_character_history( false ),
    average_speciation( std::vector<double>(5, 0.0) ),
    average_extinction( std::vector<double>(5, 0.0) ),
    time_in_state( std::vector<double>(ext->getValue().size(), 0.0) ),    
    simmap( "" ),
    cladogenesis_matrix( NULL ),
    process_age( age ),
    mu( ext ),
    lambda(NULL),
    psi( NULL),
    pi( p ),
    Q( q ),
    rate( r ),
    rho( rh ),
    Q_default( ext->getValue().size() ),
    max_num_lineages( max_lineages ),
    prune_extinct_lineages( prune ),
    NUM_TIME_SLICES( 500.0 )
{
    addParameter( mu );
    addParameter( pi );
    addParameter( Q );
    addParameter( rho );
    addParameter( rate );
    addParameter( process_age );
    
    // set the length of the time slices used by the ODE for numerical integration
    dt = process_age->getValue() / NUM_TIME_SLICES * 50.0;

    value->getTreeChangeEventHandler().addListener( this );

}


/**
 * The clone function is a convenience function to create proper copies of inherited objected.
 * E.g. a.clone() will create a clone of the correct type even if 'a' is of derived type 'B'.
 *
 * \return A new copy of myself
 */
StateDependentSpeciationExtinctionProcess* StateDependentSpeciationExtinctionProcess::clone( void ) const
{
    StateDependentSpeciationExtinctionProcess* tmp = new StateDependentSpeciationExtinctionProcess( *this );
    tmp->getValue().getTreeChangeEventHandler().addListener(tmp);
    return tmp;
}


/**
 * Destructor. Because we added ourselves as a reference to tau when we added a listener to its
 * TreeChangeEventHandler, we need to remove ourselves as a reference and possibly delete tau
 * when we die. All other parameters are handled by others.
 */
StateDependentSpeciationExtinctionProcess::~StateDependentSpeciationExtinctionProcess( void )
{
    // We don't delete the params, because they might be used somewhere else too. The model needs to do that!

    // remove myself from the tree listeners
    value->getTreeChangeEventHandler().removeListener( this );
}


/**
 * Compute the log-transformed probability of the current value under the current parameter values.
 *
 */
double StateDependentSpeciationExtinctionProcess::computeLnProbability( void )
{
    
    // check that the ages are in correct chronological order
    // i.e., no child is older than its parent
    const std::vector<TopologyNode*>& nodes = value->getNodes();
    for (std::vector<TopologyNode*>::const_iterator it = nodes.begin(); it != nodes.end(); it++)
    {
        
        const TopologyNode &the_node = *(*it);
        if ( the_node.isRoot() == false )
        {
            
            if ( (the_node.getAge() - (*it)->getParent().getAge()) > 0 && the_node.isSampledAncestor() == false )
            {
                return RbConstants::Double::neginf;
            }
            else if ( (the_node.getAge() - (*it)->getParent().getAge()) > 0 && the_node.isSampledAncestor() == true )
            {
                return RbConstants::Double::neginf;
            }
            
        }
        
    }
    
    // check that the sampled ancestor nodes have a zero branch length
    for (std::vector<TopologyNode*>::const_iterator it = nodes.begin(); it != nodes.end(); it++)
    {
        
        const TopologyNode &the_node = *(*it);
        if ( the_node.isSampledAncestor() == true )
        {
            
            if ( the_node.isFossil() == false )
            {
                return RbConstants::Double::neginf;
            }
            else if ( the_node.getBranchLength() > 0 )
            {
                return RbConstants::Double::neginf;
            }
            
        }
        
    }
    
    double num_initial_lineages = 2; // this needs to be a double!
    const TopologyNode& root = value->getRoot();

    if ( use_origin == true )
    {
        // If we are conditioning on survival from the origin,
        // then we must divide by 2 the log survival probability computed by AbstractBirthDeathProcess
        num_initial_lineages = 1;
    }
    // if conditioning on root, root node must be a "true" bifurcation event
    else if (root.getChild(0).isSampledAncestor() || root.getChild(1).isSampledAncestor())
    {
        return RbConstants::Double::neginf;
    }

    // present time
    double ra = root.getAge();
    double process_time = getOriginAge();
    
    if ( ra > process_time || ra != getRootAge() )
    {
        return RbConstants::Double::neginf;
    }
    
    const std::vector<TopologyNode*> &c = root.getChildren();

    for (std::vector<TopologyNode*>::const_iterator it = c.begin(); it != c.end(); ++it)
    {
        if ( ra < (*it)->getAge() )
        {
            return RbConstants::Double::neginf;
        }
    }

    if ( value->getNumberOfNodes() != dirty_nodes.size() )
    {
        resizeVectors(value->getNumberOfNodes());
    }
    
    // variable declarations and initialization
    double lnProbTimes = 0;
    
    // conditioning on survival
    if ( condition == "survival" )
    {
        lnProbTimes = - num_initial_lineages*log( pSurvival(0, process_time) );
    }
    
    // multiply the probability of a descendant of the initial species
    lnProbTimes += computeRootLikelihood();
    
    return lnProbTimes + lnProbTreeShape();
}


void StateDependentSpeciationExtinctionProcess::computeNodeProbability(const RevBayesCore::TopologyNode &node, size_t node_index) const
{
    
    // check for recomputation
    if ( dirty_nodes[node_index] == true || sample_character_history == true )
    {
        // mark as computed
        dirty_nodes[node_index] = false;
        
        std::vector<double> &node_likelihood  = node_partial_likelihoods[node_index][active_likelihood[node_index]];

        if ( node.isTip() == true )
        {
            // this is a tip node
            TreeDiscreteCharacterData* tree = static_cast<TreeDiscreteCharacterData*>( this->value );

            std::vector<double> sampling(num_states, rho->getValue());
            std::vector<double> extinction(num_states, 1.0 - rho->getValue());

            if (psi != NULL && node.isFossil())
            {
                sampling = psi->getValue();
                extinction = pExtinction(0.0, node.getAge());
            }
            
            RbBitSet obs_state(num_states, true);
            bool gap = true;

            if ( tree->hasCharacterData() == true )
            {
                const DiscreteCharacterState &state = tree->getCharacterData().getTaxonData( node.getTaxon().getName() )[0];
                obs_state = state.getState();
                gap = (state.isMissingState() == true || state.isGapState() == true);
            }

            for (size_t j = 0; j < num_states; ++j)
            {
                
                node_likelihood[j] = extinction[j];
                
                if ( obs_state.isSet( j ) == true || gap == true )
                {
                    node_likelihood[num_states+j] = sampling[j];
                }
                else
                {
                    node_likelihood[num_states+j] = 0.0;
                }
            }
            
        }
        else
        {
            
            // this is an internal node
            const TopologyNode          &left           = node.getChild(0);
            size_t                      left_index      = left.getIndex();
            computeNodeProbability( left, left_index );
            const TopologyNode          &right          = node.getChild(1);
            size_t                      right_index     = right.getIndex();
            computeNodeProbability( right, right_index );
            
            // get the likelihoods of descendant nodes
            const std::vector<double> &left_likelihoods  = node_partial_likelihoods[left_index][active_likelihood[left_index]];
            const std::vector<double> &right_likelihoods = node_partial_likelihoods[right_index][active_likelihood[right_index]];

            std::map<std::vector<unsigned>, double> eventMap;
            std::vector<double> speciation_rates;
            if ( use_cladogenetic_events == true )
            {
                // get cladogenesis event map (sparse speciation rate matrix)
                eventMap = cladogenesis_matrix->getValue().getEventMap();
            }
            else
            {
                speciation_rates = lambda->getValue();
            }
            
            bool speciation_node = true;
            if ( left.isSampledAncestor() || right.isSampledAncestor() )
            {
                speciation_node = (psi == NULL);
            }

            // merge descendant likelihoods
            for (size_t i=0; i<num_states; ++i)
            {
                node_likelihood[i] = left_likelihoods[i];

                if ( use_cladogenetic_events == true && speciation_node == true )
                {
                    
                    double like_sum = 0.0;
                    std::map<std::vector<unsigned>, double>::iterator it;
                    for (it = eventMap.begin(); it != eventMap.end(); it++)
                    {
                        const std::vector<unsigned>& states = it->first;
                        double speciation_rate = it->second;
                        if (i == states[0])
                        {
                            double likelihoods = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
                            like_sum += speciation_rate * likelihoods;
                        }
                    }
                    node_likelihood[num_states + i] = like_sum;
                    
                }
                else
                {
                    node_likelihood[num_states + i] = left_likelihoods[num_states + i] * right_likelihoods[num_states + i];
                    node_likelihood[num_states + i] *= speciation_node ? speciation_rates[i] : 1.0;
                }
            }
            
        }
        
        double begin_age = node.getAge();
        double end_age = node.getParent().getAge();
        
        if ( node.isSampledAncestor() == false )
        {
            // calculate likelihoods for this branch
            if ( sample_character_history == false )
            {
                // numerically integrate over the entire branch length
                numericallyIntegrateProcess(node_likelihood, begin_age, end_age, true, false);
            }
            else
            {
                // calculate the conditional likelihoods for each time slice moving
                // along this branch backwards in time from the tip towards the root

                std::vector<std::vector<double> > branch_likelihoods;
                size_t current_dt = 0;
                
                // calculate partial likelihoods for each time slice and store them in branch_likelihoods
                while ( (current_dt * dt) + begin_age < end_age )
                {

                    std::vector<double> dt_likelihood;

                    double current_dt_start = (current_dt * dt) + begin_age;
                    double current_dt_end = ((current_dt + 1) * dt) + begin_age;
                    if (current_dt_end > end_age)
                    {
                        current_dt_end = end_age;
                    }
                    numericallyIntegrateProcess(node_likelihood, current_dt_start, current_dt_end, true, false);

                    std::vector<double>::const_iterator first = node_likelihood.begin() + num_states;
                    std::vector<double>::const_iterator last = node_likelihood.begin() + (num_states * 2);
                    dt_likelihood = std::vector<double>(first, last);

                    branch_likelihoods.push_back(dt_likelihood);
                    current_dt++;

                }
                
                // save the branch conditional likelihoods
                branch_partial_likelihoods[node_index] = branch_likelihoods;
            }
        }
        
        if ( RbSettings::userSettings().getUseScaling() == true ) //&& node_index % RbSettings::userSettings().getScalingDensity() == 0 )
        {
            // rescale the conditional likelihoods at the "end" of the branch
            double max = 0.0;
            for (size_t i=0; i<num_states; ++i)
            {
                if ( node_likelihood[num_states+i] > max )
                {
                    max = node_likelihood[num_states+i];
                }
            }
            max *= num_states;
            
            for (size_t i=0; i<num_states; ++i)
            {
                node_likelihood[num_states+i] /= max;
            }

            scaling_factors[node_index][active_likelihood[node_index]] = log(max);

            if ( node.isTip() == false )
            {
                const TopologyNode          &left           = node.getChild(0);
                size_t                      left_index      = left.getIndex();
                const TopologyNode          &right          = node.getChild(1);
                size_t                      right_index     = right.getIndex();
                scaling_factors[node_index][active_likelihood[node_index]] += scaling_factors[left_index][active_likelihood[left_index]] + scaling_factors[right_index][active_likelihood[right_index]];
            }
        }
    }
    
}


double StateDependentSpeciationExtinctionProcess::computeRootLikelihood( void ) const
{
    // get the likelihoods of descendant nodes
    const TopologyNode     &root            = value->getRoot();
    size_t                  node_index      = root.getIndex();
    const TopologyNode     &left            = root.getChild(0);
    size_t                  left_index      = left.getIndex();
    computeNodeProbability( left, left_index );
    const TopologyNode     &right           = root.getChild(1);
    size_t                  right_index     = right.getIndex();
    computeNodeProbability( right, right_index );

    // get the likelihoods of descendant nodes
    const std::vector<double> &left_likelihoods  = node_partial_likelihoods[left_index][active_likelihood[left_index]];
    const std::vector<double> &right_likelihoods = node_partial_likelihoods[right_index][active_likelihood[right_index]];

    std::vector<double> &node_likelihood  = node_partial_likelihoods[node_index][active_likelihood[node_index]];

    std::map<std::vector<unsigned>, double> eventMap;
    std::vector<double> speciation_rates;
    if ( use_cladogenetic_events == true )
    {
        // get cladogenesis event map (sparse speciation rate matrix)
        eventMap = cladogenesis_matrix->getValue().getEventMap();
    }
    else
    {
        speciation_rates = lambda->getValue();
    }

    bool speciation_node = false;
    if ( left.isSampledAncestor() || right.isSampledAncestor() )
    {
        speciation_node = (psi == NULL);
    }

    // merge descendant likelihoods
    for (size_t i=0; i<num_states; ++i)
    {
        node_likelihood[i] = left_likelihoods[i];

        if ( use_cladogenetic_events == true && speciation_node == true )
        {

            double like_sum = 0.0;
            std::map<std::vector<unsigned>, double>::iterator it;
            for (it = eventMap.begin(); it != eventMap.end(); it++)
            {
                const std::vector<unsigned>& states = it->first;
                double speciation_rate = it->second;
                if (i == states[0])
                {
                    double likelihoods = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
                    like_sum += speciation_rate * likelihoods;
                }
            }
            node_likelihood[num_states + i] = like_sum;

        }
        else
        {
            node_likelihood[num_states + i] = left_likelihoods[num_states + i] * right_likelihoods[num_states + i];
            node_likelihood[num_states + i] *= speciation_node ? speciation_rates[i] : 1.0;
        }
    }
    
    // calculate likelihoods for the root branch
    if ( use_origin )
    {
        double begin_age = getRootAge();
        double end_age = getOriginAge();

        if ( sample_character_history == false )
        {
            // numerically integrate over the entire branch length
            numericallyIntegrateProcess(node_likelihood, begin_age, end_age, true, false);
        }
        else
        {
            // calculate the conditional likelihoods for each time slice moving
            // along this branch backwards in time from the tip towards the root

            std::vector<std::vector<double> > branch_likelihoods;
            size_t current_dt = 0;

            // calculate partial likelihoods for each time slice and store them in branch_likelihoods
            while ( (current_dt * dt) + begin_age < end_age )
            {

                std::vector<double> dt_likelihood;

                double current_dt_start = (current_dt * dt) + begin_age;
                double current_dt_end = ((current_dt + 1) * dt) + begin_age;
                if (current_dt_end > end_age)
                {
                    current_dt_end = end_age;
                }
                numericallyIntegrateProcess(node_likelihood, current_dt_start, current_dt_end, true, false);

                std::vector<double>::const_iterator first = node_likelihood.begin() + num_states;
                std::vector<double>::const_iterator last = node_likelihood.begin() + (num_states * 2);
                dt_likelihood = std::vector<double>(first, last);

                branch_likelihoods.push_back(dt_likelihood);
                current_dt++;

            }

            // save the branch conditional likelihoods
            branch_partial_likelihoods[node_index] = branch_likelihoods;
        }
    }

    // sum the root likelihoods
    const RbVector<double> &freqs = getRootFrequencies();
    double prob = 0.0;

    for (size_t i = 0; i < num_states; ++i)
    {
        prob += freqs[i] * node_likelihood[num_states + i];
    }

    scaling_factors[node_index][active_likelihood[node_index]] = scaling_factors[left_index][active_likelihood[left_index]] + scaling_factors[right_index][active_likelihood[right_index]];
    
    return log(prob) + scaling_factors[node_index][active_likelihood[node_index]];
}


void StateDependentSpeciationExtinctionProcess::fireTreeChangeEvent( const RevBayesCore::TopologyNode &n, const unsigned& m )
{
    // call a recursive flagging of all node above (closer to the root) and including this node
    recursivelyFlagNodeDirty( n );

}


const RevBayesCore::AbstractHomologousDiscreteCharacterData& StateDependentSpeciationExtinctionProcess::getCharacterData() const
{
    return static_cast<TreeDiscreteCharacterData*>(this->value)->getCharacterData();
}


void StateDependentSpeciationExtinctionProcess::drawJointConditionalAncestralStates(std::vector<size_t>& startStates, std::vector<size_t>& endStates)
{
    // now begin the root-to-tip pass, drawing ancestral states conditional on the start states
    
    std::map<std::vector<unsigned>, double> eventMap;
    std::vector<double> speciation_rates;
    if ( use_cladogenetic_events == true )
    {
        // get cladogenesis event map (sparse speciation rate matrix)
        eventMap = cladogenesis_matrix->getValue().getEventMap();
    }
    else
    {
        speciation_rates = lambda->getValue();
    }
    
    // get the likelihoods of descendant nodes
    const TopologyNode  &root               = value->getRoot();
    size_t               node_index         = root.getIndex();
    const TopologyNode  &left               = root.getChild(0);
    size_t               left_index         = left.getIndex();
    const state_type    &left_likelihoods   = node_partial_likelihoods[left_index][active_likelihood[left_index]];
    const TopologyNode  &right              = root.getChild(1);
    size_t               right_index        = right.getIndex();
    const state_type    &right_likelihoods  = node_partial_likelihoods[right_index][active_likelihood[right_index]];
    
    // get root frequencies
    const RbVector<double> &freqs = getRootFrequencies();
    
    std::map<std::vector<unsigned>, double> sample_probs;
    double sample_probs_sum = 0.0;
    std::map<std::vector<unsigned>, double>::iterator it;
    
    // calculate probabilities for each state
    if ( use_cladogenetic_events == true )
    {
        // iterate over each cladogenetic event possible
        // and initialize probabilities for each clado event
        for (it = eventMap.begin(); it != eventMap.end(); it++)
        {
            const std::vector<unsigned>& states = it->first;
            double speciation_rate = it->second;
            
            // we need to sample from the ancestor, left, and right states jointly,
            // so keep track of the probability of each clado event
            double prob = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
            prob *= freqs[states[0]] * speciation_rate;
            sample_probs[ states ] = prob;
            sample_probs_sum += prob;
        }
    }
    else
    {
        for (size_t i = 0; i < num_states; i++)
        {
            double likelihood = left_likelihoods[num_states + i] * right_likelihoods[num_states + i] * speciation_rates[i];
            std::vector<unsigned> states = boost::assign::list_of(i)(i)(i);
            sample_probs[ states ] = likelihood * freqs[i];
            sample_probs_sum += likelihood * freqs[i];
        }
    }
    
    // sample ancestor, left, and right character states from probs
    size_t a = 0, l = 0, r = 0;
    
    if (sample_probs_sum == 0)
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        size_t u = rng->uniform01() * sample_probs.size();
        size_t v = 0;
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            if (u < v)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                endStates[node_index] = a;
                startStates[left_index] = l;
                startStates[right_index] = r;
                break;
             }
             v++;
         }
    }
    else
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        double u = rng->uniform01() * sample_probs_sum;
       
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            u -= it->second;
            if (u < 0.0)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                endStates[node_index] = a;
                startStates[node_index] = a;
                startStates[left_index] = l;
                startStates[right_index] = r;
                break;
            }
        }
    } 
    
    // recurse towards tips
    recursivelyDrawJointConditionalAncestralStates(left, startStates, endStates);
    recursivelyDrawJointConditionalAncestralStates(right, startStates, endStates);
    
}


void StateDependentSpeciationExtinctionProcess::recursivelyDrawJointConditionalAncestralStates(const TopologyNode &node, std::vector<size_t>& startStates, std::vector<size_t>& endStates)
{
    
    size_t node_index = node.getIndex();
    
    if ( node.isTip() == true )
    {
        const AbstractHomologousDiscreteCharacterData& data = static_cast<TreeDiscreteCharacterData*>(this->value)->getCharacterData();
        const AbstractDiscreteTaxonData& taxon_data = data.getTaxonData( node.getName() );
        
        const DiscreteCharacterState &char_state = taxon_data.getCharacter(0);
        
        // get the observed state at the tip if it is known, otherwise simulate it
        if ( char_state.isAmbiguous() == false && char_state.isMissingState() == false )
        {
            endStates[node_index] = char_state.getStateIndex();
        }
        else
        {
            // initialize the conditional likelihoods for this branch
            state_type branch_conditional_probs = std::vector<double>(2 * num_states, 0);
            size_t start_state = startStates[node_index];
            branch_conditional_probs[ num_states + start_state ] = 1.0;
            
            // first calculate extinction likelihoods via a backward time pass
            double end_age = node.getParent().getAge();
            numericallyIntegrateProcess(branch_conditional_probs, 0, end_age, true, true);
            
            // now calculate conditional likelihoods along branch in forward time
            end_age        = node.getParent().getAge() - node.getAge();
            numericallyIntegrateProcess(branch_conditional_probs, 0, end_age, false, false);
            
            double total_prob = 0.0;
            for (size_t i = 0; i < num_states; ++i)
            {
                if ( char_state.isMissingState() == true || char_state.isGapState() == true || char_state.isStateSet(i) == true )
                {
                    total_prob += branch_conditional_probs[ num_states + i ];
                }
            }
            
            RandomNumberGenerator* rng = GLOBAL_RNG;
            double u = rng->uniform01() * total_prob;
            
            for (size_t i = 0; i < num_states; ++i)
            {
                
                if ( char_state.isMissingState() == true || char_state.isGapState() == true || char_state.isStateSet(i) == true )
                {
                    u -= branch_conditional_probs[ num_states + i ];
                    if ( u <= 0.0 )
                    {
                        endStates[node_index] = i;
                        break;
                    }
                    
                }
                
            }
            
        }
    }
    else
    {
        // sample characters by their probability conditioned on the branch's start state going to end states
        
        // initialize the conditional likelihoods for this branch
        state_type branch_conditional_probs = std::vector<double>(2 * num_states, 0);
        size_t start_state = startStates[node_index];
        branch_conditional_probs[ num_states + start_state ] = 1.0;

        // first calculate extinction likelihoods via a backward time pass
        double end_age = node.getParent().getAge();
        numericallyIntegrateProcess(branch_conditional_probs, 0, end_age, true, true);
        
        // now calculate conditional likelihoods along branch in forward time
        end_age        = node.getParent().getAge() - node.getAge();
        numericallyIntegrateProcess(branch_conditional_probs, 0, end_age, false, false);
        
        std::map<std::vector<unsigned>, double> event_map;
        std::vector<double> speciation_rates;
        if ( use_cladogenetic_events == true )
        {
            // get cladogenesis event map (sparse speciation rate matrix)
            event_map = cladogenesis_matrix->getValue().getEventMap();
        }
        else
        {
            speciation_rates = lambda->getValue();
        }
        
        // get likelihoods of descendant nodes
        const TopologyNode &left = node.getChild(0);
        size_t left_index = left.getIndex();
        state_type left_likelihoods = node_partial_likelihoods[left_index][active_likelihood[left_index]];
        const TopologyNode &right = node.getChild(1);
        size_t right_index = right.getIndex();
        state_type right_likelihoods = node_partial_likelihoods[right_index][active_likelihood[right_index]];
        
        std::map<std::vector<unsigned>, double> sample_probs;
        double sample_probs_sum = 0.0;
        std::map<std::vector<unsigned>, double>::iterator it;

        // calculate probabilities for each state
        if ( use_cladogenetic_events == true )
        {
            // iterate over each cladogenetic event possible
            // and initialize probabilities for each clado event
            for (it = event_map.begin(); it != event_map.end(); it++)
            {
                const std::vector<unsigned>& states = it->first;
                double speciation_rate = it->second;
                
                // we need to sample from the ancestor, left, and right states jointly,
                // so keep track of the probability of each clado event
                double prob = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
                prob *= speciation_rate * branch_conditional_probs[num_states + states[0]];
                sample_probs[ states ] = prob;
                sample_probs_sum += prob;
            }
        }
        else
        {
            for (size_t i = 0; i < num_states; i++)
            {
                double prob = left_likelihoods[num_states + i] * right_likelihoods[num_states + i] * speciation_rates[i];
                prob *= branch_conditional_probs[num_states + i];
                std::vector<unsigned> states = boost::assign::list_of(i)(i)(i);
                sample_probs[ states ] = prob;
                sample_probs_sum += prob;
            }
        }
        
        // finally, sample ancestor, left, and right character states from probs
        size_t a = 0, l = 0, r = 0;

        if (sample_probs_sum == 0)
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            size_t u = rng->uniform01() * sample_probs.size();
            size_t v = 0;
            for (it = sample_probs.begin(); it != sample_probs.end(); it++)
            {
                if (u < v)
                {
                    const std::vector<unsigned>& states = it->first;
                    a = states[0];
                    l = states[1];
                    r = states[2];
                    endStates[node_index] = a;
                    startStates[left_index] = l;
                    startStates[right_index] = r;
                    break;
                 }
                 v++;
             }
        }
        else
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            double u = rng->uniform01() * sample_probs_sum;
            
            for (it = sample_probs.begin(); it != sample_probs.end(); it++)
            {
                u -= it->second;
                if (u < 0.0)
                {
                    const std::vector<unsigned>& states = it->first;
                    a = states[0];
                    l = states[1];
                    r = states[2];
                    endStates[node_index] = a;
                    startStates[left_index] = l;
                    startStates[right_index] = r;
                    break;
                }
            }
        }
        
        // recurse towards tips
        recursivelyDrawJointConditionalAncestralStates(left, startStates, endStates);
        recursivelyDrawJointConditionalAncestralStates(right, startStates, endStates);
    }
    
}


void StateDependentSpeciationExtinctionProcess::recursivelyFlagNodeDirty( const RevBayesCore::TopologyNode &n ) {

    // we need to flag this node and all ancestral nodes for recomputation
    size_t index = n.getIndex();

    // if this node is already dirty, the also all the ancestral nodes must have been flagged as dirty
    if ( dirty_nodes[index] == false )
    {
        // the root doesn't have an ancestor
        if ( n.isRoot() == false )
        {
            recursivelyFlagNodeDirty( n.getParent() );
        }

        // set the flag
        dirty_nodes[index] = true;

        // if we previously haven't touched this node, then we need to change the active likelihood pointer
        if ( changed_nodes[index] == false )
        {
            active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
            changed_nodes[index] = true;
        }

    }

}


void StateDependentSpeciationExtinctionProcess::drawStochasticCharacterMap(std::vector<std::string*>& character_histories)
{
    // first populate partial likelihood vectors along all the branches
    sample_character_history = true;
    computeLnProbability();

    for (size_t i = 0; i < num_states; i++) 
    {
        time_in_state[i] = 0.0;
    }

    // now begin the root-to-tip pass, drawing ancestral states for each time slice conditional on the start states
    std::map<std::vector<unsigned>, double> eventMap;
    std::vector<double> speciation_rates;
    if ( use_cladogenetic_events == true )
    {
        // get cladogenesis event map (sparse speciation rate matrix)
        eventMap = cladogenesis_matrix->getValue().getEventMap();
    }
    else
    {
        speciation_rates = lambda->getValue();
    }
    
    // get the likelihoods of descendant nodes
    const TopologyNode  &root               = value->getRoot();
    size_t               node_index         = root.getIndex();
    const TopologyNode  &left               = root.getChild(0);
    size_t               left_index         = left.getIndex();
    const state_type    &left_likelihoods   = node_partial_likelihoods[left_index][active_likelihood[left_index]];
    const TopologyNode  &right              = root.getChild(1);
    size_t               right_index        = right.getIndex();
    const state_type    &right_likelihoods  = node_partial_likelihoods[right_index][active_likelihood[right_index]];
    
    // get root frequencies
    const RbVector<double> &freqs = getRootFrequencies();
    
    std::map<std::vector<unsigned>, double> sample_probs;
    double sample_probs_sum = 0.0;
    std::map<std::vector<unsigned>, double>::iterator it;
    
    // calculate probabilities for each state
    if ( use_cladogenetic_events == true )
    {
        // iterate over each cladogenetic event possible
        // and initialize probabilities for each clado event
        for (it = eventMap.begin(); it != eventMap.end(); it++)
        {
            const std::vector<unsigned>& states = it->first;
            double speciation_rate = it->second;
            
            // we need to sample from the ancestor, left, and right states jointly,
            // so keep track of the probability of each clado event
            double prob = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
            prob *= freqs[states[0]] * speciation_rate;
            sample_probs[ states ] = prob;
            sample_probs_sum += prob;
        }
    }
    else
    {
        for (size_t i = 0; i < num_states; i++)
        {
            double likelihood = left_likelihoods[num_states + i] * right_likelihoods[num_states + i] * speciation_rates[i];
            std::vector<unsigned> states = boost::assign::list_of(i)(i)(i);
            sample_probs[ states ] = likelihood * freqs[i];
            sample_probs_sum += likelihood * freqs[i];
        }
    }
    
    // sample ancestor, left, and right character states from probs
    size_t a = 0, l = 0, r = 0;
    
    if (sample_probs_sum == 0)
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        size_t u = rng->uniform01() * sample_probs.size();
        size_t v = 0;
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            if (u < v)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                break;
            }
            v++;
        }
    }
    else
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        double u = rng->uniform01() * sample_probs_sum;
        
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            u -= it->second;
            if (u < 0.0)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                break;
            }
        }
    }
    
    // save the character history for the root
    std::string* simmap_string = new std::string("{" + StringUtilities::toString(a) + "," + StringUtilities::toString( root.getBranchLength() ) + "}");
    character_histories[node_index] = simmap_string;
    
    // recurse towards tips
    recursivelyDrawStochasticCharacterMap(left, l, character_histories);
    recursivelyDrawStochasticCharacterMap(right, r, character_histories);

    Tree t = Tree(*value);
    t.clearNodeParameters();
    t.addNodeParameter( "character_history", character_histories, false );
    simmap = t.getSimmapNewickRepresentation();
    
    // turn off sampling until we need it again
    sample_character_history = false;

}


void StateDependentSpeciationExtinctionProcess::recursivelyDrawStochasticCharacterMap(const TopologyNode &node, size_t start_state, std::vector<std::string*>& character_histories)
{
    
    size_t node_index = node.getIndex();
    std::vector<double> speciation_rates = calculateTotalSpeciationRatePerState();
    std::vector<double> extinction_rates = mu->getValue();
    
    // sample characters by their probability conditioned on the branch's start state going to end states
    
    // initialize the conditional likelihoods for this branch
    state_type branch_conditional_probs = std::vector<double>(2 * num_states, 0);
    branch_conditional_probs[ num_states + start_state ] = 1.0;
    
    // first calculate extinction likelihoods via a backward time pass
    double start_time = node.getParent().getAge();
    numericallyIntegrateProcess(branch_conditional_probs, 0, start_time, true, true);
    
    // now calculate conditional likelihoods along branch in forward time
    double branch_length = node.getParent().getAge() - node.getAge();
    size_t current_dt = 0;
    double current_dt_start = 0;
    double current_dt_end = 0;
    
    size_t current_state = start_state;
    
    // set up vectors to hold the transition events
    std::vector<size_t> transition_states;
    std::vector<double> transition_times;
    transition_states.push_back(current_state);
    
    int downpass_dt = int( branch_partial_likelihoods[node_index].size() ) - 1;
   
    // keep track of rates in each time interval so we can calculate per branch averages of each rate
    double total_speciation_rate = 0.0;
    double total_extinction_rate = 0.0;
    double num_dts = 0.0;

    // loop over every time slice, stopping before the last time slice
    while ( downpass_dt >= 0 && ((current_dt + 1) * dt) < branch_length)
    {
        current_dt_start = (current_dt * dt);
        current_dt_end = ((current_dt + 1) * dt);
        
        numericallyIntegrateProcess(branch_conditional_probs, current_dt_start, current_dt_end, false, false);

        // draw state for this time slice
        size_t new_state = current_state;
        double probs_sum = 0.0;
        for (size_t i = 0; i < num_states; i++)
        {
            probs_sum += branch_conditional_probs[i + num_states] * branch_partial_likelihoods[node_index][downpass_dt][i];
        }
        if ( probs_sum == 0.0 )
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            double u = rng->uniform01() * num_states;
            new_state = size_t(u);
        }
        else
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            double u = rng->uniform01() * probs_sum;

            for (size_t i = 0; i < num_states; i++)
            {
                u -= branch_conditional_probs[i + num_states] * branch_partial_likelihoods[node_index][downpass_dt][i];
                if (u < 0.0)
                {
                    new_state = i;
                    break;
                }
            }
        }
        
        // check if there was a character state transition
        if (new_state != current_state)
        {
            double time_since_last_transition = 0.0;
            double transition_times_sum = 0.0;
            for (size_t j = 0; j < transition_times.size(); j++)
            {
                transition_times_sum += transition_times[j];
            }
            time_since_last_transition = current_dt_end - transition_times_sum;

            transition_times.push_back(time_since_last_transition);
            transition_states.push_back(new_state);
            current_state = new_state;
        }
        
        // condition branch_conditional_probs on the sampled state
        for (size_t i = 0; i < num_states; i++)
        {
            if (i == current_state)
            {
                branch_conditional_probs[ num_states + i ] = 1.0;
            }
            else
            {
                branch_conditional_probs[ num_states + i ] = 0.0;
            }
        }
        
        current_dt++;
        downpass_dt--;
        
        // keep track of rates in this interal so we can calculate per branch averages of each rate
        total_speciation_rate += speciation_rates[current_state];
        total_extinction_rate += extinction_rates[current_state];
        time_in_state[current_state] += dt;
        num_dts += 1;
    }
    
    if ( node.isTip() == true )
    {
        // the last time slice of the branch will be the observed state
        
        const AbstractHomologousDiscreteCharacterData& data = static_cast<TreeDiscreteCharacterData*>(this->value)->getCharacterData();
        const AbstractDiscreteTaxonData& taxon_data = data.getTaxonData( node.getName() );
        
        const DiscreteCharacterState &char_state = taxon_data.getCharacter(0);
        size_t new_state = current_state;
        
        if ( char_state.isAmbiguous() == false )
        {
            new_state = char_state.getStateIndex();
        }
        else
        {
            // use the simulated state
            new_state = current_state;
        }
        
        // keep track of rates in this interval so we can calculate per branch averages of each rate
        total_speciation_rate += speciation_rates[new_state];
        total_extinction_rate += extinction_rates[new_state];
        time_in_state[new_state] += dt;
        num_dts += 1;
        
        // check if there was a character state transition
        if (new_state != current_state)
        {
            double time_since_last_transition = 0.0;
            double transition_times_sum = 0.0;
            for (size_t j = 0; j < transition_times.size(); j++)
            {
                transition_times_sum += transition_times[j];
            }
            time_since_last_transition = current_dt_end - transition_times_sum;
            
            transition_times.push_back(time_since_last_transition);
            transition_states.push_back(new_state);
        }
        
        // add the length of the final character state
        double time_since_last_transition = 0.0;
        double transition_times_sum = 0.0;
        for (size_t j = 0; j < transition_times.size(); j++)
        {
            transition_times_sum += transition_times[j];
        }
        time_since_last_transition = branch_length - transition_times_sum;
        transition_times.push_back(time_since_last_transition);
        
        // make SIMMAP string
        std::string simmap_string = "{";
        for (size_t i = transition_times.size(); i > 0; i--)
        {
            simmap_string = simmap_string + StringUtilities::toString(transition_states[i - 1]) + "," + StringUtilities::toString(transition_times[i - 1]);
            if (i != 1)
            {
                simmap_string = simmap_string + ":";
            }
        }
        simmap_string = simmap_string + "}";
        
        // calculate average diversification rates on this branch
        average_speciation[node_index] = total_speciation_rate / num_dts;
        average_extinction[node_index] = total_extinction_rate / num_dts;

        // save the character history for this branch
        character_histories[node_index] = new std::string(simmap_string);
        
    }
    else
    {
        // the last time slice of the branch will be the state of the node before any cladogenetic events
        
        std::map<std::vector<unsigned>, double> event_map;
        if ( use_cladogenetic_events == true )
        {
            // get cladogenesis event map (sparse speciation rate matrix)
            event_map = cladogenesis_matrix->getValue().getEventMap();
        }
        
        // get likelihoods of descendant nodes
        const TopologyNode &left = node.getChild(0);
        size_t left_index = left.getIndex();
        state_type left_likelihoods = node_partial_likelihoods[left_index][active_likelihood[left_index]];
        const TopologyNode &right = node.getChild(1);
        size_t right_index = right.getIndex();
        state_type right_likelihoods = node_partial_likelihoods[right_index][active_likelihood[right_index]];
        
        std::map<std::vector<unsigned>, double> sample_probs;
        double sample_probs_sum = 0.0;
        std::map<std::vector<unsigned>, double>::iterator it;
        
        // calculate probabilities for each state
        if ( use_cladogenetic_events == true )
        {
            // iterate over each cladogenetic event possible
            // and initialize probabilities for each clado event
            for (it = event_map.begin(); it != event_map.end(); it++)
            {
                const std::vector<unsigned>& states = it->first;
                double speciation_rate = it->second;
                
                // we need to sample from the ancestor, left, and right states jointly,
                // so keep track of the probability of each clado event
                double prob = left_likelihoods[num_states + states[1]] * right_likelihoods[num_states + states[2]];
                prob *= speciation_rate * branch_conditional_probs[num_states + states[0]];
                sample_probs[ states ] = prob;
                sample_probs_sum += prob;
            }
        }
        else
        {
            for (size_t i = 0; i < num_states; i++)
            {
                double prob = left_likelihoods[num_states + i] * right_likelihoods[num_states + i] * speciation_rates[i];
                prob *= branch_conditional_probs[num_states + i];
                std::vector<unsigned> states = boost::assign::list_of(i)(i)(i);
                sample_probs[ states ] = prob;
                sample_probs_sum += prob;
            }
        }
        
        // finally, sample ancestor, left, and right character states from probs
        size_t a = 0;
        size_t l = 0;
        size_t r = 0;
        
        if (sample_probs_sum == 0)
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            size_t u = rng->uniform01() * sample_probs.size();
            size_t v = 0;
            for (it = sample_probs.begin(); it != sample_probs.end(); it++)
            {
                if (u < v)
                {
                    const std::vector<unsigned>& states = it->first;
                    a = states[0];
                    l = states[1];
                    r = states[2];
                    break;
                }
                v++;
            }
        }
        else
        {
            RandomNumberGenerator* rng = GLOBAL_RNG;
            double u = rng->uniform01() * sample_probs_sum;
            
            for (it = sample_probs.begin(); it != sample_probs.end(); it++)
            {
                u -= it->second;
                if (u < 0.0)
                {
                    const std::vector<unsigned>& states = it->first;
                    a = states[0];
                    l = states[1];
                    r = states[2];
                    break;
                }
            }
        }
        
        // keep track of rates in this interval so we can calculate per branch averages of each rate
        total_speciation_rate += speciation_rates[a];
        total_extinction_rate += extinction_rates[a];
        time_in_state[a] += dt;
        num_dts += 1;
        
        // check if there was a character state transition
        if (a != current_state)
        {
            double time_since_last_transition = 0.0;
            double transition_times_sum = 0.0;
            for (size_t j = 0; j < transition_times.size(); j++)
            {
                transition_times_sum += transition_times[j];
            }
            time_since_last_transition = current_dt_end - transition_times_sum;

            transition_times.push_back(time_since_last_transition);
            transition_states.push_back(a);
        }
        
        // add the length of the final character state
        double time_since_last_transition = 0.0;
        double transition_times_sum = 0.0;
        for (size_t j = 0; j < transition_times.size(); j++)
        {
            transition_times_sum += transition_times[j];
        }
        time_since_last_transition = branch_length - transition_times_sum;

        transition_times.push_back(time_since_last_transition);
        
        // make SIMMAP string
        std::string simmap_string = "{";
        for (size_t i = transition_times.size(); i > 0; i--)
        {
            simmap_string = simmap_string + StringUtilities::toString(transition_states[i - 1]) + "," + StringUtilities::toString(transition_times[i - 1]);
            if (i != 1)
            {
                simmap_string = simmap_string + ":";
            }
        }
        simmap_string = simmap_string + "}";
        
        // save the character history for this branch
        character_histories[node_index] = new std::string(simmap_string);
        
        // calculate average diversification rates on this branch
        average_speciation[node_index] = total_speciation_rate / num_dts;
        average_extinction[node_index] = total_extinction_rate / num_dts;
        
        // recurse towards tips
        recursivelyDrawStochasticCharacterMap(left, l, character_histories);
        recursivelyDrawStochasticCharacterMap(right, r, character_histories);
    }
}


RevLanguage::RevPtr<RevLanguage::RevVariable> StateDependentSpeciationExtinctionProcess::executeProcedure(const std::string &name, const std::vector<DagNode *> args, bool &found)
{    
    if (name == "clampCharData")
    {
        found = true;
        
        const AbstractHomologousDiscreteCharacterData& v = static_cast<const TypedDagNode<AbstractHomologousDiscreteCharacterData > *>( args[0] )->getValue();
    
        // check if the tip names match
        bool match = true;
        std::vector<string> tips = value->getTipNames();
        for (size_t i = 0; i < tips.size(); i++)
        {
            found = false;
            for (size_t j = 0; j < v.getNumberOfTaxa(); j++)
            {
                if (tips[i] == v[j].getTaxonName()) 
                {
                    found = true;
                    break;
                }
            }
            if (found == false)
            {
                match = false;
                break;
            }
        }
        if (match == false)
        {
            throw RbException("To clamp a character data object all taxa present in the tree must be present in the character data.");
        }
        
        static_cast<TreeDiscreteCharacterData*>(this->value)->setCharacterData( v.clone() );
   
        // simulate character history over the tree conditioned on the new tip data
        size_t num_nodes = value->getNumberOfNodes();
        std::vector<std::string*> character_histories(num_nodes);
        drawStochasticCharacterMap(character_histories);

        return NULL;
    }
    
    if (name == "getCharData") 
    { 
        found = true;
        RevLanguage::AbstractHomologousDiscreteCharacterData *tip_states = new RevLanguage::AbstractHomologousDiscreteCharacterData( getCharacterData() );
        return new RevLanguage::RevVariable( tip_states );
    }
    if ( name == "getCharHistory" )
    {
        found = true;
        return new RevLanguage::RevVariable( new RlString( simmap ) );        
    }
    return TypedDistribution<Tree>::executeProcedure( name, args, found );
}


void StateDependentSpeciationExtinctionProcess::executeMethod(const std::string &name, const std::vector<const DagNode *> &args, RbVector<double> &rv) const
{
   
    if ( name == "averageSpeciationRate" )
    {
        rv = average_speciation;        
    }
    else if ( name == "averageExtinctionRate" )
    {
        rv = average_extinction;        
    }
    else if ( name == "getTimeInState" )
    {
        rv = time_in_state;        
    }
    else
    {
        throw RbException("The character dependent birth-death process does not have a member method called '" + name + "'.");
    }

}


/**
 * Get the affected nodes by a change of this node.
 * If the root age has changed than we need to call get affected again.
 */
void StateDependentSpeciationExtinctionProcess::getAffected(RbOrderedSet<DagNode *> &affected, RevBayesCore::DagNode *affecter)
{
    
    if ( affecter == process_age)
    {
        dag_node->getAffectedNodes( affected );
    }
    
}


/**
 * Get the event rate
 */
double StateDependentSpeciationExtinctionProcess::getEventRate(void) const
{

    if ( rate != NULL )
    {
        return rate->getValue();
    }
    else
    {
        return 1.0;
    }

}


/**
 * Get the event rate generator
 */
const RateGenerator& StateDependentSpeciationExtinctionProcess::getEventRateMatrix(void) const
{

    if ( Q != NULL )
    {
        return Q->getValue();
    }
    else
    {
        return Q_default;
    }

}


double StateDependentSpeciationExtinctionProcess::getOriginAge( void ) const
{

    return process_age->getValue();
}


/**
 * By default, the root age is assumed to be equal to the origin time.
 * This should be overridden if a distinct root age is needed
 */
double StateDependentSpeciationExtinctionProcess::getRootAge( void ) const
{

    if (use_origin)
    {
        if (value->getNumberOfNodes() > 0)
        {
            return value->getRoot().getAge();
        }
        else
        {
            return 0;
        }
    }
    else
        return getOriginAge();
}


/**
 * Get the stationary root frequencies
 */
std::vector<double> StateDependentSpeciationExtinctionProcess::getRootFrequencies(void) const
{

    if ( pi != NULL )
    {
        return pi->getValue();
    }
    else
    {
        return std::vector<double>(num_states, 1.0/num_states);
    }

}


/**
 * Keep the current value and reset some internal flags. Nothing to do here.
 */
void StateDependentSpeciationExtinctionProcess::keepSpecialization(DagNode *affecter)
{
    
    if ( affecter == process_age )
    {
        dag_node->keepAffected();
    }
    
    // reset all flags
    for (std::vector<bool>::iterator it = this->dirty_nodes.begin(); it != this->dirty_nodes.end(); ++it)
    {
        (*it) = false;
    }

    for (std::vector<bool>::iterator it = this->changed_nodes.begin(); it != this->changed_nodes.end(); ++it)
    {
        (*it) = false;
    }

}


double StateDependentSpeciationExtinctionProcess::lnProbTreeShape(void) const
{
    // the birth death divergence times density is derived for a (ranked) unlabeled oriented tree
    // so we convert to a (ranked) labeled non-oriented tree probability by multiplying by 2^{n+m-1} / n!
    // where n is the number of extant tips, m is the number of extinct tips

    int num_taxa = (int)value->getNumberOfTips();
    int num_extinct = (int)value->getNumberOfExtinctTips();
    int num_sa = (int)value->getNumberOfSampledAncestors();

    return (num_taxa - num_sa - 1) * RbConstants::LN2 - RbMath::lnFactorial(num_taxa - num_extinct);
}


std::vector<double> StateDependentSpeciationExtinctionProcess::pExtinction(double start, double end) const
{
    
    double samplingProbability = rho->getValue();
    state_type initial_state = std::vector<double>(2*num_states,0);
    for (size_t i=0; i<num_states; ++i)
    {
        initial_state[i] = 1.0 - samplingProbability;
        initial_state[num_states + i] = samplingProbability;
    }
    
    numericallyIntegrateProcess(initial_state, start, end, true, false);
    
    return initial_state;
}


double StateDependentSpeciationExtinctionProcess::pSurvival(double start, double end) const
{

    state_type initial_state = pExtinction(start,end);

    double prob = 0.0;
    const RbVector<double> &freqs = getRootFrequencies();
    for (size_t i=0; i<num_states; ++i)
    {
        prob += freqs[i] * initial_state[i];
        //        prob += freqs[i]*(1.0-initial_state[i])*(1.0-initial_state[i])*speciation_rates[i];
        
    }
    
    return 1.0-prob;
}



/**
 * Redraw the current value. We delegate this to the simulate method.
 */
void StateDependentSpeciationExtinctionProcess::redrawValue( void )
{
    
    simulateTree();
    
}



/**
 * Restore the current value and reset some internal flags.
 * If the root age variable has been restored, then we need to change the root age of the tree too.
 */
void StateDependentSpeciationExtinctionProcess::restoreSpecialization(DagNode *affecter)
{
    
    if ( affecter == process_age )
    {
        if ( use_origin == false )
        {
            value->getRoot().setAge( process_age->getValue() );
        }

        if ( dag_node != NULL )
        {
            dag_node->restoreAffected();
        }
    }
    
    // reset the flags
    for (std::vector<bool>::iterator it = dirty_nodes.begin(); it != dirty_nodes.end(); ++it)
    {
        (*it) = false;
    }

    // restore the active likelihoods vector
    for (size_t index = 0; index < changed_nodes.size(); ++index)
    {
        // we have to restore, that means if we have changed the active likelihood vector
        // then we need to revert this change
        if ( changed_nodes[index] == true )
        {
            active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
        }

        // set all flags to false
        changed_nodes[index] = false;
    }

}



void StateDependentSpeciationExtinctionProcess::setCladogenesisMatrix(const TypedDagNode< CladogeneticSpeciationRateMatrix >* cm)
{
    
    // remove the old parameter first
    this->removeParameter( cladogenesis_matrix );
    
    // set the value
    cladogenesis_matrix = cm;
    
    // should we use the event map for the speciation rates?
    use_cladogenetic_events = true;
    
    // add the new parameter
    this->addParameter( cladogenesis_matrix );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
}


void StateDependentSpeciationExtinctionProcess::setSerialSamplingRates(const TypedDagNode< RbVector<double> >* r)
{

    // remove the old parameter first
    this->removeParameter( psi );

    // set the value
    psi = r;

    // add the new parameter
    this->addParameter( psi );

    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
}


void StateDependentSpeciationExtinctionProcess::setSampleCharacterHistory(bool sample_history)
{
    sample_character_history = sample_history;
}


void StateDependentSpeciationExtinctionProcess::setSpeciationRates(const TypedDagNode< RbVector<double> >* r)
{
    
    // remove the old parameter first
    this->removeParameter( lambda );
    
    // set the value
    lambda = r;
    
    // should we use the event map for the speciation rates?
    use_cladogenetic_events = false;
    
    // add the new parameter
    this->addParameter( lambda );
    
    // redraw the current value
    if ( this->dag_node == NULL || this->dag_node->isClamped() == false )
    {
        this->redrawValue();
    }
}


void StateDependentSpeciationExtinctionProcess::setNumberOfTimeSlices( double n )
{
    
    NUM_TIME_SLICES = n;
    dt = process_age->getValue() / NUM_TIME_SLICES;
    
}


/**
 * Set the current value.
 */
void StateDependentSpeciationExtinctionProcess::setValue(Tree *v, bool f )
{

    value->getTreeChangeEventHandler().removeListener( this );

    // delegate to super class
    //    TypedDistribution<Tree>::setValue(v, f);
    static_cast<TreeDiscreteCharacterData *>(this->value)->setTree( *v );

    resizeVectors(v->getNumberOfNodes());
    
    // clear memory
    delete v;
    
    value->getTreeChangeEventHandler().addListener( this );
    
    if ( process_age != NULL && use_origin == false )
    {
        const StochasticNode<double> *stoch_process_age = dynamic_cast<const StochasticNode<double>* >(process_age);
        if ( stoch_process_age != NULL )
        {
            const_cast<StochasticNode<double> *>(stoch_process_age)->setValue( new double( value->getRoot().getAge() ), f);
        }
        else
        {
            value->getRoot().setAge( process_age->getValue() );
        }
        
    }

    // make character data objects -- all unknown/missing
    std::vector<string> tips = value->getTipNames();
    HomologousDiscreteCharacterData<NaturalNumbersState> *tip_data = new HomologousDiscreteCharacterData<NaturalNumbersState>();
    for (size_t i = 0; i < tips.size(); i++)
    {
        DiscreteTaxonData<NaturalNumbersState> this_tip_data = DiscreteTaxonData<NaturalNumbersState>(tips[i]);
        NaturalNumbersState state = NaturalNumbersState(0, num_states);
        state.setState("?");
        this_tip_data.addCharacter(state);
        tip_data->addTaxonData(this_tip_data);
    }
    static_cast<TreeDiscreteCharacterData*>(this->value)->setCharacterData(tip_data);
   
    // simulate character history over the new tree
    size_t num_nodes = value->getNumberOfNodes();
    if (num_nodes > 2)
    {
        std::vector<std::string*> character_histories(num_nodes);
        drawStochasticCharacterMap(character_histories);
    }
}


std::vector<double> StateDependentSpeciationExtinctionProcess::calculateTotalSpeciationRatePerState( void ) 
{
    std::vector<double> total_rates = std::vector<double>(num_states, 0);
    std::map<std::vector<unsigned>, double> eventMap;
    std::vector<double> speciation_rates;
    std::map<std::vector<unsigned>, double>::iterator it;
    if ( use_cladogenetic_events == true )
    {
        // get cladogenesis event map (sparse speciation rate matrix)
        eventMap = cladogenesis_matrix->getValue().getEventMap();
        // iterate over each cladogenetic event possible
        for (it = eventMap.begin(); it != eventMap.end(); it++)
        {
            const std::vector<unsigned>& states = it->first;
            total_rates[states[0]] += it->second;
        }
    }
    else
    {
        speciation_rates = lambda->getValue();
        for (size_t i = 0; i < num_states; i++)
        {
            total_rates[i] += speciation_rates[i];    
        }
    }
    return total_rates;
}


std::vector<double> StateDependentSpeciationExtinctionProcess::calculateTotalAnageneticRatePerState( void ) 
{
    std::vector<double> total_rates = std::vector<double>(num_states, 0);
    const RateGenerator *rate_matrix = &getEventRateMatrix();
    for (size_t i = 0; i < num_states; i++)
    {
        for (size_t j = 0; j < num_states; j++)
        {
            if (i != j)
            {
                total_rates[i] += rate_matrix->getRate(i, j, 0.0, getEventRate());
            } 
        }
    }

    return total_rates;
}


/**
 *
 */
void StateDependentSpeciationExtinctionProcess::simulateTree( void )
{
    if ( use_origin == true )
    {
        // if originAge is set we start with one lineage
        // if rootAge is set we start with two lineages and their speciation event
        throw RbException("Simulations are currently only implemented when rootAge is set. You set the originAge.");
    }

    // a vector keeping track of the lineages currently surviving in each state
    // as we simulate forward in time
    std::vector< std::vector<size_t> > lineages_in_state = std::vector< std::vector<size_t> >(num_states, std::vector<size_t>());
    std::vector< std::vector<size_t> > extinct_lineages_in_state = std::vector< std::vector<size_t> >(num_states, std::vector<size_t>());

    // CharacterData object to hold the tip states
    HomologousDiscreteCharacterData<NaturalNumbersState> *tip_data = new HomologousDiscreteCharacterData<NaturalNumbersState>();

    // vectors keeping track of the total rate of all
    // cladogenetic/anagenetic/extinction events for each state
    std::vector<double> extinction_rates = mu->getValue();
    std::vector<double> total_speciation_rates = calculateTotalSpeciationRatePerState();
    std::vector<double> total_anagenetic_rates = calculateTotalAnageneticRatePerState();
    std::vector<double> total_rate_for_state = std::vector<double>(num_states, 0);
    for (size_t i = 0; i < num_states; i++)
    {
        total_rate_for_state[i] = extinction_rates[i] + total_speciation_rates[i] + total_anagenetic_rates[i];
    }

    // get the speciation rates, extinction rates, and Q matrix
    std::map<std::vector<unsigned>, double> eventMap;
    std::vector<double> speciation_rates;
    std::map<std::vector<unsigned>, double>::iterator it;
    if ( use_cladogenetic_events == true )
    {
        eventMap = cladogenesis_matrix->getValue().getEventMap();
    }
    else
    {
        speciation_rates = lambda->getValue();
    }
    const RateGenerator *rate_matrix = &getEventRateMatrix();

    // a vector of all nodes in our simulated tree
    std::vector<TopologyNode*> nodes;

    // initialize the root node
    TopologyNode* root = new TopologyNode(0);
    double t = process_age->getValue();
    root->setAge(t);
    root->setNodeType(false, true, true);
    nodes.push_back(root);

    // now draw a state for the root cladogenetic event
    
    // get root frequencies
    const RbVector<double> &freqs = getRootFrequencies();
    
    std::map<std::vector<unsigned>, double> sample_probs;
    double sample_probs_sum = 0.0;
    
    // calculate probabilities for each state
    if ( use_cladogenetic_events == true )
    {
        // iterate over each cladogenetic event possible
        // and initialize probabilities for each clado event
        for (it = eventMap.begin(); it != eventMap.end(); it++)
        {
            const std::vector<unsigned>& states = it->first;
            double speciation_rate = it->second;
            
            // we need to sample from the ancestor, left, and right states jointly,
            // so keep track of the probability of each clado event
            double prob = freqs[states[0]] * speciation_rate;
            sample_probs[ states ] = prob;
            sample_probs_sum += prob;
        }
    }
    else
    {
        for (size_t i = 0; i < num_states; i++)
        {
            std::vector<unsigned> states = boost::assign::list_of(i)(i)(i);
            sample_probs[ states ] = speciation_rates[i] * freqs[i];
            sample_probs_sum += speciation_rates[i] * freqs[i];
        }
    }
    
    // sample ancestor, left, and right character states from probs
    size_t a = 0, l = 0, r = 0;
    
    if (sample_probs_sum == 0)
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        size_t u = rng->uniform01() * sample_probs.size();
        size_t v = 0;
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            if (u < v)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                break;
            }
            v++;
        }
    }
    else
    {
        RandomNumberGenerator* rng = GLOBAL_RNG;
        double u = rng->uniform01() * sample_probs_sum;
        
        for (it = sample_probs.begin(); it != sample_probs.end(); it++)
        {
            u -= it->second;
            if (u < 0.0)
            {
                const std::vector<unsigned>& states = it->first;
                a = states[0];
                l = states[1];
                r = states[2];
                break;
            }
        }
    }

    // make nodes for each daughter
    TopologyNode* left = new TopologyNode(1);
    left->setAge(t);
    root->addChild(left);
    left->setParent(root);
    left->setNodeType(true, false, false);
    lineages_in_state[l].push_back(1);
    nodes.push_back(left);

    TopologyNode* right = new TopologyNode(2);
    right->setAge(t);
    root->addChild(right);
    right->setParent(root);
    right->setNodeType(true, false, false);
    lineages_in_state[r].push_back(2);
    nodes.push_back(right);

    // simulate moving forward in time
    while (true) {

        // sum over all rates for all states (multiplied by num lineages in each state)
        double total_rate = 0;
        for (size_t i = 0; i < num_states; i++)
        {
            total_rate += total_rate_for_state[i] * lineages_in_state[i].size();    
        }

        // draw the time to next event
        RandomNumberGenerator* rng = GLOBAL_RNG;
        double dt = RbStatistics::Exponential::rv( total_rate, *rng );
        t = t - dt;
      
        if (t < 0)
        {
            t = 0;
        }

        // extend all surviving branches to the new time
        size_t num_lineages = 0;
        for (size_t i = 0; i < num_states; i++)
        {
            for (size_t j = 0; j < lineages_in_state[i].size(); j++)
            {
                size_t idx = lineages_in_state[i][j];
                nodes[idx]->setAge(t);
                num_lineages++;
            }
        }

        // stop if we have reached the present or exceeded max num lineages
        if (t == 0 || num_lineages >= max_num_lineages) 
        {
            for (size_t i = 0; i < nodes.size(); i++)
            {
                if (nodes[i]->getAge() == t) 
                {
                    std::stringstream ss;
                    ss << "sp" << i;
                    std::string name = ss.str();
                    nodes[i]->setName(name);
                    nodes[i]->setNodeType(true, false, false);
                }
            }

            // set CharacterData object for each tip state
            for (size_t i = 0; i < num_states; i++)
            {
                for (size_t j = 0; j < lineages_in_state[i].size(); j++)
                {
                    size_t this_node = lineages_in_state[i][j];
                    if (nodes[this_node]->isTip() == true)
                    {
                        DiscreteTaxonData<NaturalNumbersState> this_tip_data = DiscreteTaxonData<NaturalNumbersState>(nodes[this_node]->getName());
                        NaturalNumbersState state = NaturalNumbersState(i, num_states);
                        this_tip_data.addCharacter(state);
                        tip_data->addTaxonData(this_tip_data);
                    }
                }
                if (prune_extinct_lineages == false)
                {
                    for (size_t j = 0; j < extinct_lineages_in_state[i].size(); j++)
                    {
                        size_t this_node = extinct_lineages_in_state[i][j];
                        if (nodes[this_node]->isTip() == true)
                        {
                            DiscreteTaxonData<NaturalNumbersState> this_tip_data = DiscreteTaxonData<NaturalNumbersState>(nodes[this_node]->getName());
                            NaturalNumbersState state = NaturalNumbersState(i, num_states);
                            this_tip_data.addCharacter(state);
                            tip_data->addTaxonData(this_tip_data);
                        }
                    }
                }
            }
            break;
        }

        // determine the state for the event that occurred
        size_t event_state = 0; 
        double u = rng->uniform01() * total_rate;
        for (size_t i = 0; i < num_states; i++)
        {
            u -= total_rate_for_state[i] * lineages_in_state[i].size();
            if (u < 0)
            {
                event_state = i;
                break;
            }
        }

        // determine the type of event
        std::string event_type = ""; 
        u = rng->uniform01() * total_rate_for_state[event_state];
        while (true) {
            u = u - extinction_rates[event_state];
            if (u < 0) 
            {
                event_type = "extinction";
                break;
            }
            u = u - total_speciation_rates[event_state];
            if (u < 0) 
            {
                event_type = "speciation";
                break;
            }
            u = u - total_anagenetic_rates[event_state];
            if (u < 0) 
            {
                event_type = "anagenetic";
                break;
            }
        }

        // determine which lineage gets the event
        size_t event_index = 0;
        u = rng->uniform01() * static_cast<double>(lineages_in_state[event_state].size());
        event_index = floor(lineages_in_state[event_state][u]);

        if (event_type == "extinction")
        {
            extinct_lineages_in_state[event_state].push_back(event_index);
            lineages_in_state[event_state].erase(std::remove(lineages_in_state[event_state].begin(), lineages_in_state[event_state].end(), event_index), lineages_in_state[event_state].end());
            std::stringstream ss;
            ss << "ex" << event_index;
            std::string name = ss.str();
            nodes[event_index]->setName(name);
            nodes[event_index]->setNodeType(true, false, false);
        }
        
        if (event_type == "anagenetic")
        {
            // remove this lineage from the current state
            lineages_in_state[event_state].erase(std::remove(lineages_in_state[event_state].begin(), lineages_in_state[event_state].end(), event_index), lineages_in_state[event_state].end());

            // draw a new state
            size_t new_state = 0;
            u = rng->uniform01() * total_anagenetic_rates[event_state];
            for (size_t i = 0; i < this->num_states; i++)
            {
                if (i != event_state)
                {
                    u -= rate_matrix->getRate( event_state, i, 0, getEventRate() );
                    if (u < 0.0)
                    {
                        new_state = i;
                        break;
                    }
                }
            } 
            lineages_in_state[new_state].push_back(event_index);
        }
        
        if (event_type == "speciation")
        {
            // gather the probabilities for each type of cladogenetic event
            std::map<std::vector<unsigned>, double> sample_probs;
            double sample_probs_sum = 0.0;
            if ( use_cladogenetic_events == true )
            {
                // iterate over each cladogenetic event possible
                for (it = eventMap.begin(); it != eventMap.end(); it++)
                {
                    const std::vector<unsigned>& states = it->first;
                    double speciation_rate = it->second;
                    if (states[0] == event_state) 
                    {
                        // we need to sample from the ancestor, left, and right states jointly,
                        // so keep track of the probability of each clado event
                        double prob = speciation_rate;
                        sample_probs[ states ] = prob;
                        sample_probs_sum += prob;
                    }
                }
            }
            else
            {
                std::vector<unsigned> states = boost::assign::list_of(event_state)(event_state)(event_state);
                sample_probs[ states ] = speciation_rates[event_state];
                sample_probs_sum += speciation_rates[event_state];
            }
            
            // sample ancestor, left, and right character states from probs
            size_t a = 0, l = 0, r = 0;
            
            if (sample_probs_sum == 0)
            {
                RandomNumberGenerator* rng = GLOBAL_RNG;
                size_t u = rng->uniform01() * sample_probs.size();
                size_t v = 0;
                for (it = sample_probs.begin(); it != sample_probs.end(); it++)
                {
                    if (u < v)
                    {
                        const std::vector<unsigned>& states = it->first;
                        a = states[0];
                        l = states[1];
                        r = states[2];
                        break;
                    }
                    v++;
                }
            }
            else
            {
                RandomNumberGenerator* rng = GLOBAL_RNG;
                double u = rng->uniform01() * sample_probs_sum;
                
                for (it = sample_probs.begin(); it != sample_probs.end(); it++)
                {
                    u -= it->second;
                    if (u < 0.0)
                    {
                        const std::vector<unsigned>& states = it->first;
                        a = states[0];
                        l = states[1];
                        r = states[2];
                        break;
                    }
                }
            }
            
            // make nodes for each daughter
            size_t index = nodes.size();
            TopologyNode* left = new TopologyNode(index);
            left->setAge(t);
            nodes[event_index]->addChild(left);
            left->setParent(nodes[event_index]);
            left->setNodeType(true, false, false);
            lineages_in_state[l].push_back(index);
            nodes.push_back(left);

            index = nodes.size();
            TopologyNode* right = new TopologyNode(index);
            right->setAge(t);
            nodes[event_index]->addChild(right);
            right->setParent(nodes[event_index]);
            right->setNodeType(true, false, false);
            lineages_in_state[r].push_back(index);
            nodes.push_back(right);
           
            // remove the parent node from our vector of current lineages
            nodes[event_index]->setNodeType(false, false, true);
            lineages_in_state[event_state].erase(std::remove(lineages_in_state[event_state].begin(), lineages_in_state[event_state].end(), event_index), lineages_in_state[event_state].end());
        }
    }
   
    // make a tree object 
    Tree *psi = new Tree();
    psi->setRoot(root, true);
    psi->setRooted(true);
    
    if (prune_extinct_lineages == true)
    {
        for (size_t i = 0; i < num_states; i++)
        {
            for (size_t j = 0; j < extinct_lineages_in_state[i].size(); j++)
            {
                size_t this_node = extinct_lineages_in_state[i][j];
                if (nodes[this_node]->isTip() == true)
                {
                    psi->dropTipNodeWithName( nodes[this_node]->getName() );
                }
            }
        }
    }
    resizeVectors(psi->getNumberOfNodes());
    setValue(psi);
    static_cast<TreeDiscreteCharacterData*>(this->value)->setCharacterData(tip_data);
    
    size_t num_nodes = value->getNumberOfNodes();
    if (num_nodes > 2)
    {
        std::vector<std::string*> character_histories(num_nodes);
        drawStochasticCharacterMap(character_histories);
    }
    
}



/**
 * Swap the parameters held by this distribution.
 *
 *
 * \param[in]    oldP      Pointer to the old parameter.
 * \param[in]    newP      Pointer to the new parameter.
 */
void StateDependentSpeciationExtinctionProcess::swapParameterInternal(const DagNode *oldP, const DagNode *newP)
{
    
    if ( oldP == process_age )
    {
        process_age = static_cast<const TypedDagNode<double>* >( newP );
    }
    if ( oldP == mu )
    {
        mu = static_cast<const TypedDagNode<RbVector<double> >* >( newP );
    }
    if ( oldP == lambda )
    {
        lambda = static_cast<const TypedDagNode<RbVector<double> >* >( newP );
    }
    if ( oldP == psi )
    {
        psi = static_cast<const TypedDagNode<RbVector<double> >* >( newP );
    }
    if ( oldP == Q )
    {
        Q = static_cast<const TypedDagNode<RateGenerator>* >( newP );
    }
    if ( oldP == rate )
    {
        rate = static_cast<const TypedDagNode<double>* >( newP );
    }
    if ( oldP == pi )
    {
        pi = static_cast<const TypedDagNode<Simplex>* >( newP );
    }
    if ( oldP == rho )
    {
        rho = static_cast<const TypedDagNode<double>* >( newP );
    }
    if ( oldP == cladogenesis_matrix )
    {
        cladogenesis_matrix = static_cast<const TypedDagNode<CladogeneticSpeciationRateMatrix>* >( newP );
    }
    
}



/**
 * Touch the current value and reset some internal flags.
 * If the root age variable has been restored, then we need to change the root age of the tree too.
 */
void StateDependentSpeciationExtinctionProcess::touchSpecialization(DagNode *affecter, bool touchAll)
{
    
    if ( affecter == process_age )
    {
        if ( use_origin == false)
        {
            value->getRoot().setAge( process_age->getValue() );
        }

        if ( dag_node != NULL )
        {
            dag_node->touchAffected();
        }
    }
    
    if ( affecter != this->dag_node )
    {
        
        for (std::vector<bool>::iterator it = dirty_nodes.begin(); it != dirty_nodes.end(); ++it)
        {
            (*it) = true;
        }
        
        // flip the active likelihood pointers
        for (size_t index = 0; index < changed_nodes.size(); ++index)
        {
            if ( changed_nodes[index] == false )
            {
                active_likelihood[index] = (active_likelihood[index] == 0 ? 1 : 0);
                changed_nodes[index] = true;
            }
        }
    }
    
}


/**
 * Wrapper function for the ODE time stepper function.
 */
void StateDependentSpeciationExtinctionProcess::numericallyIntegrateProcess(state_type &likelihoods, double begin_age, double end_age, bool backward_time, bool extinction_only) const
{
    const std::vector<double> &extinction_rates = mu->getValue();
    SSE_ODE ode = SSE_ODE(extinction_rates, &getEventRateMatrix(), getEventRate(), backward_time, extinction_only);
    if ( use_cladogenetic_events == true )
    {
        cladogenesis_matrix->getValue(); // we must call getValue() to update the speciation and extinction rates in the event map
        
        // get cladogenesis event map (sparse speciation rate matrix)
        std::map<std::vector<unsigned>, double> event_map = cladogenesis_matrix->getValue().getEventMap();
        
        ode.setEventMap( event_map );
    }
    else
    {
        const std::vector<double> &speciation_rates = lambda->getValue();
        ode.setSpeciationRate( speciation_rates );
    }

    if ( psi != NULL )
    {
        const std::vector<double> &serial_sampling_rates = psi->getValue();
        ode.setSerialSamplingRate( serial_sampling_rates );
    }

    typedef boost::numeric::odeint::runge_kutta_dopri5< state_type > stepper_type;
    boost::numeric::odeint::integrate_adaptive( make_controlled( 1E-6 , 1E-6 , stepper_type() ) , ode , likelihoods , begin_age , end_age , dt );

    // catch negative extinction probabilities that can result from
    // rounding errors in the ODE stepper
    for (size_t i = 0; i < 2 * num_states; ++i)
    {
        
//        if ( likelihoods[i] < 0.0 )
//            std::cerr << "Rounding error (<0)!:\t\t" << likelihoods[i] << "\n";
//        if ( likelihoods[i] > 1.0 )
//            std::cerr << "Rounding error (>1)!:\t\t" << likelihoods[i] << "\n";
        
        // Sebastian: The likelihoods here are probability densities (not log-transformed).
        // These are densities because they are multiplied by the probability density of the speciation event happening.
        likelihoods[i] = ( likelihoods[i] < 0.0 ? 0.0 : likelihoods[i] );
//        likelihoods[i] = ( likelihoods[i] > 1.0 ? 1.0 : likelihoods[i] );
    }
}


/**
 * Resize various vectors depending on the current number of nodes.
 */
void StateDependentSpeciationExtinctionProcess::resizeVectors(size_t num_nodes)
{
    active_likelihood = std::vector<bool>(num_nodes, false);
    changed_nodes = std::vector<bool>(num_nodes, false);
    dirty_nodes = std::vector<bool>(num_nodes, true);
    node_partial_likelihoods = std::vector<std::vector<std::vector<double> > >(num_nodes, std::vector<std::vector<double> >(2,std::vector<double>(2*num_states,0)));
    scaling_factors = std::vector<std::vector<double> >(num_nodes, std::vector<double>(2,0.0) );
    average_speciation = std::vector<double>(num_nodes, 0.0);
    average_extinction = std::vector<double>(num_nodes, 0.0);
    time_in_state = std::vector<double>(num_states, 0.0);    
}
