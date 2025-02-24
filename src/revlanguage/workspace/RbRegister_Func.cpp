/**
 * @file
 * This file contains the Workspace function that adds types and functions
 * to the global workspace, registering them with the interpreter/compiler
 * during the process.
 *
 * @brief Function registering language objects
 *
 * Instructions
 *
 * This is the central registry of Rev objects. It is a large file and needs
 * to be properly organized to facilitate maintenance. Follow these simple
 * guidelines to ensure that your additions follow the existing structure.
 *
 * 1. All headers are added in groups corresponding to directories in the
 *    revlanguage code base.
 * 2. All objects (types, distributions, and functions) are registered in
 *    groups corresponding to directories in the revlanguage code base.
 * 3. All entries in each group are listed in alphabetical order.
 *
 * Some explanation of the directory structure is provided in the comments
 * in this file. Consult these comments if you are uncertain about where
 * to add your objects in the code.
 */

#include <sstream>
#include <vector>
#include <set>
#include <cstdlib>

/* Files including helper classes */
#include "AddContinuousDistribution.h"
#include "AddDistribution.h"
#include "AddWorkspaceVectorType.h"
#include "AddVectorizedWorkspaceType.h"
#include "RbException.h"
#include "RlUserInterface.h"
#include "Workspace.h"

/// Miscellaneous types ///

/* Base types (in folder "datatypes") */
#include "RevObject.h"

/* Primitive types (in folder "datatypes/basic") */
#include "Integer.h"
#include "Natural.h"
#include "Probability.h"
#include "RlBoolean.h"
#include "RlString.h"
#include "Real.h"
#include "RealPos.h"

/* Container types (in folder "datatypes/container") */
#include "ModelVector.h"
#include "WorkspaceVector.h"


/* Taxon types (in folder "datatypes/evolution") */
#include "RlTaxon.h"


/* Math types (in folder "datatypes/math") */
#include "RlMatrixReal.h"
#include "RlMatrixRealSymmetric.h"
#include "RlRateGeneratorSequence.h"
#include "RlRateMatrix.h"
#include "RlSimplex.h"


/* Argument rules (in folder "functions/argumentrules") */
#include "ArgumentRule.h"


/* Basic functions (in folder "functions/basic"). */

/* These are core functions for the Rev environment, providing user help
   and other essential services. */


/* Functions related to evolution (in folder "functions/phylogenetics") */
#include "Func_branchScoreDistance.h"
#include "Func_checkNodeOrderConstraints.h"
#include "Func_computeWeightedNodeOrderConstraintsScore.h"
#include "Func_concatenate.h"
#include "Func_CladeSpecificHierarchicalBranchRate.h"
#include "Func_concatenateFromVector.h"
#include "Func_constructRootedTripletDistribution.h"
#include "Func_extantTree.h"
#include "Func_formatDiscreteCharacterData.h"
#include "Func_maximumTree.h"
#include "Func_mrcaIndex.h"
#include "Func_nodeAgeByID.h"
#include "Func_phyloDiversity.h"
#include "Func_PhylogeneticIndependentContrasts.h"
#include "Func_PhylogeneticIndependentContrastsMultiSample.h"
#include "Func_pomoStateConverter.h"
#include "Func_pomoRootFrequencies.h"
#include "Func_readPomoCountFile.h"
#include "Func_pruneTree.h"
#include "Func_simTree.h"
#include "Func_simCompleteTree.h"
#include "Func_stitchTree.h"
#include "Func_symmetricDifference.h"
#include "Func_tmrca.h"
#include "Func_treeAssembly.h"
#include "Func_treePairwiseDistances.h"
#include "Func_treeScale.h"


/* Rate matrix functions (in folder "functions/phylogenetics/ratematrix") */
#include "Func_blosum62.h"
#include "Func_chromosomes.h"
#include "Func_chromosomesPloidy.h"
#include "Func_codonSynonymousNonsynonymousRateMatrix.h"
#include "Func_codonSynonymousNonsynonymousHKYRateMatrix.h"
#include "Func_covarionRateMatrix.h"
#include "Func_covarion.h"
#include "Func_cpRev.h"
#include "Func_dayhoff.h"
#include "Func_DECRateMatrix.h"
#include "Func_epoch.h"
#include "Func_f81.h"
#include "Func_FreeBinary.h"
#include "Func_FreeK.h"
#include "Func_freeSymmetricRateMatrix.h"
#include "Func_gtr.h"
#include "Func_hky.h"
#include "Func_hiddenStateRateMatrix.h"
#include "Func_InfiniteSitesRateMatrix.h"
#include "Func_jc.h"
#include "Func_jones.h"
#include "Func_k80.h"
#include "Func_Kimura81.h"
#include "Func_lg.h"
#include "Func_mtRev.h"
#include "Func_mtMam.h"
#include "Func_orderedRateMatrix.h"
#include "Func_pomo.h"
#include "Func_reversiblePomo.h"
#include "Func_rtRev.h"
#include "Func_vt.h"
#include "Func_t92.h"
#include "Func_TamuraNei.h"
#include "Func_TIM.h"
#include "Func_TVM.h"
#include "Func_wag.h"

/* Functions related to evolution (in folder "functions/popgen") */
#include "Func_PattersonsD.h"
#include "Func_SegregatingSites.h"
#include "Func_TajimasD.h"
#include "Func_TajimasPi.h"
#include "Func_WattersonTheta.h"


/* Rate map functions (in folder "functions/evolution/ratemap") */
#include "Func_adjacentRateModifier.h"
#include "Func_biogeo_de.h"
#include "Func_distanceRateModifier.h"
#include "Func_generalRateGeneratorSequence.h"
#include "Func_rangeEvolutionRateModifier.h"
#include "Func_siteRateModifier.h"
#include "Func_phylogeneticDistanceRateModifier.h"


/* Cladogeneic state prob function */
#include "Func_DECCladoProbs.h"
#include "Func_DECRates.h"
#include "Func_DECRoot.h"
#include "Func_EpochCladoProbs.h"
#include "Func_chromosomesCladoProbs.h"
#include "Func_chromosomesCladoEventsBD.h"
#include "Func_chromosomesPloidyCladoEventsBD.h"
#include "Func_cladogeneticSpeciationRateMatrix.h"
#include "Func_MixtureCladoProbs.h"
#include "Func_SampledCladogenesisRootFrequencies.h"


/* Input/output functions (in folder "functions/io") */
#include "Func_ancestralStateTree.h"
#include "Func_consensusTree.h"
#include "Func_convertToPhylowood.h"
#include "Func_formatDiscreteCharacterData.h"
#include "Func_module.h"
#include "Func_readAtlas.h"
#include "Func_readCharacterDataDelimited.h"
#include "Func_readContinuousCharacterData.h"
#include "Func_readDiscreteCharacterData.h"
#include "Func_readDistanceMatrix.h"
#include "Func_readStochasticVariableTrace.h"
#include "Func_readTrace.h"
#include "Func_readTrees.h"
#include "Func_readBranchLengthTrees.h"
#include "Func_readTreeTrace.h"
#include "Func_readAncestralStateTreeTrace.h"
#include "Func_readAncestralStateTrace.h"
#include "Func_source.h"
#include "Func_summarizeCharacterMaps.h"
#include "Func_TaxonReader.h"
#include "Func_treeTrace.h"
#include "Func_write.h"
#include "Func_writeCharacterDataDelimited.h"
#include "Func_writeFasta.h"
#include "Func_writeNexus.h"


/* Math functions (in folder "functions/math") */
#include "Func_abs.h"
#include "Func_absVector.h"
#include "Func_ceil.h"
#include "Func_choose.h"
#include "Func_coala.h"
#include "Func_diagonalMatrix.h"
#include "Func_empiricalQuantile.h"
#include "Func_exp.h"
#include "Func_floor.h"
#include "Func_gamma.h"
#include "Func_lnProbability.h"
#include "Func_geographicalDistance.h"
#include "Func_hyperbolicTangent.h"
#include "Func_hyperbolicSine.h"
#include "Func_ln.h"
#include "Func_log.h"
#include "Func_logistic.h"
#include "Func_matrix.h"
#include "Func_max.h"
#include "Func_mean.h"
#include "Func_meanPositive.h"
#include "Func_median.h"
#include "Func_min.h"
#include "Func_normalize.h"
#include "Func_posteriorPredictiveProbability.h"
#include "Func_power.h"
#include "Func_powerVector.h"
#include "Func_probability.h"
#include "Func_round.h"
#include "Func_shortestDistance.h"
#include "Func_simplex.h"
#include "Func_simplexFromVector.h"
#include "Func_sort.h"
#include "Func_sum.h"
#include "Func_sumPositive.h"
#include "Func_sumInteger.h"
#include "Func_sumNatural.h"
#include "Func_standardDeviation.h"
#include "Func_sqrt.h"
#include "Func_trunc.h"
#include "Func_upperTriangle.h"
#include "Func_variance.h"
#include "Func_vectorFlatten.h"


/* Statistics functions (in folder "functions/statistics") */
/* These are functions related to statistical distributions */
#include "Func_discretizeBeta.h"
#include "Func_discretizeBetaQuadrature.h"
#include "Func_discretizeGamma.h"
#include "Func_discretizeGammaQuadrature.h"
#include "Func_discretizeLognormalQuadrature.h"
#include "Func_discretizeDistribution.h"
#include "Func_discretizePositiveDistribution.h"
#include "Func_dppConcFromMean.h"
#include "Func_dppMeanFromConc.h"
#include "Func_fnNormalizedQuantile.h"
#include "Func_numUniqueInVector.h"
#include "Func_stirling.h"
#include "Func_varianceCovarianceMatrix.h"
#include "Func_decomposedVarianceCovarianceMatrix.h"
#include "Func_partialToCorrelationMatrix.h"

#include "RlDiscreteCharacterState.h"


/** Initialize global workspace */
void RevLanguage::Workspace::initializeFuncGlobalWorkspace(void)
{

    try
    {
        ///////////////////////////////////////////
        /* Add functions (in "functions" folder) */
        ///////////////////////////////////////////

        /* Rate matrix generator functions (in folder "functions/evolution/ratematrix") */
        addFunction( new Func_blosum62()                                    );
        addFunction( new Func_chromosomes()                                 );
        addFunction( new Func_chromosomesPloidy()                           );
        addFunction( new Func_codonSynonymousNonsynonymousRateMatrix()      );
        addFunction( new Func_codonSynonymousNonsynonymousHKYRateMatrix()   );
        addFunction( new Func_covarionRateMatrix()                          );
        addFunction( new Func_covarion()                                    );
        addFunction( new Func_cpRev()                                       );
        addFunction( new Func_dayhoff()                                     );
        addFunction( new Func_DECRateMatrix()                               );
        addFunction( new Func_epoch()                                       );
        addFunction( new Func_f81()                                         );
        addFunction( new Func_FreeBinary()                                  );
        addFunction( new Func_FreeK()                                       );
        addFunction( new Func_freeSymmetricRateMatrix()                     );
        addFunction( new Func_gtr()                                         );
        addFunction( new Func_hky()                                         );
        addFunction( new Func_hiddenStateRateMatrix()                       );
        addFunction( new Func_InfiniteSitesRateMatrix()                     );
        addFunction( new Func_jc()                                          );
        addFunction( new Func_jones()                                       );
        addFunction( new Func_k80()                                         );
        addFunction( new Func_Kimura81()                                    );
        addFunction( new Func_lg()                                          );
        addFunction( new Func_mtMam()                                       );
        addFunction( new Func_mtRev()                                       );
        addFunction( new Func_orderedRateMatrix()                           );
        addFunction( new Func_pomo()                                        );
        addFunction( new Func_reversiblePomo()                              );
        addFunction( new Func_rtRev()                                       );
        addFunction( new Func_t92()                                         );
        addFunction( new Func_TamuraNei()                                   );
        addFunction( new Func_TIM()                                         );
        addFunction( new Func_TVM()                                         );
        addFunction( new Func_vt()                                          );
        addFunction( new Func_wag()                                         );

        /* rate maps used for data augmentation (in folder "functions/evolution/ratemap") */
        addFunction( new Func_adjacentRateModifier() );
        addFunction( new Func_biogeo_de() );
        addFunction( new Func_distanceRateModifier() );
        addFunction( new Func_generalRateGeneratorSequence() );
        addFunction( new Func_phylogeneticDistanceRateModifier() );
        addFunction( new Func_rangeEvolutionRateModifier() );
        addFunction( new Func_siteRateModifier() );

        /* cladogenic probs used for e.g. DEC models (in folder "functions/phylogenetics") */
        addFunction( new Func_DECCladoProbs() );
        addFunction( new Func_DECRates() );
        addFunction( new Func_DECRoot() );
        addFunction( new Func_EpochCladoProbs() );
        addFunction( new Func_chromosomesCladoProbs() );
        addFunction( new Func_chromosomesCladoEventsBD() );
        addFunction( new Func_chromosomesPloidyCladoEventsBD() );
        addFunction( new Func_CladeSpecificHierarchicalBranchRate() );
        addFunction( new Func_cladogeneticSpeciationRateMatrix() );
        addFunction( new Func_MixtureCladoProbs() );
        addFunction( new Func_SampledCladogenesisRootFrequencies() );

		/* Functions related to phylogenetic trees (in folder "functions/phylogenetics/tree") */
        addFunction( new Func_branchScoreDistance()                             );
        addFunction( new Func_checkNodeOrderConstraints()                       );
        addFunction( new Func_computeWeightedNodeOrderConstraintsScore()        );
        addFunction( new Func_concatenate()                                     );
        addFunction( new Func_concatenateFromVector()                           );
        addFunction( new Func_constructRootedTripletDistribution()              );
        addFunction( new Func_formatDiscreteCharacterData()                     );
        addFunction( new Func_extantTree()                                      );
        addFunction( new Func_maximumTree()                                     );
        addFunction( new Func_mrcaIndex()                                       );
        addFunction( new Func_nodeAgeByID()                                     );
        addFunction( new Func_phyloDiversity()                                  );
        addFunction( new Func_PhylogeneticIndependentContrasts()                );
        addFunction( new Func_PhylogeneticIndependentContrastsMultiSample()     );
        addFunction( new Func_pomoStateConverter()                              );
        addFunction( new Func_pomoRootFrequencies()                             );
        addFunction( new Func_pruneTree()                                       );
        addFunction( new Func_readPomoCountFile()                               );
        addFunction( new Func_simTree()                                         );
        addFunction( new Func_simCompleteTree()                                 );
        addFunction( new Func_stitchTree()                                      );
        addFunction( new Func_symmetricDifference()                             );
        addFunction( new Func_tmrca()                                           );
        addFunction( new Func_treePairwiseDistances()                           );
        addFunction( new Func_treeAssembly()                                    );
        addFunction( new Func_treeScale()                                       );

        /* Population genetics functions (in folder "functions/popgen") */
        addFunction( new Func_PattersonsD()      );
        addFunction( new Func_SegregatingSites() );
        addFunction( new Func_TajimasD()         );
        addFunction( new Func_TajimasPi()        );
        addFunction( new Func_WattersonTheta()   );


        /* Math functions (in folder "functions/math") */

		// absolute function
        addFunction( new Func_abs()                  );
        addFunction( new Func_absVector()            );

		// ceil function
        addFunction( new Func_ceil<Real,Integer>()  );
        addFunction( new Func_ceil<RealPos,Natural>()  );

        // choose function
        addFunction( new Func_choose() );

        // coala function
        addFunction( new Func_coala()        );

        // diagonal matrix
        addFunction( new Func_diagonalMatrix() );

        // empirical quantile function
        addFunction( new Func_empiricalQuantile()  );

        // exponential function
        addFunction( new Func_exp() );

		// floor function
        addFunction( new Func_floor<Real,Integer>()  );
        addFunction( new Func_floor<RealPos,Natural>()  );

        // gamma function
        addFunction( new Func_gamma() );

        // logistic function
        addFunction( new Func_logistic() );

        // natural log function
        addFunction( new Func_ln()  );

        // log function
        addFunction( new Func_log()  );
        
        // matrix function (converts into MatrixReal)
        addFunction( new Func_matrix() );

        // max function
        addFunction( new Func_max()  );

        // mean function
        addFunction( new Func_mean()  );
        addFunction( new Func_meanPositive()  );
        
        // median function
        addFunction( new Func_median()  );

        // min function
		addFunction( new Func_min()  );

        // normalize vector function
		addFunction( new Func_normalize()  );

        // conversion function from Real to Probability
        addFunction( new Func_probability() );

		// round function
        addFunction( new Func_round<Real,Integer>()  );
        addFunction( new Func_round<RealPos,Natural>()  );

        // simplex constructor function (from RealPos ellipsis argument values)
        addFunction( new Func_simplex()                  );

        // simplex constructor function (from vector of RealPos values)
        addFunction( new Func_simplexFromVector()        );

        // sort vector function
        addFunction( new Func_sort() );

		// square root function
        addFunction( new Func_sqrt()  );

        // sum function
        addFunction( new Func_sum()  );
        addFunction( new Func_sumPositive()  );
        addFunction( new Func_sumInteger()  );
        addFunction( new Func_sumNatural()  );

        // standard deviation function
        addFunction( new Func_standardDeviation()  );

        // geographical distance function
        addFunction( new Func_geographicalDistance() );
        addFunction( new Func_shortestDistance() );
        
                // hyperbolic tangent function
        addFunction( new Func_hyperbolicTangent() );

        // hyperbolic sine function
        addFunction( new Func_hyperbolicSine() );

		// truncate function
        addFunction( new Func_trunc<Real,Integer>()  );
        addFunction( new Func_trunc<RealPos,Natural>()  );

        // upper triangle of a matrix function
        addFunction( new Func_upperTriangle()  );
        
        // variance function
        addFunction( new Func_variance()  );

        // vector flatten
        addFunction( new Func_vectorFlatten() );

        // get ln Probability function
        addFunction( new Func_lnProbability() );
        
        // empirical cummulative probability function
        addFunction( new Func_posteriorPredictiveProbability()  );


 		/* Statistics functions (in folder "functions/statistics") */

		// some helper statistics for the DPP distribution
        addFunction( new Func_dppConcFromMean( )     );
        addFunction( new Func_dppMeanFromConc( )  );
        addFunction( new Func_stirling( )     );

		// count the number of unique elements in vector
        addFunction( new Func_numUniqueInVector<Real>( )  );
        addFunction( new Func_numUniqueInVector<RealPos>( )  );
        addFunction( new Func_numUniqueInVector<Integer>( )  );
        addFunction( new Func_numUniqueInVector<Natural>( )  );
        addFunction( new Func_numUniqueInVector<Probability>( )  );
        addFunction( new Func_numUniqueInVector<Simplex>( )  );

        // return a distcretized (by quantile) and normalized vector from a continuous distribution
        addFunction( new Func_fnNormalizedQuantile<Real>()    );
        addFunction( new Func_fnNormalizedQuantile<RealPos>()    );

        addFunction( new Func_discretizeDistribution( )         );
        addFunction( new Func_discretizePositiveDistribution( ) );

        // return a discretized gamma distribution (for gamma-dist rates)
        addFunction( new Func_discretizeBeta( )    );
        addFunction( new Func_discretizeBetaQuadrature( )    );
        addFunction( new Func_discretizeGamma( )   );
        addFunction( new Func_discretizeGammaQuadrature( )   );
        addFunction( new Func_discretizeLognormalQuadrature( )   );

        addFunction( new Func_varianceCovarianceMatrix( )           );
        addFunction( new Func_decomposedVarianceCovarianceMatrix( ) );
        addFunction( new Func_partialToCorrelationMatrix( )         );


    }
    catch(RbException& rbException)
    {

        RBOUT("Caught an exception while initializing functions in the workspace\n");
        std::ostringstream msg;
        rbException.print(msg);
        msg << std::endl;
        RBOUT(msg.str());

        RBOUT("Please report this bug to the RevBayes Development Core Team");

        RBOUT("Press any character to exit the program.");
        getchar();
        exit(1);
    }

}
