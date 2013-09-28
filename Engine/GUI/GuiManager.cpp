#include "Include.h"
// ------------------------------------ //
#ifndef LEVIATHAN_GUI_MAIN
#include "GuiManager.h"
#endif
#include "Engine.h"
#include "Script\ScriptInterface.h"
#include "FileSystem.h"
#include "Rendering\Graphics.h"
#include <boost\assign\list_of.hpp>
#include <Rocket\Controls\Controls.h>
#include <Rocket\Debugger\Debugger.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include "Rendering\GUI\RenderInterfaceOgre3D.h"
#include "Rendering/GUI/FontManager.h"
#include "GuiCollection.h"
#include "Common\DataStoring\DataStore.h"
#include "Common\DataStoring\DataBlock.h"
using namespace Leviathan;
using namespace Leviathan::Gui;
// ------------------------------------ //
Leviathan::Gui::GuiManager::GuiManager() : ID(IDFactory::GetID()), RocketRenderer(NULL), RocketInternals(NULL), WindowContext(NULL), Visible(true),
	Cursor(NULL)
{
	ObjectAmountChanged = false;

	staticaccess = this;
}
Leviathan::Gui::GuiManager::~GuiManager(){

	staticaccess = NULL;
}

GuiManager* Leviathan::Gui::GuiManager::staticaccess = NULL;
GuiManager* Leviathan::Gui::GuiManager::Get(){
	return staticaccess; 
}
// ------------------------------------ //
bool Leviathan::Gui::GuiManager::Init(AppDef* vars, Graphics* graph){

	ThisRenderer = graph;


	// create renderer for Rocket GUI //
	RocketRenderer = new RenderInterfaceOgre3D(graph->GetOwningWindow()->GetWidth(), graph->GetOwningWindow()->GetHeight());

	Rocket::Core::SetRenderInterface(RocketRenderer);

	RocketInternals = new RocketSysInternals();

	Rocket::Core::SetSystemInterface(RocketInternals);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	// font database //
	Rocket::Core::FontDatabase::Initialise();

	// load fonts //
	graph->GetFontManager()->LoadAllFonts();

	// create context for this window //
	WindowContext = Rocket::Core::CreateContext("window01_context", Rocket::Core::Vector2i(graph->GetOwningWindow()->GetWidth(), 
		graph->GetOwningWindow()->GetHeight())/*, static_cast<Rocket::Core::RenderInterface*>RocketRenderer*/);
	
	Rocket::Debugger::Initialise(WindowContext);

	Rocket::Debugger::SetVisible(true);



	// we render during Ogre overlay //
	graph->GetWindowScene()->addRenderQueueListener(this);


	return true;
}

void Leviathan::Gui::GuiManager::Release(){
	// default mouse back //
	SetMouseFile(L"none");

	for(unsigned int i = 0; i < Objects.size(); i++){
		// object's release function will do everything needed (even deleted if last reference) //
		SAFE_RELEASE(Objects[i]);

		Objects.erase(Objects.begin()+i);
		i--;
	}

	SAFE_DELETE_VECTOR(Collections);
	// unload sheets //
	GuiSheets.clear();


	// shutdown rocket //
	WindowContext->RemoveReference();
	Rocket::Core::Shutdown();
	Rocket::Core::FontDatabase::Shutdown();

	SAFE_DELETE(RocketInternals);
	SAFE_DELETE(RocketRenderer);
}
// ------------------------------------ //
DLLEXPORT bool Leviathan::Gui::GuiManager::ProcessKeyDown(OIS::KeyCode key, int specialmodifiers){

	for(unsigned int i = 0; i < Collections.size(); i++){
		if(Collections[i]->GetTogglingKey().Match(key, specialmodifiers, false)){
			// is a match, toggle //
			Collections[i]->ToggleState();

			return true;
		}
	}

	return false;
}
// ------------------------------------ //
void Leviathan::Gui::GuiManager::GuiTick(int mspassed){
	// send tick event //

}

void Leviathan::Gui::GuiManager::Render(){
	UpdateArrays();

	


	// update Rocket input //
	ThisRenderer->GetOwningWindow()->GatherInput(WindowContext);
}
// ------------------------------------ //
void Leviathan::Gui::GuiManager::OnResize(){
	// call events //
	this->CallEvent(new Event(EVENT_TYPE_WINDOW_RESIZE, (void*)new Int2(DataStore::Get()->GetWidth(), DataStore::Get()->GetHeight())));

	// resize Rocket on this window //
	WindowContext->SetDimensions(Rocket::Core::Vector2i(DataStore::Get()->GetWidth(), DataStore::Get()->GetHeight()));
}
// ------------------------------------ //
bool Leviathan::Gui::GuiManager::AddGuiObject(BaseGuiObject* obj){
	ObjectAmountChanged = true;

	Objects.push_back(obj);
	return true;
}

void Leviathan::Gui::GuiManager::DeleteObject(int id){
	for(unsigned int i = 0; i < Objects.size(); i++){
		if(Objects[i]->GetID() == id){
			ObjectAmountChanged = true;

			SAFE_RELEASE(Objects[i]);
			Objects.erase(Objects.begin()+i);
			return;
		}
	}
}

int Leviathan::Gui::GuiManager::GetObjectIndexFromId(int id){
	for(unsigned int i = 0; i < Objects.size(); i++){
		if(Objects[i]->GetID() == id)
			return i;
	}
	return -1;
}

BaseGuiObject* Leviathan::Gui::GuiManager::GetObject(unsigned int index){
	ARR_INDEX_CHECK(index, Objects.size()){
		return Objects[index];
	}
	return NULL;
}
// ------------------------------------ //
DLLEXPORT bool Leviathan::Gui::GuiManager::LoadGUIFile(const wstring &file){
	// header flag definitions //
	vector<shared_ptr<NamedVariableList>> headerdata;

	// parse file to structure //
	vector<shared_ptr<ObjectFileObject>> data = ObjectFileProcessor::ProcessObjectFile(file, headerdata);

	NamedVars varlist(headerdata);

	// we need to load the corresponding rocket file first //
	wstring relativepath;
	// get path //
	ObjectFileProcessor::LoadValueFromNamedVars<wstring>(varlist, L"RocketScript", relativepath, L"", true,
		L"GuiManager: LoadGUIFile: No Rocket script file attached: ");

	if(!relativepath.size()){

		return false;
	}
	
	shared_ptr<GuiLoadedSheet> sheet;
	
	try{
		sheet = shared_ptr<GuiLoadedSheet>(new GuiLoadedSheet(WindowContext, Convert::WstringToString(FileSystem::GetScriptsFolder()+relativepath)));

		if(!sheet.get()){

			return false;
		}
	}
	catch(const ExceptionInvalidArguement &e){
		// something was thrown //
		Logger::Get()->Error(L"GuiManager: LoadGUIFile: exit due to exception:");
		e.PrintToLog();

		return false;
	}

	// temporary object data stores //
	vector<BaseGuiObject*> TempOs;

	// reserve space //
	TempOs.reserve(data.size());


	for(size_t i = 0; i < data.size(); i++){
		// check what type the object is //
		if(data[i]->TName == L"Collection"){

			if(!GuiCollection::LoadCollection(this, *data[i], sheet)){

				// report error //
				Logger::Get()->Error(L"GuiManager: ExecuteGuiScript: failed to load collection, named "+data[i]->Name);
				continue;
			}
			// delete rest of the object //
			goto guiprocessguifileloopdeleteprocessedobject;
		}
		if(data[i]->TName == L"GuiObject"){

			// try to load //
			if(!BaseGuiObject::LoadFromFileStructure(this, TempOs, *data[i], sheet)){

				// report error //
				Logger::Get()->Error(L"GuiManager: ExecuteGuiScript: failed to load GuiObject, named "+data[i]->Name);
				continue;
			}
			// delete rest of the object //
			goto guiprocessguifileloopdeleteprocessedobject;
		}

		Logger::Get()->Error(L"GuiManager: ExecuteGuiScript: Unrecognized type! typename: "+data[i]->TName);

guiprocessguifileloopdeleteprocessedobject:

		// delete current //
		data.erase(data.begin()+i);
		i--;
	}

	for(size_t i = 0; i < TempOs.size(); i++){

		// add to real objects //
		AddGuiObject(TempOs[i]);
	}

	return true;
}
// ------------------------------------ //
DLLEXPORT void Leviathan::Gui::GuiManager::SetMouseFile(const wstring &file){

	if(file == L"none" || Cursor){

		if(Cursor){
			Cursor->Close();
			Cursor->RemoveReference();
			Cursor = NULL;
		}
		// show default window cursor //
		Engine::GetEngine()->GetDefinition()->GetWindow()->SetHideCursor(false);
		if(file == L"none")
			return;
	}


	Cursor = WindowContext->LoadMouseCursor(Convert::WstringToString(file).c_str());
	if(Cursor){

		Cursor->Hide();
		// hide window cursor //
		Engine::GetEngine()->GetDefinition()->GetWindow()->SetHideCursor(true);
	}
}
// ------------------------------------ //
void Leviathan::Gui::GuiManager::UpdateArrays(){
	if(!ObjectAmountChanged)
		return;

	ObjectAmountChanged = false;
	for(unsigned int i = 0; i < Objects.size(); i++){
		// check does it contain renderable data //
		if(Objects[i]->ObjectFlags & GUIOBJECTHAS_RENDERABLE){

		}
	}
}
// ----------------- event handler part --------------------- //
bool Leviathan::Gui::GuiManager::CallEvent(Event* pEvent){
	// loop through listeners and call events //
	int returval = 0;
	for(size_t i = 0; i < Objects.size(); i++){
		// call
		returval = Objects[i]->OnEvent(&pEvent);
		// check for deletion and end if event got deleted //
		if(!pEvent){
			return true;
		}
	}

	// delete object ourselves //
	SAFE_DELETE(pEvent);
	// nobody wanted the event //
	return returval > 0 ? true: false;
}
// used to send hide events to individual objects //
int GuiManager::CallEventOnObject(BaseGuiObject* receive, Event* pEvent){
	// find right object
	int returval = -3;

	for(size_t i = 0; i < Objects.size(); i++){
		if(Objects[i] == receive){
			// call
			returval = Objects[i]->OnEvent(&pEvent);
			break;
		}
	}

	// delete object ourselves (if it still exists) //
	SAFE_DELETE(pEvent);
	return returval;
}
// ----------------- collection managing --------------------- //
void GuiManager::AddCollection(GuiCollection* add){
	Collections.push_back(add);
}
GuiCollection* Leviathan::Gui::GuiManager::GetCollection(const int &id, const wstring &name){
	// look for collection based on id or name //
	for(size_t i = 0; i < Collections.size(); i++){
		if(id >= 0){
			if(Collections[i]->GetID() != id){
				// no match //
				continue;
			}
		} else {
			// name should be specified, check for it //
			if(Collections[i]->GetName() != name){
				continue; 
			}
		}

		// match
		return Collections[i];
	}

	return NULL;
}
// -------------------------------------- //
// code from Rocket Ogre sample //
void Leviathan::Gui::GuiManager::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation){
	// we render Rocket at the same time with OGRE overlay //
	if(queueGroupId == Ogre::RENDER_QUEUE_OVERLAY && Visible){

		WindowContext->Update();

		ConfigureRenderSystem();
		WindowContext->Render();
	}
}

// Configures Ogre's rendering system for rendering Rocket.
void Leviathan::Gui::GuiManager::ConfigureRenderSystem()
{
	Ogre::RenderSystem* render_system = Ogre::Root::getSingleton().getRenderSystem();

	// Set up the projection and view matrices.
	Ogre::Matrix4 projection_matrix;
	BuildProjectionMatrix(projection_matrix);
	render_system->_setProjectionMatrix(projection_matrix);
	render_system->_setViewMatrix(Ogre::Matrix4::IDENTITY);

	// Disable lighting, as all of Rocket's geometry is unlit.
	render_system->setLightingEnabled(false);
	// Disable depth-buffering; all of the geometry is already depth-sorted.
	render_system->_setDepthBufferParams(false, false);
	// Rocket generates anti-clockwise geometry, so enable clockwise-culling.
	render_system->_setCullingMode(Ogre::CULL_CLOCKWISE);
	// Disable fogging.
	render_system->_setFog(Ogre::FOG_NONE);
	// Enable writing to all four channels.
	render_system->_setColourBufferWriteEnabled(true, true, true, true);
	// Unbind any vertex or fragment programs bound previously by the application.
	render_system->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
	render_system->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);

	// Set texture settings to clamp along both axes.
	Ogre::TextureUnitState::UVWAddressingMode addressing_mode;
	addressing_mode.u = Ogre::TextureUnitState::TAM_CLAMP;
	addressing_mode.v = Ogre::TextureUnitState::TAM_CLAMP;
	addressing_mode.w = Ogre::TextureUnitState::TAM_CLAMP;
	render_system->_setTextureAddressingMode(0, addressing_mode);

	// Set the texture coordinates for unit 0 to be read from unit 0.
	render_system->_setTextureCoordSet(0, 0);
	// Disable texture coordinate calculation.
	render_system->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
	// Enable linear filtering; images should be rendering 1 texel == 1 pixel, so point filtering could be used
	// except in the case of scaling tiled decorators.
	render_system->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
	// Disable texture coordinate transforms.
	render_system->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
	// Reject pixels with an alpha of 0.
	render_system->_setAlphaRejectSettings(Ogre::CMPF_GREATER, 0, false);
	// Disable all texture units but the first.
	render_system->_disableTextureUnitsFrom(1);

	// Enable simple alpha blending.
	render_system->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

	// Disable depth bias.
	render_system->_setDepthBias(0, 0);
}

// Builds an OpenGL-style orthographic projection matrix.
void Leviathan::Gui::GuiManager::BuildProjectionMatrix(Ogre::Matrix4& projection_matrix)
{
	float z_near = -1;
	float z_far = 1;

	projection_matrix = Ogre::Matrix4::ZERO;

	// Set up matrices.
	projection_matrix[0][0] = 2.0f / ThisRenderer->GetOwningWindow()->GetWidth();
	projection_matrix[0][3]= -1.0000000f;
	projection_matrix[1][1]= -2.0f / ThisRenderer->GetOwningWindow()->GetHeight();
	projection_matrix[1][3]= 1.0000000f;
	projection_matrix[2][2]= -2.0f / (z_far - z_near);
	projection_matrix[3][3]= 1.0000000f;
}