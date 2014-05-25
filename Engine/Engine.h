#ifndef LEVIATHAN_ENGINE
#define LEVIATHAN_ENGINE
// ------------------------------------ //
#ifndef LEVIATHAN_DEFINE
#include "Define.h"
#endif
// ------------------------------------ //
// ---- includes ---- //
#include "Application/AppDefine.h"
#include "Events/EventHandler.h"
#include "Statistics/RenderingStatistics.h"
#include "FileSystem.h"
#include "Common/DataStoring/DataStore.h"
#include "Handlers/OutOfMemoryHandler.h"
#include "Utility/Random.h"
#include "Threading/ThreadingManager.h"



#define TICKSPEED 60



namespace Leviathan{

	//! \brief The main class of the Leviathan Game Engine
	//!
	//! Allocates a lot of classes and performs almost all startup operations.
	//! \note Should be thread safe, but might not actually be
	class Engine : public Object, public ThreadSafe{
		
		friend GraphicalInputEntity;
		friend Gui::GuiManager;
	public:
		DLLEXPORT Engine(LeviathanApplication* owner);
		DLLEXPORT ~Engine();

		DLLEXPORT bool Init(AppDef* definition, NETWORKED_TYPE ntype);
		//! \todo Add a thread that monitors if the thing gets stuck on a task
		DLLEXPORT void Release(bool forced = false);

		//! \brief Sets objects ready to be released
		//! \note The Tick function must be called after this but before Release
		DLLEXPORT void PreRelease();

		//! \brief Checks if PreRelease is done and Release can be called
		//! \pre PreRelease is called
		DLLEXPORT bool HasPreRleaseBeenDone() const;


		//! \brief Causes VLD to dump current memory leaks
		DLLEXPORT static void DumpMemoryLeaks();

		DLLEXPORT void Tick();
		DLLEXPORT void RenderFrame();
		DLLEXPORT void PhysicsUpdate();
		DLLEXPORT void ResetPhysicsTime();
		DLLEXPORT void PreFirstTick();

		DLLEXPORT int GetWindowOpenCount();

		// ------------------------------------ //
		// Passes the commands and preprocesses them, but also interprets commands like --nogui //
		DLLEXPORT void PassCommandLine(const wstring &commands);
		// Runs the normal commands passed by the PassCommandLine function //
		DLLEXPORT void ExecuteCommandLine();


		DLLEXPORT shared_ptr<GameWorld> CreateWorld(GraphicalInputEntity* owningwindow, shared_ptr<ViewerCameraPos> worldscamera);

		DLLEXPORT void SaveScreenShot();

		DLLEXPORT Graphics* GetGraphics(){ return Graph; };
		DLLEXPORT EventHandler* GetEventHandler(){ return MainEvents; };
		DLLEXPORT ObjectLoader* GetObjectLoader(){return Loader;};
		DLLEXPORT RenderingStatistics* GetRenderingStatistics(){ return RenderTimer;};
		DLLEXPORT LeapManager* GetLeapManager(){ return LeapData;};
		DLLEXPORT ScriptConsole* GetScriptConsole(){ return MainConsole;};
		DLLEXPORT FileSystem* GetFileSystem(){ return MainFileHandler; };
		DLLEXPORT AppDef* GetDefinition(){ return Define;};
		DLLEXPORT GraphicalInputEntity* GetWindowEntity(){ return GraphicalEntity1; };
		DLLEXPORT NewtonManager* GetNewtonManager(){ return _NewtonManager; };
		DLLEXPORT LeviathanApplication* GetOwningApplication(){ return Owner; };
		DLLEXPORT PhysicsMaterialManager* GetPhysicalMaterialManager(){ return PhysMaterials; };
		DLLEXPORT NetworkHandler* GetNetworkHandler(){ return _NetworkHandler; };
		DLLEXPORT ThreadingManager* GetThreadingManager(){ return _ThreadingManager; };
		DLLEXPORT ResourceRefreshHandler* GetResourceRefreshHandler(){ return _ResourceRefreshHandler; };

		DLLEXPORT bool GetNoGui(){ return NoGui; };

		// Static access //
		DLLEXPORT static Engine* GetEngine();
		DLLEXPORT static Engine* Get();

		// For NoGui mode //
#ifdef _WIN32
		DLLEXPORT static void WinAllocateConsole();
#endif

	private:
		// after load function //
		void PostLoad();

		// Function called by first instance of Window class after creating a window to not error when registering threads to work with Ogre //
		void _NotifyThreadsRegisterOgre();

		// ------------------------------------ //
		AppDef* Define;

		RenderingStatistics* RenderTimer;
		Graphics* Graph;
		GraphicalInputEntity* GraphicalEntity1;

		SoundDevice* Sound;
		DataStore* Mainstore;
		EventHandler* MainEvents;
		ScriptInterface* MainScript;
		ObjectLoader* Loader;
		LeapManager* LeapData;
		ScriptConsole* MainConsole;
		FileSystem* MainFileHandler;
		Random* MainRandom;
		OutOfMemoryHandler* OutOMemory;
		NewtonManager* _NewtonManager;
		PhysicsMaterialManager* PhysMaterials;
		NetworkHandler* _NetworkHandler;
		ThreadingManager* _ThreadingManager;
		RemoteConsole* _RemoteConsole;
		ResourceRefreshHandler* _ResourceRefreshHandler;


		IDFactory* IDDefaultInstance;
		LeviathanApplication* Owner;
		// world data //
		std::vector<shared_ptr<GameWorld>> GameWorlds;

		// data //
		__int64 LastFrame;
		int TimePassed;
		int FrameLimit;
		int TickCount;
		int TickTime;
		int FrameCount;

		// flags //
		bool MouseCaptured : 1;
		bool WantsToCapture : 1;
		bool Focused : 1;
		bool GuiActive : 1;
		bool Inited : 1;
		//! Set when PreRelease is called and Tick has happened
		bool PreReleaseDone : 1;
		//! Set when PreRelease called and waiting for Tick
		//! see PreReleaseDone
		bool PreReleaseWaiting : 1;
		bool NoGui;
		bool NoLeap;



		// Stores the command line before running it //
		std::vector<unique_ptr<wstring>> PassedCommands;

		// NoGui input handler //
		boost::thread CinThread;

		static Engine* instance;
	};

}
#endif
