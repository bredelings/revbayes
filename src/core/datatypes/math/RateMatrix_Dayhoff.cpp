/**
 * @file
 * This file contains the implementation of RateMatrix_Dayhoff, which is
 * class that holds a rate matrix in RevBayes.
 *
 * @brief Implementation of RateMatrix_Dayhoff
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date: 2012-12-11 14:46:24 +0100 (Tue, 11 Dec 2012) $
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 * @version 1.0
 * @since 2009-08-27, version 1.0
 * @interface Mcmc
 * @package distributions
 *
 * $Id: RateMatrix_Dayhoff.cpp 1921 2012-12-11 13:46:24Z hoehna $
 */

#include "RateMatrix_Dayhoff.h"
#include "RbException.h"
#include "RbMathMatrix.h"
#include "TransitionProbabilityMatrix.h"


using namespace RevBayesCore;

/** Construct rate matrix with n states */
RateMatrix_Dayhoff::RateMatrix_Dayhoff( void ) : RateMatrix_Empirical( 20 ){
    
    MatrixReal &m = *theRateMatrix;
    
    /* Dayhoff */
	m[ 0][ 0] =   0; m[ 0][ 1] =  27; m[ 0][ 2] =  98; m[ 0][ 3] = 120; m[ 0][ 4] =  36; 
	m[ 0][ 5] =  89; m[ 0][ 6] = 198; m[ 0][ 7] = 240; m[ 0][ 8] =  23; m[ 0][ 9] =  65; 
	m[ 0][10] =  41; m[ 0][11] =  26; m[ 0][12] =  72; m[ 0][13] =  18; m[ 0][14] = 250; 
	m[ 0][15] = 409; m[ 0][16] = 371; m[ 0][17] =   0; m[ 0][18] =  24; m[ 0][19] = 208; 
	m[ 1][ 0] =  27; m[ 1][ 1] =   0; m[ 1][ 2] =  32; m[ 1][ 3] =   0; m[ 1][ 4] =  23; 
	m[ 1][ 5] = 246; m[ 1][ 6] =   1; m[ 1][ 7] =   9; m[ 1][ 8] = 240; m[ 1][ 9] =  64; 
	m[ 1][10] =  15; m[ 1][11] = 464; m[ 1][12] =  90; m[ 1][13] =  14; m[ 1][14] = 103; 
	m[ 1][15] = 154; m[ 1][16] =  26; m[ 1][17] = 201; m[ 1][18] =   8; m[ 1][19] =  24; 
	m[ 2][ 0] =  98; m[ 2][ 1] =  32; m[ 2][ 2] =   0; m[ 2][ 3] = 905; m[ 2][ 4] =   0; 
	m[ 2][ 5] = 103; m[ 2][ 6] = 148; m[ 2][ 7] = 139; m[ 2][ 8] = 535; m[ 2][ 9] =  77; 
	m[ 2][10] =  34; m[ 2][11] = 318; m[ 2][12] =   1; m[ 2][13] =  14; m[ 2][14] =  42; 
	m[ 2][15] = 495; m[ 2][16] = 229; m[ 2][17] =  23; m[ 2][18] =  95; m[ 2][19] =  15; 
	m[ 3][ 0] = 120; m[ 3][ 1] =   0; m[ 3][ 2] = 905; m[ 3][ 3] =   0; m[ 3][ 4] =   0; 
	m[ 3][ 5] = 134; m[ 3][ 6] = 1153; m[ 3][ 7] = 125; m[ 3][ 8] =  86; m[ 3][ 9] =  24; 
	m[ 3][10] =   0; m[ 3][11] =  71; m[ 3][12] =   0; m[ 3][13] =   0; m[ 3][14] =  13; 
	m[ 3][15] =  95; m[ 3][16] =  66; m[ 3][17] =   0; m[ 3][18] =   0; m[ 3][19] =  18; 
	m[ 4][ 0] =  36; m[ 4][ 1] =  23; m[ 4][ 2] =   0; m[ 4][ 3] =   0; m[ 4][ 4] =   0; 
	m[ 4][ 5] =   0; m[ 4][ 6] =   0; m[ 4][ 7] =  11; m[ 4][ 8] =  28; m[ 4][ 9] =  44; 
	m[ 4][10] =   0; m[ 4][11] =   0; m[ 4][12] =   0; m[ 4][13] =   0; m[ 4][14] =  19; 
	m[ 4][15] = 161; m[ 4][16] =  16; m[ 4][17] =   0; m[ 4][18] =  96; m[ 4][19] =  49; 
	m[ 5][ 0] =  89; m[ 5][ 1] = 246; m[ 5][ 2] = 103; m[ 5][ 3] = 134; m[ 5][ 4] =   0; 
	m[ 5][ 5] =   0; m[ 5][ 6] = 716; m[ 5][ 7] =  28; m[ 5][ 8] = 606; m[ 5][ 9] =  18; 
	m[ 5][10] =  73; m[ 5][11] = 153; m[ 5][12] = 114; m[ 5][13] =   0; m[ 5][14] = 153; 
	m[ 5][15] =  56; m[ 5][16] =  53; m[ 5][17] =   0; m[ 5][18] =   0; m[ 5][19] =  35; 
	m[ 6][ 0] = 198; m[ 6][ 1] =   1; m[ 6][ 2] = 148; m[ 6][ 3] = 1153; m[ 6][ 4] =   0; 
	m[ 6][ 5] = 716; m[ 6][ 6] =   0; m[ 6][ 7] =  81; m[ 6][ 8] =  43; m[ 6][ 9] =  61; 
	m[ 6][10] =  11; m[ 6][11] =  83; m[ 6][12] =  30; m[ 6][13] =   0; m[ 6][14] =  51; 
	m[ 6][15] =  79; m[ 6][16] =  34; m[ 6][17] =   0; m[ 6][18] =  22; m[ 6][19] =  37; 
	m[ 7][ 0] = 240; m[ 7][ 1] =   9; m[ 7][ 2] = 139; m[ 7][ 3] = 125; m[ 7][ 4] =  11; 
	m[ 7][ 5] =  28; m[ 7][ 6] =  81; m[ 7][ 7] =   0; m[ 7][ 8] =  10; m[ 7][ 9] =   0; 
	m[ 7][10] =   7; m[ 7][11] =  27; m[ 7][12] =  17; m[ 7][13] =  15; m[ 7][14] =  34; 
	m[ 7][15] = 234; m[ 7][16] =  30; m[ 7][17] =   0; m[ 7][18] =   0; m[ 7][19] =  54; 
	m[ 8][ 0] =  23; m[ 8][ 1] = 240; m[ 8][ 2] = 535; m[ 8][ 3] =  86; m[ 8][ 4] =  28; 
	m[ 8][ 5] = 606; m[ 8][ 6] =  43; m[ 8][ 7] =  10; m[ 8][ 8] =   0; m[ 8][ 9] =   7; 
	m[ 8][10] =  44; m[ 8][11] =  26; m[ 8][12] =   0; m[ 8][13] =  48; m[ 8][14] =  94; 
	m[ 8][15] =  35; m[ 8][16] =  22; m[ 8][17] =  27; m[ 8][18] = 127; m[ 8][19] =  44; 
	m[ 9][ 0] =  65; m[ 9][ 1] =  64; m[ 9][ 2] =  77; m[ 9][ 3] =  24; m[ 9][ 4] =  44; 
	m[ 9][ 5] =  18; m[ 9][ 6] =  61; m[ 9][ 7] =   0; m[ 9][ 8] =   7; m[ 9][ 9] =   0; 
	m[ 9][10] = 257; m[ 9][11] =  46; m[ 9][12] = 336; m[ 9][13] = 196; m[ 9][14] =  12; 
	m[ 9][15] =  24; m[ 9][16] = 192; m[ 9][17] =   0; m[ 9][18] =  37; m[ 9][19] = 889; 
	m[10][ 0] =  41; m[10][ 1] =  15; m[10][ 2] =  34; m[10][ 3] =   0; m[10][ 4] =   0; 
	m[10][ 5] =  73; m[10][ 6] =  11; m[10][ 7] =   7; m[10][ 8] =  44; m[10][ 9] = 257; 
	m[10][10] =   0; m[10][11] =  18; m[10][12] = 527; m[10][13] = 157; m[10][14] =  32; 
	m[10][15] =  17; m[10][16] =  33; m[10][17] =  46; m[10][18] =  28; m[10][19] = 175; 
	m[11][ 0] =  26; m[11][ 1] = 464; m[11][ 2] = 318; m[11][ 3] =  71; m[11][ 4] =   0; 
	m[11][ 5] = 153; m[11][ 6] =  83; m[11][ 7] =  27; m[11][ 8] =  26; m[11][ 9] =  46; 
	m[11][10] =  18; m[11][11] =   0; m[11][12] = 243; m[11][13] =   0; m[11][14] =  33; 
	m[11][15] =  96; m[11][16] = 136; m[11][17] =   0; m[11][18] =  13; m[11][19] =  10; 
	m[12][ 0] =  72; m[12][ 1] =  90; m[12][ 2] =   1; m[12][ 3] =   0; m[12][ 4] =   0; 
	m[12][ 5] = 114; m[12][ 6] =  30; m[12][ 7] =  17; m[12][ 8] =   0; m[12][ 9] = 336; 
	m[12][10] = 527; m[12][11] = 243; m[12][12] =   0; m[12][13] =  92; m[12][14] =  17; 
	m[12][15] =  62; m[12][16] = 104; m[12][17] =   0; m[12][18] =   0; m[12][19] = 258; 
	m[13][ 0] =  18; m[13][ 1] =  14; m[13][ 2] =  14; m[13][ 3] =   0; m[13][ 4] =   0; 
	m[13][ 5] =   0; m[13][ 6] =   0; m[13][ 7] =  15; m[13][ 8] =  48; m[13][ 9] = 196; 
	m[13][10] = 157; m[13][11] =   0; m[13][12] =  92; m[13][13] =   0; m[13][14] =  11; 
	m[13][15] =  46; m[13][16] =  13; m[13][17] =  76; m[13][18] = 698; m[13][19] =  12; 
	m[14][ 0] = 250; m[14][ 1] = 103; m[14][ 2] =  42; m[14][ 3] =  13; m[14][ 4] =  19; 
	m[14][ 5] = 153; m[14][ 6] =  51; m[14][ 7] =  34; m[14][ 8] =  94; m[14][ 9] =  12; 
	m[14][10] =  32; m[14][11] =  33; m[14][12] =  17; m[14][13] =  11; m[14][14] =   0; 
	m[14][15] = 245; m[14][16] =  78; m[14][17] =   0; m[14][18] =   0; m[14][19] =  48; 
	m[15][ 0] = 409; m[15][ 1] = 154; m[15][ 2] = 495; m[15][ 3] =  95; m[15][ 4] = 161; 
	m[15][ 5] =  56; m[15][ 6] =  79; m[15][ 7] = 234; m[15][ 8] =  35; m[15][ 9] =  24; 
	m[15][10] =  17; m[15][11] =  96; m[15][12] =  62; m[15][13] =  46; m[15][14] = 245; 
	m[15][15] =   0; m[15][16] = 550; m[15][17] =  75; m[15][18] =  34; m[15][19] =  30; 
	m[16][ 0] = 371; m[16][ 1] =  26; m[16][ 2] = 229; m[16][ 3] =  66; m[16][ 4] =  16; 
	m[16][ 5] =  53; m[16][ 6] =  34; m[16][ 7] =  30; m[16][ 8] =  22; m[16][ 9] = 192; 
	m[16][10] =  33; m[16][11] = 136; m[16][12] = 104; m[16][13] =  13; m[16][14] =  78; 
	m[16][15] = 550; m[16][16] =   0; m[16][17] =   0; m[16][18] =  42; m[16][19] = 157; 
	m[17][ 0] =   0; m[17][ 1] = 201; m[17][ 2] =  23; m[17][ 3] =   0; m[17][ 4] =   0; 
	m[17][ 5] =   0; m[17][ 6] =   0; m[17][ 7] =   0; m[17][ 8] =  27; m[17][ 9] =   0; 
	m[17][10] =  46; m[17][11] =   0; m[17][12] =   0; m[17][13] =  76; m[17][14] =   0; 
	m[17][15] =  75; m[17][16] =   0; m[17][17] =   0; m[17][18] =  61; m[17][19] =   0; 
	m[18][ 0] =  24; m[18][ 1] =   8; m[18][ 2] =  95; m[18][ 3] =   0; m[18][ 4] =  96; 
	m[18][ 5] =   0; m[18][ 6] =  22; m[18][ 7] =   0; m[18][ 8] = 127; m[18][ 9] =  37; 
	m[18][10] =  28; m[18][11] =  13; m[18][12] =   0; m[18][13] = 698; m[18][14] =   0; 
	m[18][15] =  34; m[18][16] =  42; m[18][17] =  61; m[18][18] =   0; m[18][19] =  28; 
	m[19][ 0] = 208; m[19][ 1] =  24; m[19][ 2] =  15; m[19][ 3] =  18; m[19][ 4] =  49; 
	m[19][ 5] =  35; m[19][ 6] =  37; m[19][ 7] =  54; m[19][ 8] =  44; m[19][ 9] = 889; 
	m[19][10] = 175; m[19][11] =  10; m[19][12] = 258; m[19][13] =  12; m[19][14] =  48; 
	m[19][15] =  30; m[19][16] = 157; m[19][17] =   0; m[19][18] =  28; m[19][19] =   0;
    
	stationaryFreqs[ 0] = 0.087127;
	stationaryFreqs[ 1] = 0.040904;
	stationaryFreqs[ 2] = 0.040432;
	stationaryFreqs[ 3] = 0.046872;
	stationaryFreqs[ 4] = 0.033474;
	stationaryFreqs[ 5] = 0.038255;
	stationaryFreqs[ 6] = 0.049530;
	stationaryFreqs[ 7] = 0.088612;
	stationaryFreqs[ 8] = 0.033618;
	stationaryFreqs[ 9] = 0.036886;
	stationaryFreqs[10] = 0.085357;
	stationaryFreqs[11] = 0.080482;
	stationaryFreqs[12] = 0.014753;
	stationaryFreqs[13] = 0.039772;
	stationaryFreqs[14] = 0.050680;
	stationaryFreqs[15] = 0.069577;
	stationaryFreqs[16] = 0.058542;
	stationaryFreqs[17] = 0.010494;
	stationaryFreqs[18] = 0.029916;
	stationaryFreqs[19] = 0.064718;
    
    
    // set the diagonal values
    setDiagonal();
    
    // rescale 
    rescaleToAverageRate( 1.0 );
    
    // update the eigensystem
    updateEigenSystem();
}


/** Copy constructor */
RateMatrix_Dayhoff::RateMatrix_Dayhoff(const RateMatrix_Dayhoff& m) : RateMatrix_Empirical( m ) {
    
}


/** Destructor */
RateMatrix_Dayhoff::~RateMatrix_Dayhoff(void) {
    
}




RateMatrix_Dayhoff* RateMatrix_Dayhoff::clone( void ) const {
    return new RateMatrix_Dayhoff( *this );
}


