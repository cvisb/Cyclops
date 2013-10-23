/*
 * Parameter.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: trevorshaddox
 */

#include "Parameter.h"


using namespace std;
namespace bsccs{

	Parameter::Parameter(bsccs::real * data, int sizeIn){
		initialize(data, sizeIn);
	}

	Parameter::Parameter(){}

	void Parameter::initialize(bsccs::real * data, int sizeIn){
		restorable = true;
		size = sizeIn;
		numberOfChanges = 0;
		parameterValues = (bsccs::real*) calloc(sizeIn, sizeof(bsccs::real));
		storedValues = (bsccs::real*) calloc(size, sizeof(bsccs::real));
		memcpy(storedValues, data, sizeof(bsccs::real)*sizeIn);
		memcpy(parameterValues, data, sizeof(bsccs::real)*sizeIn);


		for (int i = 0; i < size; i++) {
			parameterDoubleValues.push_back(1.00);
			storedDoubleValues.push_back(1.00);
			vectorOfChanges.push_back(false);
		}

	}

	Parameter::~Parameter(){
		free(storedValues);
		free(parameterValues);

	}

	int Parameter::getSize(){
		return size;
	}

	bsccs::real	 Parameter::get(int index){
		return parameterValues[index];
	}

	bsccs::real Parameter::getStored(int index){
		return storedValues[index];
	}

	void Parameter::set(int index, bsccs::real setTo){
		numberOfChanges ++;
		vectorOfChanges[index] = true;
		parameterValues[index] = setTo;
	}

	void Parameter::set(bsccs::real* newData){
		memcpy(parameterValues, newData, sizeof(bsccs::real)*size);
	}

	void Parameter::logParameter() {
		cout << "LogParameter:: Parameter value is <";
		for (int i = 0; i < 4; i++) {
			cout << parameterValues[i] << ", ";
		}
		cout << ">" << endl;

	}

	void Parameter::logStored() {
		cout << "LogStored: Stored Parameter value is <";
		for (int i = 0; i < 4; i++) {
			cout << storedValues[i] << ", ";
		}
		cout << ">" << endl;

	}

	void Parameter::logParameter(const char * fileName) {
		// TODO Implement a write to file function...
	}

	void Parameter::store(){
		memcpy(storedValues, parameterValues, sizeof(bsccs::real)*size);
		restorable = true;
	}

	void Parameter::restore(){
		cout << "Parameter::restore()" << endl;
		if (restorable) {
			bsccs::real* temp;
			temp = storedValues;
			storedValues = parameterValues;
			parameterValues = temp;
			restorable = false;
		} else {
			cerr << "error trying to restore twice" << endl;
			exit(-1);		}
	}

	bool Parameter::getRestorable() {
		return restorable;
	}

	void Parameter::setRestorable(bool status) {
		restorable = status;
	}

	void Parameter::resetChangesRecorded() {
		numberOfChanges = 0;

		for (int i = 0; i < size; i++) {
			vectorOfChanges.push_back(false);
		}

	}

	std::vector<int> Parameter::returnIndicesOfChanges(){

		std::vector<int> indicesOfChanges;

		for (int i = 0; i < size; i++) {
			if (vectorOfChanges[i]){
				indicesOfChanges.push_back(i);
			}
		}
		return(indicesOfChanges);

	}

	std::vector<double> Parameter::returnCurrentValues() {
		std::vector<double> returnVector;

		cout << "Parameter value is <";
		for (int i = 0; i < 4; i++) {
			cout << parameterValues[i] << ", ";
		}
		cout << ">" << endl;


		for (int i = 0; i < size; i++) {
			returnVector.push_back((double) parameterValues[i]);
		}

		return returnVector;
	}

	std::vector<double> Parameter::returnStoredValues() {
		std::vector<double> returnVector;

		for (int i = 0; i < size; i++) {
			returnVector.push_back((double) storedValues[i]);
		}

		return returnVector;
	}

	std::vector<double> * Parameter::returnCurrentValuesPointer() {
		//std::vector<double> returnVector;

		for (int i = 0; i < size; i++) {
			//returnVector.push_back((double) parameterValues[i]);
			parameterDoubleValues[i] = (double) parameterValues[i];
		}


		return & parameterDoubleValues; //returnVector;
	}

	std::vector<double> * Parameter::returnStoredValuesPointer() {
		//std::vector<double> returnVector;

//		for (int i = 0; i < size; i++) {
//			//returnVector.push_back((double) storedValues[i]);
//			storedDoubleValues.push_back((double) storedValues[i]);
//		}

		for (int i = 0; i < size; i++) {
			storedDoubleValues[i] = (double) storedValues[i];
		}


		return & storedDoubleValues; //returnVector;
	}



}


