/**
 * @file
 * This file contains the distribution class for a character state evolving along a tree.
 * This abstract base class can be derived for any character evolution model with homogeneous sites. A
 * homogeneous model over sites is a model where all sites are drawn from the same distribution. 
 *
 * The pruning algorithm is implemented in this base class and calles some few pure virtual methods. 
 * The important functions you have to override are:
 * - getRootFrequencies()
 * - updateTransitionProbabilities()
 *
 * The data is stored for convenience in this class in a matrix (std::vector<std::vector< unsigned > >) and can
 * be compressed.
 *
 *
 * @brief Declaration of the character state evolution along a tree class.
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date:$
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 * @version 1.0
 * @since 2012-06-17, version 1.0
 * @interface TypedDagNode
 *
 * $Id:$
 */



#ifndef AbstractSiteHomogeneousCharEvoModel_H
#define AbstractSiteHomogeneousCharEvoModel_H

#include "AbstractCharacterData.h"
#include "CharacterData.h"
#include "RateMatrix.h"
#include "TopologyNode.h"
#include "TransitionProbabilityMatrix.h"
#include "Tree.h"
#include "TreeChangeEventListener.h"
#include "TypedDistribution.h"

namespace RevBayesCore {
    
    template<class charType, class treeType>
    class AbstractSiteHomogeneousCharEvoModel : public TypedDistribution< AbstractCharacterData >, public TreeChangeEventListener {
        
    public:
        // Note, we need the size of the alignment in the constructor to correctly simulate an initial state
        AbstractSiteHomogeneousCharEvoModel(const TypedDagNode<treeType> *t, bool c, size_t nSites);
        AbstractSiteHomogeneousCharEvoModel(const AbstractSiteHomogeneousCharEvoModel &n);                                                                                          //!< Copy constructor
        virtual                                                            ~AbstractSiteHomogeneousCharEvoModel(void);                                                              //!< Virtual destructor
        
        // public member functions
        // pure virtual
        virtual AbstractSiteHomogeneousCharEvoModel*                        clone(void) const = 0;                                                                      //!< Create an independent clone
        
        // virtual (you need to overwrite this method if you have additional parameters)
        virtual void                                                        swapParameter(const DagNode *oldP, const DagNode *newP);                                //!< Implementation of swaping paramoms
        
        // non-virtual
        double                                                              computeLnProbability(void);
        void                                                                fireTreeChangeEvent(const TopologyNode &n);                                             //!< The tree has changed and we want to know which part.
        void                                                                setValue(AbstractCharacterData *v);                                                   //!< Set the current value, e.g. attach an observation (clamp)
        void                                                                redrawValue(void);
        
    protected:
        // helper method for this and derived classes
        void                                                                recursivelyFlagNodeDirty(const TopologyNode& n);
        
        // virtual methods that may be overwritten, but then the derived class should call this methods
        virtual void                                                        keepSpecialization(DagNode* affecter);
        virtual void                                                        restoreSpecialization(DagNode *restorer);
        virtual void                                                        touchSpecialization(DagNode *toucher);
        
        // pure virtual methods
        virtual void                                                        updateTransitionProbabilities(size_t nodeIdx, double brlen) = 0;
        virtual const std::vector<double>&                                  getRootFrequencies(void) = 0;
        
        // members
        double                                                              lnProb;
        const size_t                                                        numChars;
        size_t                                                              numSites;
        const TypedDagNode<treeType>*                                       tau;
        TransitionProbabilityMatrix                                         transitionProbMatrix;
        
        // the likelihoods
        double*                                                             partialLikelihoods;
        std::vector<size_t>                                                 activeLikelihood;
        
        // the data
        std::vector<std::vector<unsigned int> >                             charMatrix;
        std::vector<std::vector<bool> >                                     gapMatrix;
        std::vector<size_t>                                                 patternCounts;
        size_t                                                              numPatterns;
        bool                                                                compressed;
        
    private:
        // private methods
        void                                                                compress(void);
        void                                                                computeRootLikelihood(size_t root, size_t l, size_t r);
        void                                                                computeInternalNodeLikelihood(const TopologyNode &n, size_t nIdx, size_t l, size_t r);
        void                                                                computeTipLikelihood(const TopologyNode &node, size_t nIdx);
        void                                                                fillLikelihoodVector(const TopologyNode &n, size_t nIdx);
        void                                                                simulate(const TopologyNode& node, std::vector< TaxonData< charType > > &t);
        
        std::vector<bool>                                                   changedNodes;
        std::vector<bool>                                                   dirtyNodes;
        
        // offsets for nodes
        size_t                                                              activeLikelihoodOffset;
        size_t                                                              nodeOffset;
        size_t                                                              siteOffset;
    };
    
}


#include "DiscreteCharacterState.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "TopologyNode.h"
#include "TransitionProbabilityMatrix.h"

#include <cmath>

template<class charType, class treeType>
RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::AbstractSiteHomogeneousCharEvoModel(const TypedDagNode<treeType> *t, bool c, size_t nSites) : TypedDistribution< AbstractCharacterData >(  new CharacterData<charType>() ), 
numSites( nSites ), 
numChars( charType().getNumberOfStates() ),
tau( t ), 
transitionProbMatrix( TransitionProbabilityMatrix(numChars) ),
partialLikelihoods( new double[2*tau->getValue().getNumberOfNodes()*numSites*numChars] ),
activeLikelihood( std::vector<size_t>(tau->getValue().getNumberOfNodes(), 0) )      ,
charMatrix(), 
gapMatrix(),
patternCounts(),
numPatterns( numSites ),
compressed( c ),
changedNodes( std::vector<bool>(tau->getValue().getNumberOfNodes(),false) ),
dirtyNodes( std::vector<bool>(tau->getValue().getNumberOfNodes(), true) ) {
    
    // add the paramoms to the parents list
    this->addParameter( tau );
    tau->getValue().getTreeChangeEventHandler().addListener( this );
    
    
    activeLikelihoodOffset      =  tau->getValue().getNumberOfNodes()*numPatterns*numChars;
    nodeOffset                  =  numPatterns*numChars;
    siteOffset                  =  numChars;
}


template<class charType, class treeType>
RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::AbstractSiteHomogeneousCharEvoModel(const AbstractSiteHomogeneousCharEvoModel &n) : TypedDistribution< AbstractCharacterData >( n ), 
numSites( n.numSites ), 
numChars( n.numChars ),
tau( n.tau ), 
transitionProbMatrix( n.transitionProbMatrix ),
partialLikelihoods( new double[2*tau->getValue().getNumberOfNodes()*numSites*numChars] ),
activeLikelihood( n.activeLikelihood ),
charMatrix( n.charMatrix ), 
gapMatrix( n.gapMatrix ), 
patternCounts( n.patternCounts ),
numPatterns( n.numPatterns ),
compressed( n.compressed ),
changedNodes( n.changedNodes ),
dirtyNodes( n.dirtyNodes ) {
    // parameters are automatically copied
    
    tau->getValue().getTreeChangeEventHandler().addListener( this );
    
    // copy the partial likelihoods
    memcpy(partialLikelihoods, n.partialLikelihoods, 2*tau->getValue().getNumberOfNodes()*numPatterns*numChars*sizeof(double));
    
    activeLikelihoodOffset      =  tau->getValue().getNumberOfNodes()*numPatterns*numChars;
    nodeOffset                  =  numPatterns*numChars;
    siteOffset                  =  numChars;
}


template<class charType, class treeType>
RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::~AbstractSiteHomogeneousCharEvoModel( void ) {
    // We don't delete the paramoms, because they might be used somewhere else too. The model needs to do that!
    
    // remove myself from the tree listeners
    if ( tau != NULL ) 
    {
        // TODO: this needs to be implemented (Sebastian)
        //        tau->getValue().getTreeChangeEventHandler().removeListener( this );
    }
    
    // free the partial likelihoods
    delete [] partialLikelihoods;
}


template<class charType, class treeType>
RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>* RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::clone( void ) const {
    
    return new AbstractSiteHomogeneousCharEvoModel<charType, treeType>( *this );
}



template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::compress( void ) {
 
    charMatrix.clear();
    gapMatrix.clear();
    patternCounts.clear();
    numPatterns = 0;
    
    // resize the matrices
    size_t tips = tau->getValue().getNumberOfTips();
    charMatrix.resize(tips);
    gapMatrix.resize(tips);
    
    std::vector<bool> unique(numSites, true);
    // compress the character matrix if we're asked to
    if ( compressed ) 
    {
        // find the unique site patterns and compute their respective frequencies
        std::vector<TopologyNode*> nodes = tau->getValue().getNodes();
        std::map<std::string,size_t> patterns;
        for (size_t site = 0; site < numSites; ++site) 
        {
            // create the site pattern
            std::string pattern = "";
            for (std::vector<TopologyNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it) 
            {
                if ( (*it)->isTip() ) 
                {
                    AbstractTaxonData& taxon = this->value->getTaxonData( (*it)->getName() );
                    CharacterState &c = taxon.getCharacter(site);
                    pattern += c.getStringValue();
                }
            }
            // check if we have already seen this site pattern
            std::map<std::string, size_t>::const_iterator index = patterns.find( pattern );
            if ( index != patterns.end() ) 
            {
                // we have already seen this pattern
                // increase the frequency counter
                patternCounts[ index->second ]++;
                
                // obviously this site isn't unique nor the first encounter
                unique[site] = false;
            }
            else 
            {
                // create a new pattern frequency counter for this pattern
                patternCounts.push_back(1);
                
                // insert this pattern with the corresponding index in the map
                patterns.insert( std::pair<std::string,size_t>(pattern,numPatterns) );
                
                // increase the pattern counter
                numPatterns++;
                
                // flag that this site is unique (or the first occurence of this pattern)
                unique[site] = true;
            }
        }
    } 
    else 
    {
        // we do not compress
        numPatterns = numSites;
        patternCounts = std::vector<size_t>(numSites,1);
    }
    
    
    // allocate and fill the cells of the matrices
    std::vector<TopologyNode*> nodes = tau->getValue().getNodes();
    for (std::vector<TopologyNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it) 
    {
        if ( (*it)->isTip() ) 
        {
            size_t nodeIndex = (*it)->getIndex();
            AbstractTaxonData& taxon = this->value->getTaxonData( (*it)->getName() );
            
            // resize the column
            charMatrix[nodeIndex].resize(numPatterns);
            gapMatrix[nodeIndex].resize(numPatterns);
            size_t patternIndex = 0;
            for (size_t site = 0; site < numSites; ++site) 
            {
                // only add this site if it is unique
                if ( unique[site] ) 
                {
                    charType &c = static_cast<charType &>( taxon.getCharacter(site) );
                    charMatrix[nodeIndex][patternIndex] = c.getState();
                    gapMatrix[nodeIndex][patternIndex] = c.isGapState();
                    
                    // increase the pattern index
                    patternIndex++;
                }
            }
        }
    }
    
    // finally we resize the partial likelihood vectors to the new pattern counts
    delete [] partialLikelihoods;
    partialLikelihoods = new double[2*tau->getValue().getNumberOfNodes()*numPatterns*numChars];
    
    // set the offsets for easier iteration through the likelihood vector 
    activeLikelihoodOffset      =  tau->getValue().getNumberOfNodes()*numPatterns*numChars;
    nodeOffset                  =  numPatterns*numChars;
    siteOffset                  =  numChars;
    
}

template<class charType, class treeType>
double RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::computeLnProbability( void ) {
    
    // compute the ln probability by recursively calling the probability calculation for each node
    const TopologyNode &root = tau->getValue().getRoot();
    
    // we start with the root and then traverse down the tree
    size_t rootIndex = root.getIndex();
    
    // only necessary if the root is actually dirty
    if ( dirtyNodes[rootIndex] ) 
    {
        // mark as computed
        dirtyNodes[rootIndex] = false;
        
        
        // start by filling the likelihood vector for the two children of the root
        const TopologyNode &left = root.getChild(0);
        size_t leftIndex = left.getIndex();
        fillLikelihoodVector( left, leftIndex );
        const TopologyNode &right = root.getChild(1);
        size_t rightIndex = right.getIndex();
        fillLikelihoodVector( right, rightIndex );
        
        // compute the likelihood of the root
        computeRootLikelihood( rootIndex, leftIndex, rightIndex );
    }
    
    return this->lnProb;
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::computeRootLikelihood( size_t root, size_t left, size_t right) {
    
    // reset the likelihood
    this->lnProb = 0.0;
    
    // get the root frequencies
    const std::vector<double> &f                    = getRootFrequencies();
    std::vector<double>::const_iterator f_end       = f.end();
    std::vector<double>::const_iterator f_begin     = f.begin();
    
    // get the pointers to the partial likelihoods of the left and right subtree
    const double* p_left   = this->partialLikelihoods + activeLikelihood[left]*activeLikelihoodOffset + left*nodeOffset;
    const double* p_right  = this->partialLikelihoods + activeLikelihood[right]*activeLikelihoodOffset + right*nodeOffset;
    
    // create pointers to the likelihood for the sites to iterate over
    const double*   p_site_left     = p_left;
    const double*   p_site_right    = p_right;
    // iterate over all sites
    for (size_t site = 0; site < numPatterns; ++site)
    {
        // temporary variable storing the likelihood
        double tmp = 0.0;
        // get the pointer to the stationary frequencies
        std::vector<double>::const_iterator f_j             = f_begin;
        // get the pointers to the likelihoods for this site
        const double* p_site_left_j   = p_site_left;
        const double* p_site_right_j  = p_site_right;
        // iterate over all starting states
        for (; f_j != f_end; ++f_j) 
        {
            // add the probability of starting from this state
            tmp += *p_site_left_j * *p_site_right_j * *f_j;
                
            // increment pointers
            ++p_site_left_j; ++p_site_right_j;
        }
        // add the likelihood for this site
        this->lnProb += log(tmp);
            
        // increment the pointers to the next site
        p_site_left+=siteOffset; p_site_right+=siteOffset;
    }
        
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::computeInternalNodeLikelihood(const TopologyNode &node, size_t nodeIndex, size_t left, size_t right) {    
    
    // compute the transition probability matrix
    updateTransitionProbabilities( nodeIndex, node.getBranchLength() );
    
    // get the pointers to the partial likelihoods for this node and the two descendant subtrees
    const double*   p_left  = this->partialLikelihoods + this->activeLikelihood[left]*activeLikelihoodOffset + left*nodeOffset;
    const double*   p_right = this->partialLikelihoods + this->activeLikelihood[right]*activeLikelihoodOffset + right*nodeOffset;
    double*         p_node  = this->partialLikelihoods + this->activeLikelihood[nodeIndex]*activeLikelihoodOffset + nodeIndex*nodeOffset;
    
    const double*   tp_begin    = transitionProbMatrix.getElements();
        
    // create the pointers to the likelihood per site
    double*          p_site          = p_node;
    const double*    p_site_left     = p_left;
    const double*    p_site_right    = p_right;
    // compute the per site probabilities
    for (size_t site = 0; site < numPatterns ; ++site)
    {
            
        // get the pointers for this site
        double*             p_a     = p_site;
        const double*       tp_a    = tp_begin;
        // iterate over the possible starting states
        for (size_t c1 = 0; c1 < numChars; ++c1) 
        {
            // temporary variable
            double sum = 0.0;
                
            // get the pointers for this starting state
            const double*       p_site_left_d   = p_site_left;
            const double*       p_site_right_d  = p_site_right;
            const double*       tp_a_d          = tp_a;
            // iterate over all possible terminal states
            for (size_t c2 = 0; c2 < numChars; ++c2 ) 
            {
                sum += *p_site_left_d * *p_site_right_d * *tp_a_d;
                
                // increment the pointers to the next terminal state
                ++tp_a_d; ++p_site_left_d; ++p_site_right_d;
            }
            // store the likelihood for this starting state
            *p_a = sum;
                
            // increment the pointers to the next starting state
            tp_a+=numChars; ++p_a;
        }
            
        // increment the pointers to the next site
        p_site_left+=siteOffset; p_site_right+=siteOffset; p_site+=siteOffset;
    }
        
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::computeTipLikelihood(const TopologyNode &node, size_t nodeIndex) {    
    
    double* p_node = this->partialLikelihoods + this->activeLikelihood[nodeIndex]*activeLikelihoodOffset + nodeIndex*nodeOffset;
    
    const std::vector<bool> &gap_node = this->gapMatrix[nodeIndex];
    const std::vector<unsigned int> &char_node = this->charMatrix[nodeIndex];
    
    // compute the transition probabilities
    updateTransitionProbabilities( nodeIndex, node.getBranchLength() );
    
    
    // the transition probability matrix
    const double*   tp_begin    = transitionProbMatrix.getElements();
        
    // pointers to the data per site
    std::vector<bool>::const_iterator           gap_site    = gap_node.begin();
    std::vector<unsigned int>::const_iterator   char_site   = char_node.begin();
        
    // create the pointer to the likelihoods for this site
    double*     p_site      = p_node;
        
    // iterate over all sites
    for (size_t site = 0; site != numPatterns; ++site) 
    {
            
        // is this site a gap?
        if ( *gap_site == true ) 
        {
            // since this is a gap we need to assume that the actual state could have been any state
                
            // get the pointer to the likelihood memory location for this site
            double*             p_a     = p_site;
            // get the pointer to the transition probability matrix
            const double*       tp_a    = tp_begin;
            // iterate over all initial state for the transitions
            for (size_t c1 = 0; c1 < numChars; ++c1) 
            {
                // temporary variable for starting the sum of the likelihoods to the different states
                double tmp = 0.0;
                    
                // get the pointer to the transition probabilities starting from this state
                const double* d   = tp_a;
                // iterate over all possible terminal states
                for (size_t c2 = 0; c2 < numChars; ++c2) 
                {
                    // since we don't know anything about the observation, we assume it could have been any state and add the likelihood for this terminal state
                    tmp += *d;
                    // increment pointer to next terminal state
                    ++d;
                }
                // store the likelihood
                *p_a = tmp;
                    
                // increment pointers to next starting state
                tp_a+=numChars; ++p_a;
            }
        } 
        else // we have observed a character
        {
                
            // get the original character
            unsigned int org_val = *char_site;
                
            // get the pointer to the likelihood for this site
            // this is were we want to store the partial likelihoods in
            double*             p_a     = p_site;
            // get the pointer to the transition probabilities
            const double*       tp_a    = tp_begin;
            // iterate over all possible initial states 
            for (size_t c1 = 0; c1 < numChars; ++c1) 
            {
                    
                double tmp = 0.0;
                    
                // compute the likelihood that we had a transition from state c1 to the observed state org_val
                // note, the observed state could be ambiguous!
                unsigned int val = org_val;
                // get the pointer to the transition probabilities for the terminal states
                const double* d  = tp_a;
                while ( val != 0 ) // there are still observed states left
                {
                    // check whether we observed this state
                    if ( (val & 1) == 1 ) 
                    {
                        // add the probability
                        tmp += *d;
                    }
                        
                    // removed this state from the observed states
                    val >>= 1;
                    // increment the pointer to the next transition probability
                    ++d;
                }
                // store the likelihood
                *p_a = tmp;
                    
                // increment pointers to next starting state
                tp_a+=numChars; ++p_a;
            }
        }
            
        // increment the pointers to next site
        p_site+=siteOffset; ++char_site; ++gap_site;
    }
    
}



template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::fillLikelihoodVector(const TopologyNode &node, size_t nodeIndex) {    
    
    // check for recomputation
    if ( dirtyNodes[nodeIndex] ) 
    {
        // mark as computed
        dirtyNodes[nodeIndex] = false;
        
        if ( node.isTip() ) 
        {
            // this is a tip node
            // compute the likelihood for the tip and we are done
            computeTipLikelihood(node, nodeIndex);
        } 
        else 
        {
            // this is an internal node
            
            // start by filling the likelihood vector for the two children of this node
            const TopologyNode &left = node.getChild(0);
            size_t leftIndex = left.getIndex();
            fillLikelihoodVector( left, leftIndex );
            const TopologyNode &right = node.getChild(1);
            size_t rightIndex = right.getIndex();
            fillLikelihoodVector( right, rightIndex );
            
            // now compute the likelihoods of this internal node
            computeInternalNodeLikelihood(node,nodeIndex,leftIndex,rightIndex);
        }
    }
}



template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::fireTreeChangeEvent( const RevBayesCore::TopologyNode &n ) {
    
    // call a recursive flagging of all node above (closer to the root) and including this node
    recursivelyFlagNodeDirty( n );
    
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::keepSpecialization( DagNode* affecter ) {
    
    // reset all flags
    for (std::vector<bool>::iterator it = this->dirtyNodes.begin(); it != this->dirtyNodes.end(); ++it) 
    {
        (*it) = false;
    }
    
    for (std::vector<bool>::iterator it = this->changedNodes.begin(); it != this->changedNodes.end(); ++it) 
    {
        (*it) = false;
    }
    
}



template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::recursivelyFlagNodeDirty( const RevBayesCore::TopologyNode &n ) {
    
    // we need to flag this node and all ancestral nodes for recomputation
    size_t index = n.getIndex();
    
    // if this node is already dirty, the also all the ancestral nodes must have been flagged as dirty
    if ( !dirtyNodes[index] ) 
    {
        // the root doesn't have an ancestor
        if ( !n.isRoot() ) 
        {
            recursivelyFlagNodeDirty( n.getParent() );
        }
        
        // set the flag
        dirtyNodes[index] = true;
        
        // if we previously haven't touched this node, then we need to change the active likelihood pointer
        if ( !changedNodes[index] ) 
        {
            activeLikelihood[index] ^= 1;
            changedNodes[index] = true;
        }
        
    }
    
}



template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::redrawValue( void ) {
    
    // delete the old value first
    delete this->value;
    
    // create a new character data object
    this->value = new CharacterData<charType>();
    
    // create a vector of taxon data 
    std::vector< TaxonData<charType> > taxa = std::vector< TaxonData< charType > >( tau->getValue().getNumberOfNodes(), TaxonData<charType>() );

    // simulate the root sequence
    RandomNumberGenerator* rng = GLOBAL_RNG;
    const std::vector< double > &stationaryFreqs = getRootFrequencies();
    TaxonData< charType > &root = taxa[ tau->getValue().getRoot().getIndex() ];
    for ( size_t i = 0; i < numSites; ++i ) 
    {
        // create the character
        charType *c = new charType();
        // draw the state
        size_t state = 0;
        double u = rng->uniform01();
        while ( u > 0 ) 
        {
            u -= stationaryFreqs[state];
            ++state;
        }
        
        // convert into a char
        char val = (0x1 << state);
        
        // set the state
        c->setState( val );
        
        // add the character to the sequence
        root.addCharacter( c );
    }
    
    // recursively simulate the sequences
    simulate( tau->getValue().getRoot(), taxa );
    
    // add the taxon data to the character data
    for (size_t i = 0; i < tau->getValue().getNumberOfTips(); ++i) 
    {
        this->value->addTaxonData( taxa[i] );
    }
    
    // compress the data and initialize internal variables
    this->compress();
    
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::restoreSpecialization( DagNode* affecter ) {
    
    // reset the flags
    for (std::vector<bool>::iterator it = dirtyNodes.begin(); it != dirtyNodes.end(); ++it) 
    {
        (*it) = false;
    }
    
    // restore the active likelihoods vector
    for (size_t index = 0; index < changedNodes.size(); ++index) 
    {
        // we have to restore, that means if we have changed the active likelihood vector
        // then we need to revert this change
        if ( changedNodes[index] ) 
        {
            activeLikelihood[index] = (activeLikelihood[index] == 0 ? 1 : 0);
        }
        
        // set all flags to false
        changedNodes[index] = false;
    }
    
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::setValue(AbstractCharacterData *v) {
    
    // delegate to the parent class
    TypedDistribution< AbstractCharacterData >::setValue(v);
    
    compress();
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::simulate( const TopologyNode &node, std::vector< TaxonData< charType > > &taxa) {
        
    // get the children of the node
    const std::vector<TopologyNode*>& children = node.getChildren();
    
    // get the sequence of this node
    size_t nodeIndex = node.getIndex();
    TaxonData< charType > &parent = taxa[ nodeIndex ];
    
    // simulate the sequence for each child
    RandomNumberGenerator* rng = GLOBAL_RNG;
    for (std::vector< TopologyNode* >::const_iterator it = children.begin(); it != children.end(); ++it) 
    {
        const TopologyNode &child = *(*it);
        
        // update the transition probability matrix
        updateTransitionProbabilities( child.getIndex(), child.getBranchLength() );
        
        TaxonData< charType > &taxon = taxa[ child.getIndex() ];
        for ( size_t i = 0; i < numSites; ++i ) 
        {
            // get the ancestral character for this site
            unsigned int parentState = parent.getCharacter( i ).getState();
            size_t p = 0;
            while ( parentState != 1 ) 
            {
                // shift to the next state
                parentState >>= 1;
                // increase the index
                ++p;
            }
            
            double *freqs = transitionProbMatrix[ p ];
            // create the character
            charType *c = new charType();
            // draw the state
            size_t state = 0;
            double u = rng->uniform01();
            while ( u > 0 ) 
            {
                u -= freqs[state];
                ++state;
            }
            
            // convert into a char
            char val = (0x1 << state);
            
            // set the state
            c->setState( val );
            
            // add the character to the sequence
            taxon.addCharacter( c );
        }
        
        if ( child.isTip() ) 
        {
            taxon.setTaxonName( child.getName() );
        }
        else 
        {
            // recursively simulate the sequences
            simulate( child, taxa );
        }
        
    }
    
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::swapParameter(const DagNode *oldP, const DagNode *newP) {
    
    // we only have the topology here as the parameter
    if (oldP == tau) 
    {
        tau = static_cast<const TypedDagNode<treeType>* >( newP );
        tau->getValue().getTreeChangeEventHandler().addListener( this );
    }
    
}


template<class charType, class treeType>
void RevBayesCore::AbstractSiteHomogeneousCharEvoModel<charType, treeType>::touchSpecialization( DagNode* affecter ) {
    
    // if the topology wasn't the culprit for the touch, then we just flag everything as dirty
    if ( affecter != tau ) 
    {
        for (std::vector<bool>::iterator it = dirtyNodes.begin(); it != dirtyNodes.end(); ++it) 
        {
            (*it) = true;
        }
        
        // flip the active likelihood pointers
        for (size_t index = 0; index < changedNodes.size(); ++index) 
        {
            if ( !changedNodes[index] ) 
            {
                activeLikelihood[index] = (activeLikelihood[index] == 0 ? 1 : 0);
                changedNodes[index] = true;
            }
        }
    }
    
}


#endif
