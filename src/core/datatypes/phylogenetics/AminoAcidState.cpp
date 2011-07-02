/**
 * @file
 * This file contains the implementation of AminoAcidState, which is
 * the class for the Amino Acid character data type in RevBayes.
 *
 * @brief Implementation of AminoAcidState
 *
 * (c) Copyright 2009-
 * @date Last modified: $Date$
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 *
 * $Id$
 */

#include "AminoAcidState.h"
#include "RbNames.h"
#include "VectorString.h"
#include <sstream>

const std::string AminoAcidState::stateLabels = "ARNDCQEGHILKMFPSTWYV";




/** Default constructor */
AminoAcidState::AminoAcidState(void) : CharacterStateDiscrete(20) {

    setState('n');
}


/** Copy constructor */
AminoAcidState::AminoAcidState(const AminoAcidState& s) : CharacterStateDiscrete(20) {

    value = s.value;
}


/** Constructor that sets the observation */
AminoAcidState::AminoAcidState(const char s) : CharacterStateDiscrete(20) {

    setState(s);
}


/** Equals comparison */
bool AminoAcidState::operator==(const Character& x) const {

    const AminoAcidState* derivedX = static_cast<const AminoAcidState*>(&x);
    for (size_t i=0; i<numStates; i++) 
        {
        if ( value[i] != derivedX->value[i] )
            return false;
        }
    return true;
}


/** Not equals comparison */
bool AminoAcidState::operator!=(const Character& x) const {

    return !operator==(x);
}


/** Set the observation */
void AminoAcidState::addState(const char s) {
        
    // look for matches against the state label static vector
    char c = toupper(s);
    size_t numMatches = 0;
    for (size_t i=0; i<numStates; i++)
        {
        if ( c == stateLabels[i] )
            {
            value[i] = true;
            numMatches++;
            }
        }
        
    // if there are no matches, we assume that the character state was something like
    // a '?', 'n', or '-' and set all of the flags to true, to indicate complete ambiguity
    if (numMatches == 0)
        {
        for (size_t i=0; i<numStates; i++)
            value[i] = true;
        }
}


/** Clone object */
AminoAcidState* AminoAcidState::clone(void) const {

	return  new AminoAcidState( *this );
}


/** Get class vector describing type of object */
const VectorString& AminoAcidState::getClass(void) const {

    static VectorString rbClass = VectorString( AminoAcidState_name ) + RbObject::getClass();
    return rbClass;
}


/** Get value */
const char AminoAcidState::getState(void) const {

    char c;
    size_t numMatches = 0;
    for (size_t i=0; i<numStates; i++)
        {
        if ( value[i] == true )
            {
            c = stateLabels[i];
            numMatches++;
            }
        }
    if (numMatches > 1)
        c = '?';
    return c;
}


/** Print information for the user */
void AminoAcidState::printValue(std::ostream &o) const {

    o << getState();
}


/** Get complete info about object */
std::string AminoAcidState::richInfo( void ) const {

	std::ostringstream o;
    printValue( o );
    return o.str();
}


void AminoAcidState::setState(const char s) {

    // wipe the value clean, setting all bool flags to false
    for (size_t i=0; i<numStates; i++)
        value[i] = false;
    addState(s);
}