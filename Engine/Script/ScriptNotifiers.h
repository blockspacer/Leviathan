#pragma once
// ------------------------------------ //
#include "Define.h"
// ------------------------------------ //
#include "Common/BaseNotifiable.h"
#include "angelscript.h"
#include "Common/ReferenceCounted.h"
#include <map>


namespace Leviathan{

	bool RegisterNotifiersWithAngelScript(asIScriptEngine* engine);
	void RegisterNotifierTypesWithAngelScript(asIScriptEngine* engine,
        std::map<int, std::string> &typeids);


	//! \brief BaseNotifier for use in scripts
	class ScriptNotifier : public BaseNotifierAll, public ReferenceCounted{
	public:
		DLLEXPORT ScriptNotifier(asIScriptFunction* functiontocall);
		DLLEXPORT virtual ~ScriptNotifier();



		REFERENCECOUNTED_ADD_PROXIESFORANGELSCRIPT_DEFINITIONS(ScriptNotifier);

	protected:

		//! \brief Calls into the script engine
		virtual void OnNotified();

		// ------------------------------------ //

		//! The function that is called when this is notified
		asIScriptFunction* CallbackFunction;
	};


	//! \brief BaseNotifiable for use in scripts
	class ScriptNotifiable : public BaseNotifiableAll, public ReferenceCounted{
	public:
		DLLEXPORT ScriptNotifiable(asIScriptFunction* functiontocall);
		DLLEXPORT virtual ~ScriptNotifiable();


		REFERENCECOUNTED_ADD_PROXIESFORANGELSCRIPT_DEFINITIONS(ScriptNotifiable);


	protected:

		//! \brief Calls into the script engine
		virtual void OnNotified();

		// ------------------------------------ //

		//! The function that is called when this is notified
		asIScriptFunction* CallbackFunction;
	};


}

