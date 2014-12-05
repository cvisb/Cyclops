/*
 * RcppCcdInterface.cpp
 *
 * @author Marc Suchard
 */
 
#include <sstream>
#include <vector>
#include <map>
 
#include "Rcpp.h"
#include "RcppCyclopsInterface.h"
#include "RcppModelData.h"
//#include "engine/ModelSpecifics.h"
#include "priors/JointPrior.h"
#include "CyclicCoordinateDescent.h"
#include "io/OutputWriter.h"
#include "RcppOutputHelper.h"
#include "RcppProgressLogger.h"

// Rcpp export code

using namespace Rcpp;

// // [[Rcpp::export("test")]]
// size_t ccdTest(SEXP exp) {
// 	if (Rf_isNull(exp)) {	
// 		return 0;
// 	}
// 	std::vector<std::string> strings = as<std::vector<std::string> >(exp);
// 	return strings.size();
// }


namespace bsccs {

 static std::map<ModelType, std::string> modelTypeNames = {
 	{ModelType::NORMAL, "ls"},
 	{ModelType::POISSON, "pr"},
 	{ModelType::LOGISTIC, "lr"},
 	{ModelType::CONDITIONAL_LOGISTIC, "clr"},
 	{ModelType::TIED_CONDITIONAL_LOGISTIC, "clr_exact"},
 	{ModelType::CONDITIONAL_POISSON, "cpr"},
 	{ModelType::SELF_CONTROLLED_MODEL, "sccs"},
 	{ModelType::COX, "cox"},
 	{ModelType::COX_RAW, "cox_raw"}
 };
 
} // namespace bsccs

// [[Rcpp::export(".cyclopsGetModelTypeNames")]]
std::vector<std::string> cyclopsGetModelTypeNames() {
	std::vector<std::string> names;
	for (auto& model : bsccs::modelTypeNames) {
		names.push_back(model.second);
	}	
	return names;
}

// [[Rcpp::export(".cyclopsGetRemoveInterceptNames")]]
std::vector<std::string> cyclopsGetRemoveInterceptNames() {
	using namespace bsccs;
	std::vector<std::string> names = {
		modelTypeNames[ModelType::CONDITIONAL_LOGISTIC],
		modelTypeNames[ModelType::TIED_CONDITIONAL_LOGISTIC],
		modelTypeNames[ModelType::CONDITIONAL_POISSON],
		modelTypeNames[ModelType::SELF_CONTROLLED_MODEL],
		modelTypeNames[ModelType::COX],
		modelTypeNames[ModelType::COX_RAW]
	};
	return names;
}

// [[Rcpp::export(".cyclopsGetIsSurvivalNames")]]
std::vector<std::string> cyclopsGetIsSurvivalNames() {
	using namespace bsccs;
	std::vector<std::string> names = {
		modelTypeNames[ModelType::COX],
		modelTypeNames[ModelType::COX_RAW]
	};
	return names;
}

// [[Rcpp::export(".cyclopsGetUseOffsetNames")]]
std::vector<std::string> cyclopsGetUseOffsetNames() {
	using namespace bsccs;
	std::vector<std::string> names = {
		modelTypeNames[ModelType::SELF_CONTROLLED_MODEL],
		modelTypeNames[ModelType::COX],
		modelTypeNames[ModelType::COX_RAW]
	};
	return names;
}

// [[Rcpp::export(.cyclopsSetBeta)]]
void cyclopsSetBeta(SEXP inRcppCcdInterface, int beta, double value) {
    using namespace bsccs;
    XPtr<RcppCcdInterface> interface(inRcppCcdInterface);    
    
    interface->getCcd().setBeta(beta - 1, value);
}

// [[Rcpp::export(.cyclopsSetFixedBeta)]]
void cyclopsSetFixedBeta(SEXP inRcppCcdInterface, int beta, bool fixed) {
    using namespace bsccs;
    XPtr<RcppCcdInterface> interface(inRcppCcdInterface);    
    
    interface->getCcd().setFixedBeta(beta - 1, fixed);
}

// [[Rcpp::export(".cyclopsGetIsRegularized")]]
bool cyclopsGetIsRegularized(SEXP inRcppCcdInterface, const int index) {
    using namespace bsccs;
    XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
    return interface->getCcd().getIsRegularized(index);
}

// [[Rcpp::export(".cyclopsGetLogLikelihood")]]
double cyclopsGetLogLikelihood(SEXP inRcppCcdInterface) {
	using namespace bsccs;
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	
	return interface->getCcd().getLogLikelihood();
}

// [[Rcpp::export(".cyclopsGetFisherInformation")]]
Eigen::MatrixXd cyclopsGetFisherInformation(SEXP inRcppCcdInterface, const SEXP sexpCovariates) {
	using namespace bsccs;
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	
// 	const int p = interface->getCcd().getBetaSize();
// 	std::vector<size_t> indices;
// 	for (int i = 0; i < p; ++i) indices.push_back(i);

    std::vector<size_t> indices;
    if (!Rf_isNull(sexpCovariates)) {
	
    	ProfileVector covariates = as<ProfileVector>(sexpCovariates);    	
    	for (auto it = covariates.begin(); it != covariates.end(); ++it) {
	        size_t index = interface->getModelData().getColumnIndex(*it);
	        indices.push_back(index);	        
	    }
	} else {
		for (size_t index = 0; index < interface->getModelData().getNumberOfColumns(); ++index) {
			indices.push_back(index);
		}
	}
	
    return interface->getCcd().computeFisherInformation(indices);
}

// // [[Rcpp::export("test")]]
// void cyclopsTest(std::vector<int> map, std::vector<std::vector<int> > list) {
//     for(auto it = begin(map); it != end(map); ++it) {
//         std::cout << *it << std::endl;
//     }
//     
//     for (auto it = begin(list); it != end(list); ++it) {
//         auto& vec = *it;
//         for (auto in = begin(vec); in != end(vec); ++in) {
//             std::cout << " " << *in;
//         }
//         std::cout << std::endl;
//     }
// }

// [[Rcpp::export(".cyclopsSetPrior")]]
void cyclopsSetPrior(SEXP inRcppCcdInterface, const std::vector<std::string>& priorTypeName, 
        const std::vector<double>& variance, SEXP excludeNumeric, SEXP sexpGraph) {
	using namespace bsccs;
	
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	
//	priors::PriorType priorType = RcppCcdInterface::parsePriorType(priorTypeName);
 	ProfileVector exclude;
 	if (!Rf_isNull(excludeNumeric)) {
 		exclude = as<ProfileVector>(excludeNumeric);
 	}
 	 	
 	HierarchicalChildMap map;
 	if (!Rf_isNull(sexpGraph)) {
 		map = as<HierarchicalChildMap>(sexpGraph);
 	} 	
 	 
    interface->setPrior(priorTypeName, variance, exclude, map);
}

// [[Rcpp::export(".cyclopsProfileModel")]]
List cyclopsProfileModel(SEXP inRcppCcdInterface, SEXP sexpCovariates, double threshold, 
		bool override, bool includePenalty) {
	using namespace bsccs;
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	
	if (!Rf_isNull(sexpCovariates)) {
		ProfileVector covariates = as<ProfileVector>(sexpCovariates);
		
		ProfileInformationMap profileMap;
        interface->profileModel(covariates, profileMap, threshold, override, includePenalty);
                
        std::vector<double> lower;
        std::vector<double> upper;
        std::vector<int> evals;
        
        for (ProfileVector::const_iterator it = covariates.begin();
        		it != covariates.end(); ++it) {        
            ProfileInformation info = profileMap[*it];
            lower.push_back(info.lower95Bound);
            upper.push_back(info.upper95Bound);
            evals.push_back(info.evaluations);           
        }
        return List::create(
            Rcpp::Named("covariate") = covariates,
            Rcpp::Named("lower") = lower,
            Rcpp::Named("upper") = upper,
            Rcpp::Named("evaluations") = evals 
        );
	}
        	
	return List::create();
}

// [[Rcpp::export(".cyclopsPredictModel")]]
List cyclopsPredictModel(SEXP inRcppCcdInterface) {
	using namespace bsccs;
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	double timePredict = interface->predictModel();
	
	List list = List::create(			
			Rcpp::Named("timePredict") = timePredict
		);
	RcppCcdInterface::appendRList(list, interface->getResult());
	return list;
}
	

// [[Rcpp::export(".cyclopsSetControl")]]
void cyclopsSetControl(SEXP inRcppCcdInterface, 
		int maxIterations, double tolerance, const std::string& convergenceType,
		bool useAutoSearch, int fold, int foldToCompute, double lowerLimit, double upperLimit, int gridSteps,
		const std::string& noiseLevel, int seed
		) {
	using namespace bsccs;
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	// Convergence control
	CCDArguments& args = interface->getArguments();
	interface->getArguments().maxIterations = maxIterations;
	interface->getArguments().tolerance = tolerance;
	interface->getArguments().convergenceType = RcppCcdInterface::parseConvergenceType(convergenceType);
	
	// Cross validation control
	args.useAutoSearchCV = useAutoSearch;
	args.fold = fold;
	args.foldToCompute = foldToCompute;
	args.lowerLimit = lowerLimit;
	args.upperLimit = upperLimit;
	args.gridSteps = gridSteps;
	NoiseLevels noise = RcppCcdInterface::parseNoiseLevel(noiseLevel);
	args.noiseLevel = noise;
	interface->setNoiseLevel(noise);
	args.seed = seed;
}

// [[Rcpp::export(".cyclopsRunCrossValidation")]]
List cyclopsRunCrossValidationl(SEXP inRcppCcdInterface) {	
	using namespace bsccs;
	
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);
	interface->getArguments().doFitAtOptimal = true;		
	double timeUpdate = interface->runCrossValidation();

	interface->diagnoseModel(0.0, 0.0);
	
	List list = List::create(
			Rcpp::Named("interface")=interface, 
			Rcpp::Named("timeFit")=timeUpdate
		);
	RcppCcdInterface::appendRList(list, interface->getResult());
	return list;
}

// [[Rcpp::export(".cyclopsFitModel")]]
List cyclopsFitModel(SEXP inRcppCcdInterface) {	
	using namespace bsccs;
	
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);		
	double timeUpdate = interface->fitModel();
	
	interface->diagnoseModel(0.0, 0.0);
	
	List list = List::create(
			Rcpp::Named("interface")=interface, 
			Rcpp::Named("timeFit")=timeUpdate
		);
	RcppCcdInterface::appendRList(list, interface->getResult());
	return list;
}

// [[Rcpp::export(".cyclopsLogModel")]]
List cyclopsLogModel(SEXP inRcppCcdInterface) {	
	using namespace bsccs;
	
	XPtr<RcppCcdInterface> interface(inRcppCcdInterface);	
	bool withASE = false;
	//return List::create(interface);
	
	double timeLogModel = interface->logModel(withASE);
	//std::cout << "getResult " << interface->getResult() << std::endl;
	
	CharacterVector names;
	names.push_back("interface");
	names.push_back("timeLog");
	CharacterVector oldNames = interface->getResult().attr("names");
	List list = List::create(interface, timeLogModel);
	for (int i = 0; i < interface->getResult().size(); ++i) {
		list.push_back(interface->getResult()[i]);
		names.push_back(oldNames[i]);
	}
	list.attr("names") = names;
	
	//, interface->getResult());	
	return list;
}

// [[Rcpp::export(".cyclopsInitializeModel")]]
List cyclopsInitializeModel(SEXP inModelData, const std::string& modelType, bool computeMLE = false) {
	using namespace bsccs;

	XPtr<RcppModelData> rcppModelData(inModelData);
	XPtr<RcppCcdInterface> interface(
		new RcppCcdInterface(*rcppModelData));
	
//	interface->getArguments().modelName = "ls"; // TODO Pass as argument	
	interface->getArguments().modelName = modelType;
	if (computeMLE) {
		interface->getArguments().computeMLE = true;
	}	
	double timeInit = interface->initializeModel();
	
//	bsccs::ProfileInformationMap profileMap;
//	// TODO Profile
//	bool withASE = false; //arguments.fitMLEAtMode || arguments.computeMLE || arguments.reportASE;    
//	double timeLogModel = interface->logModel(profileMap, withASE);
//	std::cout << "Done log model" << std::endl;
	
	List list = List::create(
			Rcpp::Named("interface") = interface, 
			Rcpp::Named("data") = rcppModelData, 
			Rcpp::Named("timeInit") = timeInit
		);
	return list;
}

namespace bsccs {
	
void RcppCcdInterface::appendRList(Rcpp::List& list, const Rcpp::List& append) {
	if (append.size() > 0) {
		CharacterVector names = list.attr("names");
		CharacterVector appendNames = append.attr("names");
		for (int i = 0; i < append.size(); ++i) {
			list.push_back(append[i]);
			names.push_back(appendNames[i]);
		}
		list.attr("names") = names;
	}		
}	

void RcppCcdInterface::handleError(const std::string& str) {	
//	Rcpp::stop(str); // TODO Want this to work
	::Rf_error(str.c_str());
}

bsccs::ConvergenceType RcppCcdInterface::parseConvergenceType(const std::string& convergenceName) {
	ConvergenceType type = GRADIENT;
	if (convergenceName == "gradient") {
		type = GRADIENT;
	} else if (convergenceName == "lange") {
		type = LANGE;
	} else if (convergenceName == "mittal") {
		type = MITTAL;
	} else if (convergenceName == "zhang") {
		type = ZHANG_OLES;
	} else {
		handleError("Invalid convergence type."); 	
	}
	return type;
}

bsccs::NoiseLevels RcppCcdInterface::parseNoiseLevel(const std::string& noiseName) {
	using namespace bsccs;
	NoiseLevels level = SILENT;
	if (noiseName == "silent") {
		level = SILENT; 
	} else if (noiseName == "quiet") {
		level = QUIET;
	} else if (noiseName == "noisy") {
		level = NOISY;
	} else {
		handleError("Invalid noise level.");
	}
	return level;
}

bsccs::priors::PriorType RcppCcdInterface::parsePriorType(const std::string& priorName) {
	using namespace bsccs::priors;
	bsccs::priors::PriorType priorType = NONE;
	if (priorName == "none") {
		priorType = NONE;
	} else if (priorName == "laplace") {
		priorType = LAPLACE;		
	} else if (priorName == "normal") {
		priorType = NORMAL;
	} else {
 		handleError("Invalid prior type."); 		
 	}	
 	return priorType;
}

//  static std::map<ModelType, std::string> modelTypeNames = {
//  	{ModelType::NORMAL, "ls"},
//  	{ModelType::POISSON, "pr"},
//  	{ModelType::LOGISTIC, "lr"},
//  	{ModelType::CONDITIONAL_LOGISTIC, "clr"},
//  	{ModelType::TIED_CONDITIONAL_LOGISTIC, "clr_exact"},
//  	{ModelType::CONDITIONAL_POISSON, "cpr"},
//  	{ModelType::SELF_CONTROLLED_MODEL, "sccs"},
//  	{ModelType::COX, "cox"},
//  	{ModelType::COX_RAW, "cox_raw"}
//  };
 
bsccs::ModelType RcppCcdInterface::parseModelType(const std::string& modelName) {
	// Parse type of model 
 	bsccs::ModelType modelType =  bsccs::ModelType::NONE;
 	auto model = begin(modelTypeNames);
 	for ( ; model != end(modelTypeNames); ++model) {
 		if (modelName == model->second) {
 			modelType = model->first;
 			break;
 		}
 	}
 	if (model == end(modelTypeNames)) {
 		handleError("Invalid model type.");
 	}
 	return modelType;
}

void RcppCcdInterface::setNoiseLevel(bsccs::NoiseLevels noiseLevel) {
    using namespace bsccs;
    ccd->setNoiseLevel(noiseLevel);
}

void RcppCcdInterface::setPrior(const std::vector<std::string>& basePriorName, const std::vector<double>& baseVariance,
		const ProfileVector& flatPrior, const HierarchicalChildMap& map) {			
	using namespace bsccs::priors;
	
	JointPriorPtr prior = makePrior(basePriorName, baseVariance, flatPrior, map);
	ccd->setPrior(prior);
}

priors::JointPriorPtr RcppCcdInterface::makePrior(const std::vector<std::string>& basePriorName, const std::vector<double>& baseVariance,
		const ProfileVector& flatPrior, const HierarchicalChildMap& map) {
	using namespace bsccs::priors;
	
 	PriorPtr singlePrior = bsccs::priors::CovariatePrior::makePrior(parsePriorType(basePriorName[0]));
 	singlePrior->setVariance(baseVariance[0]);
 
 	JointPriorPtr prior;
 	if (flatPrior.size() == 0) {
 		if (map.size() == 0) {
	 		prior = bsccs::make_shared<FullyExchangeableJointPrior>(singlePrior);
	 	} else {            
			bsccs::shared_ptr<HierarchicalJointPrior> hPrior = 
                std::make_shared<HierarchicalJointPrior>(singlePrior, 2); //Depth of hierarchy fixed at 2 right now
                // TODO Check normal at top of hierarchy!
            PriorPtr classPrior = bsccs::priors::CovariatePrior::makePrior(parsePriorType(basePriorName[1]));			
			hPrior->changePrior(classPrior, 1);
			
			HierarchicalParentMap parentMap;
			for (int parent = 0; parent < map.size(); ++parent) {
				auto& vec = map[parent];
				std::for_each(begin(vec), end(vec), [&](int child) {
					parentMap.push_back(parent);
				});
			}			
   		    hPrior->setHierarchy(
                parentMap,
                map
            );
			hPrior->setVariance(0, baseVariance[0]);
			hPrior->setVariance(1, baseVariance[1]);
            prior = hPrior;
	 	}
 	} else {
 		const int length =  modelData->getNumberOfColumns();
 		bsccs::shared_ptr<MixtureJointPrior> mixturePrior = bsccs::make_shared<MixtureJointPrior>(
 						singlePrior, length
 				);
 
 		PriorPtr noPrior = bsccs::make_shared<NoPrior>();
 		for (ProfileVector::const_iterator it = flatPrior.begin();
 				it != flatPrior.end(); ++it) {
 			int index = modelData->getColumnIndexByName(*it);
 			if (index == -1) {
 				std::stringstream error;
 				error << "Variable " << *it << " not found.";
 				handleError(error.str()); 			
 			} else {
 				mixturePrior->changePrior(noPrior, index);
 			}
 		}
 		prior = mixturePrior;
 		if (map.size() != 0) {
 			handleError("Mixtures of flat and hierarchical priors are not yet implemented."); 		
 		}
 	}
 	return prior;
}
// TODO Massive code duplicate (to remove) with CmdLineCcdInterface
void RcppCcdInterface::initializeModelImpl(
		ModelData** modelData,
		CyclicCoordinateDescent** ccd,
		AbstractModelSpecifics** model) {
	 	
	 *modelData = &rcppModelData;
	 
	// Parse type of model 
	ModelType modelType = parseModelType(arguments.modelName);

	*model = AbstractModelSpecifics::factory(modelType, **modelData);
	if (*model == nullptr) {
		handleError("Invalid model type.");
	}
 
 #ifdef CUDA
 	if (arguments.useGPU) {
 		*ccd = new GPUCyclicCoordinateDescent(arguments.deviceNumber, *reader, **model);
 	} else {
 #endif
 
 // Hierarchy management
// 	HierarchyReader* hierarchyData;
// 	if (arguments.useHierarchy) {
// 		hierarchyData = new HierarchyReader(arguments.hierarchyFileName.c_str(), *modelData);
// 	}
 
 
 	using namespace bsccs::priors;
 	PriorPtr singlePrior;
 	if (arguments.useNormalPrior) {
 		singlePrior = bsccs::make_shared<NormalPrior>();
 	} else if (arguments.computeMLE) {
 		if (arguments.fitMLEAtMode) {
 			handleError("Unable to compute MLE at posterior mode, if mode is not first explored."); 		
 		}
 		singlePrior = bsccs::make_shared<NoPrior>();
 	} else {
 		singlePrior = bsccs::make_shared<LaplacePrior>();
 	}
 	singlePrior->setVariance(arguments.hyperprior);
 
 	JointPriorPtr prior;
 	if (arguments.flatPrior.size() == 0) {
 		prior = bsccs::make_shared<FullyExchangeableJointPrior>(singlePrior);
 	} else {
 		const int length =  (*modelData)->getNumberOfColumns();
 		bsccs::shared_ptr<MixtureJointPrior> mixturePrior = bsccs::make_shared<MixtureJointPrior>(
 						singlePrior, length
 				);
 
 		PriorPtr noPrior = bsccs::make_shared<NoPrior>();
 		for (ProfileVector::const_iterator it = arguments.flatPrior.begin();
 				it != arguments.flatPrior.end(); ++it) {
 			int index = (*modelData)->getColumnIndexByName(*it);
 			if (index == -1) {
 				std::stringstream error;
 				error << "Variable " << *it << " not found.";
 				handleError(error.str()); 			
 			} else {
 				mixturePrior->changePrior(noPrior, index);
 			}
 		}
 		prior = mixturePrior;
 	}
 	 
 	//Hierarchy prior
// 	if (arguments.useHierarchy) {
// 		std::shared_ptr<HierarchicalJointPrior> hierarchicalPrior = std::make_shared<HierarchicalJointPrior>(singlePrior, 2); //Depth of hierarchy fixed at 2 right now
// 		PriorPtr classPrior = std::make_shared<NormalPrior>();
// 		hierarchicalPrior->changePrior(classPrior,1);
//         hierarchicalPrior->setHierarchy(
//                 hierarchyData->returnGetParentMap(),
//                 hierarchyData->returnGetChildMap()
//             );
// 		hierarchicalPrior->setVariance(0,arguments.hyperprior);
// 		hierarchicalPrior->setVariance(1,arguments.classHierarchyVariance);
// 		prior = hierarchicalPrior;
// 	}

  logger = bsccs::make_shared<loggers::RcppProgressLogger>();
  error = bsccs::make_shared<loggers::RcppErrorHandler>();
 
 	*ccd = new CyclicCoordinateDescent(*modelData /* TODO Change to ref */, **model, prior, logger, error);
 
 #ifdef CUDA
 	}
 #endif
 
 	(*ccd)->setNoiseLevel(arguments.noiseLevel);

}

void RcppCcdInterface::predictModelImpl(CyclicCoordinateDescent *ccd, ModelData *modelData) {

// 	bsccs::PredictionOutputWriter predictor(*ccd, *modelData);
// 
//     result = List::create();
//     OutputHelper::RcppOutputHelper test(result);
//     predictor.writeStream(test);
    
    NumericVector predictions(ccd->getPredictionSize());
    //std::vector<double> predictions(ccd->getPredictionSize());
    ccd->getPredictiveEstimates(&predictions[0], NULL);
    
    if (modelData->getHasRowLabels()) {
        size_t preds = ccd->getPredictionSize();
        CharacterVector labels(preds);
        for (size_t i = 0; i < preds; ++i) {
            labels[i] = modelData->getRowLabel(i);
        }
        predictions.names() = labels;    
    }
    result = List::create(
        Rcpp::Named("prediction") = predictions
    );
    
//     predictions.resize(ccd.getPredictionSize());
// 		ccd.getPredictiveEstimates(&predictions[0], NULL);
    
}
	    
void RcppCcdInterface::logModelImpl(CyclicCoordinateDescent *ccd, ModelData *modelData,
	    ProfileInformationMap& profileMap, bool withASE) {
 		
 		// TODO Move into super-class
  	EstimationOutputWriter estimates(*ccd, *modelData);
  	estimates.addBoundInformation(profileMap);
  	// End move
		
		result = List::create();
		OutputHelper::RcppOutputHelper out(result);  		
  	estimates.writeStream(out);	  	
}

void RcppCcdInterface::diagnoseModelImpl(CyclicCoordinateDescent *ccd, ModelData *modelData,	
		double loadTime,
		double updateTime) {
	
		result = List::create();
 		DiagnosticsOutputWriter diagnostics(*ccd, *modelData);
		OutputHelper::RcppOutputHelper test(result);  		
  	diagnostics.writeStream(test);	
}

RcppCcdInterface::RcppCcdInterface(RcppModelData& _rcppModelData) 
	: rcppModelData(_rcppModelData), modelData(NULL), ccd(NULL), modelSpecifics(NULL) {
	arguments.noiseLevel = SILENT; // Change default value from command-line version
}

//RcppCcdInterface::RcppCcdInterface() {
//    // Do nothing
//}

RcppCcdInterface::~RcppCcdInterface() {
//	std::cout << "~RcppCcdInterface() called." << std::endl;
	if (ccd) delete ccd;
	if (modelSpecifics) delete modelSpecifics;		
	// Do not delete modelData
}

} // namespace

