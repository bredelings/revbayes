#ifndef DiscreteEventBirthDeathProposal_H
#define DiscreteEventBirthDeathProposal_H

#include <set>
#include <string>

#include "EventBirthDeathProposal.h"
#include "Proposal.h"
#include "StochasticNode.h"

namespace RevBayesCore {
    
    class CharacterEvent;
    
    /**
     * The birth-death proposal for events along a tree.
     *
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team
     * @since 2009-09-08, version 1.0
     *
     */
    class DiscreteEventBirthDeathProposal : public EventBirthDeathProposal {
        
    public:
        DiscreteEventBirthDeathProposal( StochasticNode<Tree> *n);                                                                //!<  constructor
        
        // Basic utility functions
        DiscreteEventBirthDeathProposal*        clone(void) const;                                                                  //!< Clone object
        const std::string&                      getProposalName(void) const;                                                        //!< Get the name of the proposal for summary printing
        
    protected:
        
        // pure virtual methods
        CharacterEvent*                         drawNewEvent(double event_time);
        double                                  computeEventProposalProbability( CharacterEvent* event );
        
    };
    
}

#endif

