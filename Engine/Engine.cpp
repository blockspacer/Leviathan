#include "Include.h"
// ------------------------------------ //
#ifndef LEVIATHAN_ENGINE
#include "Engine.h"
#endif
using namespace Leviathan;
// ------------------------------------ //
#include "Application\Application.h"

Leviathan::Engine::Engine() : LeapData(NULL), MainConsole(NULL), MainFileHandler(NULL){

	Mainlog = NULL;
	Inited = false;
	Graph = NULL;
	Define = NULL;
	MTimer = NULL;
	MainRandom = NULL;
	RenderTimer = NULL;

	Inputs = NULL;
	Sound = NULL;

	TimePassed = 0;
	LastFrame = 0;

	GManager = NULL;
	Mainstore = NULL;
	MainScript = NULL;

	TickCount = 0;
	TickTime = 0;
	FrameCount = 0;

	MainCamera = NULL;
	KeyListener = NULL;
	MainEvents = NULL;
	GObjects = NULL;
	Loader = NULL;
	OutOMemory = NULL;
	AnimManager = NULL;

	Focused = true;
	MouseCaptured = false;
	WantsToCapture = false;

}
Engine* Leviathan::Engine::instance = NULL;

Engine* Leviathan::Engine::GetEngine(){
	return instance;
}
// ------------------------------------ //
bool Leviathan::Engine::Init(AppDef* definition){
	// get time, for monitoring how long load takes //
	__int64 InitStartTime = Misc::GetTimeMs64();
	// set static access to this object //
	instance = this;
	// store parameters //
	Define = definition;

	// create logger object //
	if(Logger::GetIfExists() != NULL){
		// already exists //
		Mainlog = Logger::Get();
	} else {
		Mainlog = new Logger();
	}

	// create //
	OutOMemory = new OutOfMemoryHandler();

	// create randomizer //
	MainRandom = new Random((int)InitStartTime);
	MainRandom->SetAsMain();

	// data storage //
	Mainstore = new DataStore(true);
	CLASS_ALLOC_CHECK(Mainstore);


	// search data folder for files //
	MainFileHandler = new FileSystem();
	CLASS_ALLOC_CHECK(MainFileHandler);
	MainFileHandler->Init();

	// file parsing //
	ObjectFileProcessor::Initialize();

	// main program wide event dispatcher //
	MainEvents = new EventHandler();
	CLASS_ALLOC_CHECK(MainEvents);
	if(!MainEvents->Init()){
		Logger::Get()->Error(L"Engine: Init: Init EventHandler failed!");
		return false;
	}

	// object holder //
	GObjects = new ObjectManager();
	CLASS_ALLOC_CHECK(GObjects);
	if(!GObjects->Init()){
		Logger::Get()->Error(L"Failed to init Engine, Init ObjectManager failed!");
		return false;
	}
	// timing object //
	MTimer = new Timer();

	// create script interface before renderer //
	MainScript = new ScriptInterface();
	if(!MainScript){

		Logger::Get()->Error(L"Engine: 008");
		return false;
	}
	if(!MainScript->Init()){
		Logger::Get()->Error(L"Failed to init Engine, Init ScriptInterface failed!");
		return false;
	}

	// create console after script engine //
	MainConsole = new ScriptConsole();
	CLASS_ALLOC_CHECK(MainConsole);

	if(!MainConsole->Init(MainScript)){

		Logger::Get()->Error(L"Engine: Init: failed to initialize Console, continuing anyway");
	}

	ObjectFileProcessor::LoadValueFromNamedVars<int>(Define->GetValues(), L"MaxFPS", FrameLimit, 120, true, L"Graphics: Init:");

	Graph = new Graphics();
	CLASS_ALLOC_CHECK(Graph);

	// call init //
	if(!Graph->Init(definition)){
		Logger::Get()->Error(L"Failed to init Engine, Init graphics failed! Aborting");
		return false;
	}


	// set height/width values //
	Mainstore->SetHeight(definition->GetWindow()->GetHeight());
	Mainstore->SetWidth(definition->GetWindow()->GetWidth());


	// 3d model animations //
	AnimManager = new AnimationManager();
	CLASS_ALLOC_CHECK(AnimManager);
	if(!AnimManager->Init()){
		Logger::Get()->Error(L"Failed to init Engine, AnimationManager init failed");
		return false;
	}

	// key listening //
	KeyListener = new KeyPressManager();
	CLASS_ALLOC_CHECK(KeyListener);

	// Gui //
	GManager = new Gui::GuiManager();
	CLASS_ALLOC_CHECK(GManager);

	if(!GManager->Init(definition, Graph)){

		Logger::Get()->Error(L"Failed to init Engine, Gui init failed");
		return false;
	}

	// create leap controller //
	LeapData = new LeapManager(this);
	if(!LeapData){
		Logger::Get()->Error(L"Engine: 008");
		return false;
	}
	// try here just in case //
	try{
		if(!LeapData->Init()){

			Logger::Get()->Info(L"Engine: Init: No Leap controller found, not using one");
		}
	}
	catch(...){
		// threw something //
		Logger::Get()->Error(L"Engine: Init: Leap threw something, even without leap this shouldn't happen; continuing anyway");
	}

	// create camera that always exists //
	MainCamera = new ViewerCameraPos();
	//MainCamera->SetPos(Float3(0, 0, 5));
	MainCamera->SetPos(Float3(0, 300, 60));
	
	// set camera to be last one to receive key presses because it will ALWAYS consume them //
	MainCamera->BecomeMainListeningCamera();

	// sound device //
	Sound = new SoundDevice();
	CLASS_ALLOC_CHECK(Sound);
	if(!Sound->Init()){

		Logger::Get()->Error(L"Failed to init Engine, sound init failed");
		return false;
	}
	
	Inputs = new Input();
	CLASS_ALLOC_CHECK(Inputs);
	// load control structures //
	if(!Inputs->Init(Define->HInstance, Mainstore->GetWidth(), Mainstore->GetHeight())){

		Logger::Get()->Error(L"Failed to init Engine, input init failed");
		return false;
	}

	// create object loader //
	Loader = new ObjectLoader(this);
	CLASS_ALLOC_CHECK(Loader);

	// measuring //
	RenderTimer = new RenderingStatistics();
	CLASS_ALLOC_CHECK(RenderTimer);

	Inited = true;

	PostLoad();

	Logger::Get()->Info(L"Engine init took "+Convert::ToWstring(Misc::GetTimeMs64()-InitStartTime)+L" ms", false);

	// let's send a debug message telling engine initialized //
	Logger::Get()->Info(L"Engine initialized", true);
	return true;
}

void Leviathan::Engine::PostLoad(){

	// get time //
	LastFrame = Misc::GetTimeMs64();

	//// it is preferable that mouse isn't captured on start //
	SetGuiActive(true);
	//SetGuiActive(false);

	// increase start count //
	int startcounts = 0;

	if(Mainstore->GetValueAndConvertTo<int>(L"StartCount", startcounts)){
		// increase //
		Mainstore->SetValue(L"StartCount", new VariableBlock(new IntBlock(startcounts+1)));
	} else {
		
		Mainstore->AddVar(new NamedVariableList(L"StartCount", new VariableBlock(1)));
		// set as persistent //
		Mainstore->SetPersistance(L"StartCount", true);
	}
}

void Leviathan::Engine::Release(){
	// set inited to false to cause all engine functions just return //
	Inited = false;

	// objects WILL contain things that want to do everything before shutting down //
	SAFE_RELEASEDEL(GObjects);

	// Gui is very picky about delete order
	SAFE_RELEASEDEL(GManager);

	// clears all running timers that might have accidentally been left running //
	TimingMonitor::ClearTimers();

	SAFE_RELEASEDEL(LeapData);
	SAFE_DELETE(MainCamera);

	// console needs to be before script release //
	SAFE_RELEASEDEL(MainConsole);

	SAFE_RELEASEDEL(MainScript);
	// save at this point (just in case it crashes before exiting) //
	Mainlog->Save();

	SAFE_DELETE(Loader);

	SAFE_RELEASEDEL(AnimManager);
	SAFE_RELEASEDEL(Graph);
	SAFE_DELETE(RenderTimer);

	SAFE_RELEASEDEL(Inputs);
	SAFE_RELEASEDEL(Sound);
	SAFE_DELETE(Mainstore);

	SAFE_RELEASEDEL(MainEvents);

	SAFE_DELETE(KeyListener);


	SAFE_DELETE(MTimer);
	SAFE_DELETE(Mainlog);

	// delete randomizer last, for obvious reasons //
	SAFE_DELETE(MainRandom);

	ObjectFileProcessor::Release();
	SAFE_RELEASEDEL(MainFileHandler);

	// safe to delete this here //
	SAFE_DELETE(OutOMemory);
}
// ------------------------------------ //
void Leviathan::Engine::Tick(){
	// get time since last update //
	__int64 CurTime = Misc::GetTimeMs64();
	TimePassed = (int)(CurTime-LastFrame);

	if((TimePassed < TICKSPEED)){
		// no tick time yet //
		return;
	}

	// update focus state //


	//LastFrame = CurTime;
	LastFrame += TICKSPEED;
	TickCount++;
	
	// update input //
	Inputs->Update();
	LeapData->OnTick(TimePassed);

	// sound tick //
	Sound->Tick(TimePassed);

	if(Focused){
		KeyListener->ProcessInput(Inputs);
	} else {
		// send this so that all keys are set as up //
		KeyListener->RunUnfocusedProcess(Inputs);
	}

	GManager->GuiTick(TimePassed);

	// update texture usage times, to allow unused textures to be unloaded //
	Graph->GetTextureManager()->TimePass(TimePassed);

	// some dark magic here //
	if(TickCount % 25 == 0){
		// update values
		Mainstore->SetTickCount(TickCount);
		Mainstore->SetTickTime(TickTime);
		

		// send updated rendering statistics //
		RenderTimer->ReportStats(Mainstore);
	}

	// send tick event //
	MainEvents->CallEvent(new Event(EVENT_TYPE_ENGINE_TICK, new int(TickCount)));

	TickTime = (int)(Misc::GetTimeMs64()-LastFrame);
}
// ------------------------------------ //
void Leviathan::Engine::RenderFrame(){

	int SinceLastFrame = -1;

	// limit check //
	if(!RenderTimer->CanRenderNow(FrameLimit, SinceLastFrame)){
		// fps would go too high //
		
		return;
	}

	// since last frame is in microseconds 10^-6 convert to milliseconds //
	// SinceLastFrame is always more than 1000 (always 1 ms or more) //
	SinceLastFrame /= 1000;

	FrameCount++;

	// update monitor objects //
	// advanced statistic start monitoring //
	RenderTimer->RenderingStart();

	MainEvents->CallEvent(new Event(EVENT_TYPE_FRAME_BEGIN, new int(SinceLastFrame)));

	// Gui object animations //
	GManager->AnimationTick(SinceLastFrame);

	// update simulations will go here, or not //

	// Gui //
	GManager->Render();

	// render //
	vector<BaseRenderable*>* objects = GObjects->GetRenderableObjects();
	Graph->Frame(SinceLastFrame, MainCamera, *objects);

	MainEvents->CallEvent(new Event(EVENT_TYPE_FRAME_END, new int(FrameCount)));

	// advanced statistics frame has ended //
	RenderTimer->RenderingEnd();
}
// ------------------------------------ //
bool Leviathan::Engine::DoWindowResize(int width, int height){
	// tell window class to resize the real windows window //
	Define->GetWindow()->ResizeWindow(width, height);
	// window class updates engine's values //

	return true;
}
void Leviathan::Engine::OnResize(int width, int height){
	// skip if width and height are the same //
	if((Mainstore->GetWidth() == width) && (Mainstore->GetHeight() == height)){
		return;
	}

	// update values in DataStore and call resize functions of things //
	Mainstore->SetWidth(width);
	Mainstore->SetHeight(height);

	// update rendering stuff //


	// update Gui //
	GManager->OnResize();

	// inputs manager size //
	Inputs->ResolutionUpdated(width, height);
}
// ------------------------------------ //
void Leviathan::Engine::CaptureMouse(bool toset){
	MouseCaptured = toset;

	Define->GetWindow()->SetHideCursor(toset);

	Inputs->SetMouseCapture(toset);
	WantsToCapture = toset;
}
void Leviathan::Engine::SetGuiActive(bool toset){
	GuiActive = toset;
	if(GuiActive){
		CaptureMouse(false);

	} else {
		Define->GetWindow()->SetMouseToCenter();
		CaptureMouse(true);
	}
	Mainstore->SetGUiActive(GuiActive);
}

void Leviathan::Engine::LoseFocus(){
	Inputs->SetMouseCapture(false);
	Focused = false;
}
void Leviathan::Engine::GainFocus(){
	Focused = true;
	if(!GuiActive){
		Inputs->SetMouseCapture(true);
	}

	//SaveScreenShot();
}
// ------------------------------------ //
DLLEXPORT void Leviathan::Engine::ExecuteCommandLine(const wstring &commands){

}
void Leviathan::Engine::RunScrCommand(wstring command, wstring params){
#ifdef _DEBUG
	Mainlog->Info(L"[DEBUG] Running script command: "+command+L" with params "+params, false);
#endif

	// pass to console //
	DEBUG_BREAK;

}

DLLEXPORT void Leviathan::Engine::SaveScreenShot(){

	const wstring fileprefix = MainFileHandler->GetDataFolder()+L"Screenshots\\Captured_frame_";


	Graph->SaveScreenShot(Convert::WstringToString(fileprefix));
}

// ------------------------------------ //

// ------------------------------------ //

// ------------------------------------ //
