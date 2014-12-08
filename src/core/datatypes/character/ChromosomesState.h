/**
 * @file
 * This file contains the declaration of ChromosomesState, which is
 * the base class for the chromosome character data type in RevBayes.
 *
 * @brief Declaration of ChromosomesState
 *
 * (c) Copyright 2014-
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 *
 */


#ifndef ChromosomesState_H
#define ChromosomesState_H

#include "DiscreteCharacterState.h"
#include <ostream>
#include <set>

namespace RevBayesCore {
    
    class ChromosomesState : public DiscreteCharacterState {
        
    public:
        ChromosomesState(void);                                     //!< Default constructor
        ChromosomesState(const ChromosomesState& s);                //!< Copy constructor
        ChromosomesState(std::string s);                            //!< Constructor with an observation
        
        bool                            operator==(const CharacterState& x) const;          //!< Equality
        bool                            operator!=(const CharacterState& x) const;          //!< Inequality
        bool                            operator<(const CharacterState& d) const;           //!< Less than
        void                            operator++();                                       //!< Increment
        void                            operator++(int i);                                  //!< Increment
        void                            operator--();                                       //!< Decrement
        void                            operator--(int i);                                  //!< Decrement
        
        ChromosomesState*               clone(void) const;									//!< Get a copy of this object
        
        // Discrete character observation functions
        void                            addState(std::string symbol);                       //!< Add a character state to the set of character states
        void                            addState(char symbol);                              //!< Add a character state to the set of character states
        std::string                     getDatatype(void) const;                            //!< Get the datatype as a common string.
        unsigned int                    getNumberObservedStates(void) const;                //!< How many states are observed for the character
        const std::string&              getStateLabels(void) const;                         //!< Get valid state labels
        std::string                     getStringValue(void) const;                         //!< Get a representation of the character as a string
        size_t                          getNumberOfStates(void) const;                      //!< Get the number of discrete states for the character
        unsigned long                   getState(void) const;                               //!< Get the discrete observation
        size_t                          getStateIndex(void) const;
        bool                            isAmbiguous(void) const;                            //!< Is the character missing or ambiguous
        bool                            isGapState(void) const;                             //!< Get whether this is a gapped character state
        void                            setState(std::string symbol);                       //!< Set the discrete observation
        void                            setState(char symbol);                              //!< Set the discrete observation
        void                            setState(size_t stateIndex);                        //!< Set the discrete observation
        void                            setState(size_t pos, bool val);                     //!< Set the discrete observation
        void                            setGapState(bool tf);                               //!< Set whether this is a gapped character
        void                            setToFirstState(void);                              //!< Set this character state to the first (lowest) possible state
        
        
    private:
        unsigned int                    computeState(std::string symbol) const;             //!< Compute the internal state value for this character.
        
        unsigned int                    state;
        //size_t                          stateIndex;
        
    };
	extern size_t						g_MAX_NUM_CHROMOSOMES;
}

#endif
