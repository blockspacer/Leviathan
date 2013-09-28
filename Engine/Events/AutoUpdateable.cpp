#include "Include.h"
// ------------------------------------ //
#ifndef LEVIATHAN_AUTOUPDATEABLE
#include "AutoUpdateable.h"
#endif
#include "Common\DataStoring\DataStore.h"
using namespace Leviathan;
// ------------------------------------ //

Leviathan::AutoUpdateableObject::AutoUpdateableObject(){
	ValuesUpdated = false;
}
Leviathan::AutoUpdateableObject::~AutoUpdateableObject(){
	if(MonitoredValues.size() != 0){
		StopMonitoring(MonitoredValues, true);
	}
}
// ------------------------------------ //
void Leviathan::AutoUpdateableObject::StartMonitoring(const std::vector<VariableBlock*> &IndexesAndNamesToListen){

	// loop through wanted listen indexes and names //
	for(size_t i = 0; i < IndexesAndNamesToListen.size(); i++){
		// check for wstring //
		if(IndexesAndNamesToListen[i]->GetBlockConst()->Type == DATABLOCK_TYPE_WSTRING){
			// start listening on named index //

			// register new listener //
			DataStore::Get()->RegisterListener(this, new DataListener(-1, false, (wstring)*IndexesAndNamesToListen[i]));

			// add to listened things //
			MonitoredValues.push_back(shared_ptr<VariableBlock>(new VariableBlock(IndexesAndNamesToListen[i]->GetBlockConst()->AllocateNewFromThis())));

			continue;
		}

		// force to int //
		if(!IndexesAndNamesToListen[i]->IsConversionAllowedNonPtr<int>()){
			// cannot be listened to //
			Logger::Get()->Warning(L"AutoUpdateableObject: StartMonitoring: cannot listen to index (not int/wstring)"
				+(wstring)*IndexesAndNamesToListen[i]);

			continue;
		}

		// should be good now //
		int tmpindex = (int)*IndexesAndNamesToListen[i];

		// register new listener //
		DataStore::Get()->RegisterListener(this, new DataListener(tmpindex, true, L""));

		// add to listened things //
		MonitoredValues.push_back(shared_ptr<VariableBlock>(new VariableBlock(tmpindex)));
	}
}
void Leviathan::AutoUpdateableObject::StopMonitoring(vector<shared_ptr<VariableBlock>> &unregisterindexandnames, bool all /*= false*/){
	// check are all going to be deleted anyways //
	if(all){
		// clear all //
		MonitoredValues.clear();
		// unregister all //
		DataStore::Get()->RemoveListener(this, -1, L"", true);

		return;

	}
	// remove specific ones //
	for(size_t i = 0; i < unregisterindexandnames.size(); i++){
		// find matching already listened index //
		for(size_t a = 0; a < MonitoredValues.size(); i++){
			// check match //
			if(*unregisterindexandnames[i] == *MonitoredValues[a]){
				// unregister it //
				wstring possiblename = L"";
				int possibleindex = -1;

				if(unregisterindexandnames[i]->GetBlockConst()->Type == DATABLOCK_TYPE_WSTRING){
					// assign //
					unregisterindexandnames[i]->ConvertAndAssingToVariable<wstring>(possiblename);

				} else {
					// force to int //
					unregisterindexandnames[i]->ConvertAndAssingToVariable<int>(possibleindex);
				}

				// unregister from listening //
				DataStore::Get()->RemoveListener(this, possibleindex, possiblename, false);

				// erase //
				MonitoredValues.erase(MonitoredValues.begin()+a);
				break;
			}
		}
	}
}

DLLEXPORT bool Leviathan::AutoUpdateableObject::OnUpdate(const shared_ptr<NamedVariableList> &updated){
	ValuesUpdated = true;
	
	// push to update vector //
	UpdatedValues.push_back(updated);

	return true;
}


// ------------------------------------ //
void Leviathan::AutoUpdateableObject::_PopUdated(){
	// just clear the vector, should automatically delete //
	UpdatedValues.clear();

	ValuesUpdated = false;
}
// ------------------------------------ //

// ------------------------------------ //


